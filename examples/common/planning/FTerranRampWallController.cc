#include "common/planning/FTerranRampWallController.h"

#include <array>
#include <cmath>
#include <limits>

namespace sc2
{
namespace
{

constexpr float WallStructureMatchRadiusSquaredValue = 6.25f;
constexpr float RampThreatRadiusSquaredValue = 256.0f;

bool IsWallDepotType(const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOT:
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED:
            return true;
        default:
            return false;
    }
}

Point2D GetWallAxisDirection(const FRampWallDescriptor& RampWallDescriptorValue)
{
    const Point2D WallAxisValue = RampWallDescriptorValue.OutsideStagingPoint - RampWallDescriptorValue.InsideStagingPoint;
    const float WallAxisLengthSquaredValue =
        (WallAxisValue.x * WallAxisValue.x) + (WallAxisValue.y * WallAxisValue.y);
    if (WallAxisLengthSquaredValue <= 0.0001f)
    {
        return Point2D(1.0f, 0.0f);
    }

    const float InverseWallAxisLengthValue = 1.0f / std::sqrt(WallAxisLengthSquaredValue);
    return Point2D(WallAxisValue.x * InverseWallAxisLengthValue, WallAxisValue.y * InverseWallAxisLengthValue);
}

bool IsEnemyGroundThreatUnit(const Unit& EnemyUnitValue)
{
    return EnemyUnitValue.build_progress >= 1.0f && !EnemyUnitValue.is_flying && !EnemyUnitValue.is_building;
}

bool IsEnemyUnitInsideWall(const Unit& EnemyUnitValue, const FRampWallDescriptor& RampWallDescriptorValue)
{
    const Point2D EnemyPositionValue = Point2D(EnemyUnitValue.pos);
    const Point2D WallAxisDirectionValue = GetWallAxisDirection(RampWallDescriptorValue);
    const Point2D RelativePositionValue = EnemyPositionValue - RampWallDescriptorValue.WallCenterPoint;
    const float SignedWallAxisDistanceValue =
        (RelativePositionValue.x * WallAxisDirectionValue.x) + (RelativePositionValue.y * WallAxisDirectionValue.y);
    return SignedWallAxisDistanceValue <= 0.0f;
}

const Unit* FindWallDepotForSlot(const Units& SelfUnitsValue, const FBuildPlacementSlot& BuildPlacementSlotValue)
{
    const Unit* BestDepotUnitValue = nullptr;
    float BestDistanceSquaredValue = std::numeric_limits<float>::max();

    for (const Unit* SelfUnitValue : SelfUnitsValue)
    {
        if (SelfUnitValue == nullptr || SelfUnitValue->build_progress < 1.0f ||
            !IsWallDepotType(SelfUnitValue->unit_type.ToType()))
        {
            continue;
        }

        const float DistanceSquaredValue =
            DistanceSquared2D(Point2D(SelfUnitValue->pos), BuildPlacementSlotValue.BuildPoint);
        if (DistanceSquaredValue > WallStructureMatchRadiusSquaredValue || DistanceSquaredValue >= BestDistanceSquaredValue)
        {
            continue;
        }

        BestDistanceSquaredValue = DistanceSquaredValue;
        BestDepotUnitValue = SelfUnitValue;
    }

    return BestDepotUnitValue;
}

bool IsWallDepotAlreadyInDesiredState(const Unit& DepotUnitValue, const EWallGateState DesiredWallGateStateValue)
{
    switch (DesiredWallGateStateValue)
    {
        case EWallGateState::Open:
            return DepotUnitValue.unit_type.ToType() == UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED;
        case EWallGateState::Closed:
            return DepotUnitValue.unit_type.ToType() == UNIT_TYPEID::TERRAN_SUPPLYDEPOT;
        default:
            return true;
    }
}

bool HasMatchingMorphOrder(const Unit& DepotUnitValue, const ABILITY_ID AbilityIdValue)
{
    for (const UnitOrder& UnitOrderValue : DepotUnitValue.orders)
    {
        if (UnitOrderValue.ability_id == AbilityIdValue)
        {
            return true;
        }
    }

    return false;
}

bool IsEnemyGroundThreatNearWall(const Units& EnemyUnitsValue, const FRampWallDescriptor& RampWallDescriptorValue)
{
    for (const Unit* EnemyUnitValue : EnemyUnitsValue)
    {
        if (EnemyUnitValue == nullptr || EnemyUnitValue->build_progress < 1.0f ||
            !IsEnemyGroundThreatUnit(*EnemyUnitValue))
        {
            continue;
        }

        const Point2D EnemyPositionValue = Point2D(EnemyUnitValue->pos);
        if (DistanceSquared2D(EnemyPositionValue, RampWallDescriptorValue.OutsideStagingPoint) <=
                RampThreatRadiusSquaredValue ||
            IsEnemyUnitInsideWall(*EnemyUnitValue, RampWallDescriptorValue))
        {
            return true;
        }
    }

    return false;
}

}  // namespace

EWallGateState FTerranRampWallController::EvaluateDesiredWallGateState(
    const Units& SelfUnitsValue, const Units& EnemyUnitsValue, const FRampWallDescriptor& RampWallDescriptorValue) const
{
    if (!RampWallDescriptorValue.bIsValid)
    {
        return EWallGateState::Unavailable;
    }

    const Unit* LeftDepotUnitValue = FindWallDepotForSlot(SelfUnitsValue, RampWallDescriptorValue.LeftDepotSlot);
    const Unit* RightDepotUnitValue = FindWallDepotForSlot(SelfUnitsValue, RampWallDescriptorValue.RightDepotSlot);
    if (LeftDepotUnitValue == nullptr && RightDepotUnitValue == nullptr)
    {
        return EWallGateState::Unavailable;
    }

    return IsEnemyGroundThreatNearWall(EnemyUnitsValue, RampWallDescriptorValue) ? EWallGateState::Closed
                                                                                 : EWallGateState::Open;
}

void FTerranRampWallController::ProduceWallGateIntents(const Units& SelfUnitsValue,
                                                       const FRampWallDescriptor& RampWallDescriptorValue,
                                                       const EWallGateState DesiredWallGateStateValue,
                                                       FIntentBuffer& IntentBufferValue) const
{
    if (!RampWallDescriptorValue.bIsValid ||
        DesiredWallGateStateValue == EWallGateState::Unavailable)
    {
        return;
    }

    const ABILITY_ID DesiredAbilityIdValue =
        DesiredWallGateStateValue == EWallGateState::Open ? ABILITY_ID::MORPH_SUPPLYDEPOT_LOWER
                                                          : ABILITY_ID::MORPH_SUPPLYDEPOT_RAISE;

    const std::array<FBuildPlacementSlot, 2> WallDepotSlotsValue =
    {{
        RampWallDescriptorValue.LeftDepotSlot,
        RampWallDescriptorValue.RightDepotSlot,
    }};

    for (const FBuildPlacementSlot& WallDepotSlotValue : WallDepotSlotsValue)
    {
        const Unit* DepotUnitValue = FindWallDepotForSlot(SelfUnitsValue, WallDepotSlotValue);
        if (DepotUnitValue == nullptr || IsWallDepotAlreadyInDesiredState(*DepotUnitValue, DesiredWallGateStateValue) ||
            HasMatchingMorphOrder(*DepotUnitValue, DesiredAbilityIdValue) ||
            IntentBufferValue.HasIntentForActor(DepotUnitValue->tag))
        {
            continue;
        }

        IntentBufferValue.Add(FUnitIntent::CreateNoTarget(DepotUnitValue->tag, DesiredAbilityIdValue, 40,
                                                          EIntentDomain::StructureControl));
    }

    // Ensure any non-wall supply depots that are raised get lowered to avoid blocking placement.
    // This catches depots built outside the wall that default to raised state.
    if (DesiredWallGateStateValue == EWallGateState::Open)
    {
        for (const Unit* SelfUnitValue : SelfUnitsValue)
        {
            if (SelfUnitValue == nullptr || SelfUnitValue->build_progress < 1.0f ||
                SelfUnitValue->unit_type.ToType() != UNIT_TYPEID::TERRAN_SUPPLYDEPOT)
            {
                continue;
            }

            // Skip depots that are part of the wall (already handled above)
            const float LeftDistanceSquaredValue =
                DistanceSquared2D(Point2D(SelfUnitValue->pos), RampWallDescriptorValue.LeftDepotSlot.BuildPoint);
            const float RightDistanceSquaredValue =
                DistanceSquared2D(Point2D(SelfUnitValue->pos), RampWallDescriptorValue.RightDepotSlot.BuildPoint);
            if (LeftDistanceSquaredValue <= WallStructureMatchRadiusSquaredValue ||
                RightDistanceSquaredValue <= WallStructureMatchRadiusSquaredValue)
            {
                continue;
            }

            if (HasMatchingMorphOrder(*SelfUnitValue, ABILITY_ID::MORPH_SUPPLYDEPOT_LOWER) ||
                IntentBufferValue.HasIntentForActor(SelfUnitValue->tag))
            {
                continue;
            }

            IntentBufferValue.Add(FUnitIntent::CreateNoTarget(SelfUnitValue->tag,
                                                              ABILITY_ID::MORPH_SUPPLYDEPOT_LOWER, 30,
                                                              EIntentDomain::StructureControl));
        }
    }
}

}  // namespace sc2
