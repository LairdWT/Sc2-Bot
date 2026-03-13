#include "common/services/FTerranBuildPlacementService.h"

#include <array>
#include <cmath>
#include <limits>

#include "common/terran_models.h"

namespace sc2
{
namespace
{

struct FPlacementOffsetDescriptor
{
    float LateralOffset;
    float ForwardOffset;
};

Point2D GetNormalizedDirection(const Point2D& DirectionValue)
{
    const float LengthSquaredValue =
        (DirectionValue.x * DirectionValue.x) + (DirectionValue.y * DirectionValue.y);
    if (LengthSquaredValue <= 0.0001f)
    {
        return Point2D(1.0f, 0.0f);
    }

    const float InverseLengthValue = 1.0f / std::sqrt(LengthSquaredValue);
    return Point2D(DirectionValue.x * InverseLengthValue, DirectionValue.y * InverseLengthValue);
}

Point2D GetForwardDirection(const FBuildPlacementContext& BuildPlacementContextValue)
{
    if (!BuildPlacementContextValue.HasNaturalLocation())
    {
        return Point2D(1.0f, 0.0f);
    }

    return GetNormalizedDirection(BuildPlacementContextValue.NaturalLocation -
                                  BuildPlacementContextValue.BaseLocation);
}

Point2D GetLateralDirection(const Point2D& ForwardDirectionValue)
{
    return Point2D(-ForwardDirectionValue.y, ForwardDirectionValue.x);
}

Point2D ProjectOffsetToWorld(const Point2D& AnchorValue, const Point2D& ForwardDirectionValue,
                             const Point2D& LateralDirectionValue,
                             const FPlacementOffsetDescriptor& PlacementOffsetDescriptorValue)
{
    return Point2D(AnchorValue.x + (ForwardDirectionValue.x * PlacementOffsetDescriptorValue.ForwardOffset) +
                       (LateralDirectionValue.x * PlacementOffsetDescriptorValue.LateralOffset),
                   AnchorValue.y + (ForwardDirectionValue.y * PlacementOffsetDescriptorValue.ForwardOffset) +
                       (LateralDirectionValue.y * PlacementOffsetDescriptorValue.LateralOffset));
}

uint8_t GetNextSlotOrdinal(const std::vector<FBuildPlacementSlot>& PlacementSlotsValue,
                           const EBuildPlacementSlotType BuildPlacementSlotTypeValue)
{
    uint8_t SlotOrdinalValue = 0U;
    for (const FBuildPlacementSlot& BuildPlacementSlotValue : PlacementSlotsValue)
    {
        if (BuildPlacementSlotValue.SlotId.SlotType != BuildPlacementSlotTypeValue)
        {
            continue;
        }

        SlotOrdinalValue = static_cast<uint8_t>(SlotOrdinalValue + 1U);
    }

    return SlotOrdinalValue;
}

FBuildPlacementSlot CreatePlacementSlot(const EBuildPlacementSlotType BuildPlacementSlotTypeValue,
                                        const EBuildPlacementFootprintPolicy BuildPlacementFootprintPolicyValue,
                                        const Point2D& BuildPointValue,
                                        const uint8_t SlotOrdinalValue)
{
    FBuildPlacementSlot BuildPlacementSlotValue;
    BuildPlacementSlotValue.SlotId.SlotType = BuildPlacementSlotTypeValue;
    BuildPlacementSlotValue.SlotId.Ordinal = SlotOrdinalValue;
    BuildPlacementSlotValue.FootprintPolicy = BuildPlacementFootprintPolicyValue;
    BuildPlacementSlotValue.BuildPoint = BuildPointValue;
    return BuildPlacementSlotValue;
}

template <size_t OffsetCount>
void AppendPlacementSlots(const Point2D& AnchorValue, const Point2D& ForwardDirectionValue,
                          const Point2D& LateralDirectionValue,
                          const std::array<FPlacementOffsetDescriptor, OffsetCount>& PlacementOffsetsValue,
                          const EBuildPlacementSlotType BuildPlacementSlotTypeValue,
                          const EBuildPlacementFootprintPolicy BuildPlacementFootprintPolicyValue,
                          std::vector<FBuildPlacementSlot>& OutPlacementSlotsValue)
{
    OutPlacementSlotsValue.reserve(OutPlacementSlotsValue.size() + PlacementOffsetsValue.size());
    for (const FPlacementOffsetDescriptor& PlacementOffsetValue : PlacementOffsetsValue)
    {
        const Point2D BuildPointValue =
            ProjectOffsetToWorld(AnchorValue, ForwardDirectionValue, LateralDirectionValue, PlacementOffsetValue);
        const uint8_t SlotOrdinalValue =
            GetNextSlotOrdinal(OutPlacementSlotsValue, BuildPlacementSlotTypeValue);
        OutPlacementSlotsValue.push_back(CreatePlacementSlot(BuildPlacementSlotTypeValue,
                                                             BuildPlacementFootprintPolicyValue, BuildPointValue,
                                                             SlotOrdinalValue));
    }
}

uint32_t GetObservedAndInConstructionBuildingCount(const FGameStateDescriptor& GameStateDescriptorValue,
                                                   const UNIT_TYPEID BuildingTypeIdValue)
{
    const FBuildPlanningState& BuildPlanningStateValue = GameStateDescriptorValue.BuildPlanning;

    switch (BuildingTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOT:
            return static_cast<uint32_t>(
                       BuildPlanningStateValue.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                           UNIT_TYPEID::TERRAN_SUPPLYDEPOT)]) +
                   static_cast<uint32_t>(
                       BuildPlanningStateValue.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                           UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED)]) +
                   static_cast<uint32_t>(
                       BuildPlanningStateValue.ObservedBuildingsInConstruction[GetTerranBuildingTypeIndex(
                           UNIT_TYPEID::TERRAN_SUPPLYDEPOT)]);
        case UNIT_TYPEID::TERRAN_BARRACKS:
            return static_cast<uint32_t>(
                       BuildPlanningStateValue.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                           UNIT_TYPEID::TERRAN_BARRACKS)]) +
                   static_cast<uint32_t>(
                       BuildPlanningStateValue.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                           UNIT_TYPEID::TERRAN_BARRACKSFLYING)]) +
                   static_cast<uint32_t>(
                       BuildPlanningStateValue.ObservedBuildingsInConstruction[GetTerranBuildingTypeIndex(
                           UNIT_TYPEID::TERRAN_BARRACKS)]);
        case UNIT_TYPEID::TERRAN_FACTORY:
            return static_cast<uint32_t>(
                       BuildPlanningStateValue.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                           UNIT_TYPEID::TERRAN_FACTORY)]) +
                   static_cast<uint32_t>(
                       BuildPlanningStateValue.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                           UNIT_TYPEID::TERRAN_FACTORYFLYING)]) +
                   static_cast<uint32_t>(
                       BuildPlanningStateValue.ObservedBuildingsInConstruction[GetTerranBuildingTypeIndex(
                           UNIT_TYPEID::TERRAN_FACTORY)]);
        case UNIT_TYPEID::TERRAN_STARPORT:
            return static_cast<uint32_t>(
                       BuildPlanningStateValue.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                           UNIT_TYPEID::TERRAN_STARPORT)]) +
                   static_cast<uint32_t>(
                       BuildPlanningStateValue.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                           UNIT_TYPEID::TERRAN_STARPORTFLYING)]) +
                   static_cast<uint32_t>(
                       BuildPlanningStateValue.ObservedBuildingsInConstruction[GetTerranBuildingTypeIndex(
                           UNIT_TYPEID::TERRAN_STARPORT)]);
        default:
        {
            const size_t BuildingTypeIndexValue = GetTerranBuildingTypeIndex(BuildingTypeIdValue);
            if (!IsTerranBuildingTypeIndexValid(BuildingTypeIndexValue))
            {
                return 0U;
            }

            return static_cast<uint32_t>(BuildPlanningStateValue.ObservedBuildingCounts[BuildingTypeIndexValue]) +
                   static_cast<uint32_t>(BuildPlanningStateValue.ObservedBuildingsInConstruction[BuildingTypeIndexValue]);
        }
    }
}

bool DoesStructureAbilityRequireAddonClearance(const ABILITY_ID StructureAbilityIdValue)
{
    switch (StructureAbilityIdValue)
    {
        case ABILITY_ID::BUILD_BARRACKS:
        case ABILITY_ID::BUILD_FACTORY:
        case ABILITY_ID::BUILD_STARPORT:
            return true;
        default:
            return false;
    }
}

Point2D GetAddonFootprintCenter(const Point2D& StructureBuildPointValue)
{
    return Point2D(StructureBuildPointValue.x + 2.5f, StructureBuildPointValue.y - 0.5f);
}

bool DoesAddonFootprintSupportTerrain(const GameInfo& GameInfoValue, const Point2D& StructureBuildPointValue)
{
    const PlacementGrid PlacementGridValue(GameInfoValue);
    const Point2D AddonCenterValue = GetAddonFootprintCenter(StructureBuildPointValue);

    static const std::array<Point2D, 5> AddonSampleOffsetsValue =
    {{
        Point2D(0.0f, 0.0f),
        Point2D(-0.5f, -0.5f),
        Point2D(-0.5f, 0.5f),
        Point2D(0.5f, -0.5f),
        Point2D(0.5f, 0.5f),
    }};

    for (const Point2D& AddonSampleOffsetValue : AddonSampleOffsetsValue)
    {
        const Point2D SamplePointValue(AddonCenterValue.x + AddonSampleOffsetValue.x,
                                       AddonCenterValue.y + AddonSampleOffsetValue.y);
        if (!PlacementGridValue.IsPlacable(SamplePointValue))
        {
            return false;
        }
    }

    return true;
}

bool DoesPointSatisfyFootprintPolicy(const FFrameContext& FrameValue, const Point2D& BuildPointValue,
                                     const ABILITY_ID StructureAbilityIdValue,
                                     const EBuildPlacementFootprintPolicy BuildPlacementFootprintPolicyValue)
{
    switch (BuildPlacementFootprintPolicyValue)
    {
        case EBuildPlacementFootprintPolicy::StructureOnly:
            return true;
        case EBuildPlacementFootprintPolicy::RequiresAddonClearance:
            if (FrameValue.GameInfo == nullptr || !DoesStructureAbilityRequireAddonClearance(StructureAbilityIdValue))
            {
                return false;
            }

            return DoesAddonFootprintSupportTerrain(*FrameValue.GameInfo, BuildPointValue);
        default:
            return false;
    }
}

float GetCorridorPathableScore(const PathingGrid& PathingGridValue, const Point2D& SampleCenterValue,
                               const Point2D& LateralDirectionValue)
{
    uint32_t PathablePointCountValue = 0U;
    for (int LateralSampleIndexValue = -6; LateralSampleIndexValue <= 6; ++LateralSampleIndexValue)
    {
        const Point2D SamplePointValue(
            SampleCenterValue.x + (LateralDirectionValue.x * static_cast<float>(LateralSampleIndexValue)),
            SampleCenterValue.y + (LateralDirectionValue.y * static_cast<float>(LateralSampleIndexValue)));
        if (PathingGridValue.IsPathable(Point2DI(static_cast<int>(std::round(SamplePointValue.x)),
                                                 static_cast<int>(std::round(SamplePointValue.y)))))
        {
            ++PathablePointCountValue;
        }
    }

    return static_cast<float>(PathablePointCountValue);
}

Point2D FindRampWallCenterPoint(const FFrameContext& FrameValue,
                                const FBuildPlacementContext& BuildPlacementContextValue)
{
    const Point2D ForwardDirectionValue = GetForwardDirection(BuildPlacementContextValue);
    const Point2D LateralDirectionValue = GetLateralDirection(ForwardDirectionValue);
    const PathingGrid PathingGridValue(*FrameValue.GameInfo);

    float BestScoreValue = std::numeric_limits<float>::max();
    Point2D BestPointValue = BuildPlacementContextValue.BaseLocation +
                             Point2D(ForwardDirectionValue.x * 8.0f, ForwardDirectionValue.y * 8.0f);

    for (int ForwardSampleIndexValue = 8; ForwardSampleIndexValue <= 20; ++ForwardSampleIndexValue)
    {
        const Point2D SampleCenterValue(
            BuildPlacementContextValue.BaseLocation.x + (ForwardDirectionValue.x * static_cast<float>(ForwardSampleIndexValue)),
            BuildPlacementContextValue.BaseLocation.y + (ForwardDirectionValue.y * static_cast<float>(ForwardSampleIndexValue)));
        const float ScoreValue =
            GetCorridorPathableScore(PathingGridValue, SampleCenterValue, LateralDirectionValue);
        if (ScoreValue >= BestScoreValue)
        {
            continue;
        }

        BestScoreValue = ScoreValue;
        BestPointValue = SampleCenterValue;
    }

    return BestPointValue;
}

bool TryFindPlacementNearDesiredPoint(const FFrameContext& FrameValue, const ABILITY_ID StructureAbilityIdValue,
                                      const EBuildPlacementFootprintPolicy BuildPlacementFootprintPolicyValue,
                                      const Point2D& DesiredPointValue, Point2D& OutBuildPointValue)
{
    if (FrameValue.Query == nullptr || FrameValue.GameInfo == nullptr)
    {
        return false;
    }

    static const std::array<FPlacementOffsetDescriptor, 37> SearchOffsetsValue =
    {{
        {0.0f, 0.0f},
        {1.0f, 0.0f},
        {-1.0f, 0.0f},
        {0.0f, 1.0f},
        {0.0f, -1.0f},
        {2.0f, 0.0f},
        {-2.0f, 0.0f},
        {0.0f, 2.0f},
        {0.0f, -2.0f},
        {1.0f, 1.0f},
        {-1.0f, 1.0f},
        {1.0f, -1.0f},
        {-1.0f, -1.0f},
        {3.0f, 0.0f},
        {-3.0f, 0.0f},
        {0.0f, 3.0f},
        {0.0f, -3.0f},
        {2.0f, 1.0f},
        {-2.0f, 1.0f},
        {2.0f, -1.0f},
        {-2.0f, -1.0f},
        {1.0f, 2.0f},
        {-1.0f, 2.0f},
        {1.0f, -2.0f},
        {-1.0f, -2.0f},
        {4.0f, 0.0f},
        {-4.0f, 0.0f},
        {0.0f, 4.0f},
        {0.0f, -4.0f},
        {3.0f, 1.0f},
        {-3.0f, 1.0f},
        {3.0f, -1.0f},
        {-3.0f, -1.0f},
        {1.0f, 3.0f},
        {-1.0f, 3.0f},
        {1.0f, -3.0f},
        {-1.0f, -3.0f},
    }};

    for (const FPlacementOffsetDescriptor& SearchOffsetValue : SearchOffsetsValue)
    {
        const Point2D CandidatePointValue(
            DesiredPointValue.x + SearchOffsetValue.LateralOffset,
            DesiredPointValue.y + SearchOffsetValue.ForwardOffset);
        const Point2D ClampedCandidateValue = ClampToPlayable(*FrameValue.GameInfo, CandidatePointValue);
        if (!DoesPointSatisfyFootprintPolicy(FrameValue, ClampedCandidateValue, StructureAbilityIdValue,
                                             BuildPlacementFootprintPolicyValue))
        {
            continue;
        }

        if (!FrameValue.Query->Placement(StructureAbilityIdValue, ClampedCandidateValue))
        {
            continue;
        }

        OutBuildPointValue = ClampedCandidateValue;
        return true;
    }

    return false;
}

FRampWallDescriptor CreateDeterministicRampWallDescriptor(const FBuildPlacementContext& BuildPlacementContextValue)
{
    const Point2D ForwardDirectionValue = GetForwardDirection(BuildPlacementContextValue);
    const Point2D LateralDirectionValue = GetLateralDirection(ForwardDirectionValue);
    const Point2D PrimaryAnchorValue(
        BuildPlacementContextValue.BaseLocation.x + (ForwardDirectionValue.x * 6.0f),
        BuildPlacementContextValue.BaseLocation.y + (ForwardDirectionValue.y * 6.0f));

    FRampWallDescriptor RampWallDescriptorValue;
    RampWallDescriptorValue.bIsValid = true;
    RampWallDescriptorValue.WallCenterPoint =
        ProjectOffsetToWorld(PrimaryAnchorValue, ForwardDirectionValue, LateralDirectionValue, {0.0f, 2.5f});
    RampWallDescriptorValue.InsideStagingPoint =
        Point2D(RampWallDescriptorValue.WallCenterPoint.x - (ForwardDirectionValue.x * 4.0f),
                RampWallDescriptorValue.WallCenterPoint.y - (ForwardDirectionValue.y * 4.0f));
    RampWallDescriptorValue.OutsideStagingPoint =
        Point2D(RampWallDescriptorValue.WallCenterPoint.x + (ForwardDirectionValue.x * 6.0f),
                RampWallDescriptorValue.WallCenterPoint.y + (ForwardDirectionValue.y * 6.0f));
    RampWallDescriptorValue.LeftDepotSlot = CreatePlacementSlot(
        EBuildPlacementSlotType::MainRampDepotLeft, EBuildPlacementFootprintPolicy::StructureOnly,
        ProjectOffsetToWorld(PrimaryAnchorValue, ForwardDirectionValue, LateralDirectionValue, {3.0f, 1.0f}), 0U);
    RampWallDescriptorValue.BarracksSlot = CreatePlacementSlot(
        EBuildPlacementSlotType::MainRampBarracksWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        ProjectOffsetToWorld(PrimaryAnchorValue, ForwardDirectionValue, LateralDirectionValue, {0.0f, 5.0f}), 0U);
    RampWallDescriptorValue.RightDepotSlot = CreatePlacementSlot(
        EBuildPlacementSlotType::MainRampDepotRight, EBuildPlacementFootprintPolicy::StructureOnly,
        ProjectOffsetToWorld(PrimaryAnchorValue, ForwardDirectionValue, LateralDirectionValue, {-3.0f, 1.0f}), 0U);
    return RampWallDescriptorValue;
}

FRampWallDescriptor CreateDiscoveredRampWallDescriptor(const FFrameContext& FrameValue,
                                                       const FBuildPlacementContext& BuildPlacementContextValue)
{
    const Point2D ForwardDirectionValue = GetForwardDirection(BuildPlacementContextValue);
    const Point2D LateralDirectionValue = GetLateralDirection(ForwardDirectionValue);
    const Point2D RampCenterPointValue = FindRampWallCenterPoint(FrameValue, BuildPlacementContextValue);

    const Point2D DesiredLeftDepotPointValue(
        RampCenterPointValue.x + (LateralDirectionValue.x * 3.0f) + (ForwardDirectionValue.x * 0.5f),
        RampCenterPointValue.y + (LateralDirectionValue.y * 3.0f) + (ForwardDirectionValue.y * 0.5f));
    const Point2D DesiredRightDepotPointValue(
        RampCenterPointValue.x - (LateralDirectionValue.x * 3.0f) + (ForwardDirectionValue.x * 0.5f),
        RampCenterPointValue.y - (LateralDirectionValue.y * 3.0f) + (ForwardDirectionValue.y * 0.5f));
    const Point2D DesiredBarracksPointValue(
        RampCenterPointValue.x + (ForwardDirectionValue.x * 3.5f),
        RampCenterPointValue.y + (ForwardDirectionValue.y * 3.5f));

    Point2D LeftDepotBuildPointValue;
    Point2D BarracksBuildPointValue;
    Point2D RightDepotBuildPointValue;
    if (!TryFindPlacementNearDesiredPoint(FrameValue, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                          EBuildPlacementFootprintPolicy::StructureOnly,
                                          DesiredLeftDepotPointValue, LeftDepotBuildPointValue) ||
        !TryFindPlacementNearDesiredPoint(FrameValue, ABILITY_ID::BUILD_BARRACKS,
                                          EBuildPlacementFootprintPolicy::RequiresAddonClearance,
                                          DesiredBarracksPointValue, BarracksBuildPointValue) ||
        !TryFindPlacementNearDesiredPoint(FrameValue, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                          EBuildPlacementFootprintPolicy::StructureOnly,
                                          DesiredRightDepotPointValue, RightDepotBuildPointValue))
    {
        FRampWallDescriptor InvalidRampWallDescriptorValue;
        InvalidRampWallDescriptorValue.WallCenterPoint = RampCenterPointValue;
        InvalidRampWallDescriptorValue.InsideStagingPoint =
            Point2D(RampCenterPointValue.x - (ForwardDirectionValue.x * 5.0f),
                    RampCenterPointValue.y - (ForwardDirectionValue.y * 5.0f));
        InvalidRampWallDescriptorValue.OutsideStagingPoint =
            Point2D(RampCenterPointValue.x + (ForwardDirectionValue.x * 7.0f),
                    RampCenterPointValue.y + (ForwardDirectionValue.y * 7.0f));
        return InvalidRampWallDescriptorValue;
    }

    FRampWallDescriptor RampWallDescriptorValue;
    RampWallDescriptorValue.bIsValid = true;
    RampWallDescriptorValue.WallCenterPoint = RampCenterPointValue;
    RampWallDescriptorValue.InsideStagingPoint =
        Point2D(RampCenterPointValue.x - (ForwardDirectionValue.x * 5.0f),
                RampCenterPointValue.y - (ForwardDirectionValue.y * 5.0f));
    RampWallDescriptorValue.OutsideStagingPoint =
        Point2D(RampCenterPointValue.x + (ForwardDirectionValue.x * 7.0f),
                RampCenterPointValue.y + (ForwardDirectionValue.y * 7.0f));
    RampWallDescriptorValue.LeftDepotSlot = CreatePlacementSlot(
        EBuildPlacementSlotType::MainRampDepotLeft, EBuildPlacementFootprintPolicy::StructureOnly,
        LeftDepotBuildPointValue, 0U);
    RampWallDescriptorValue.BarracksSlot = CreatePlacementSlot(
        EBuildPlacementSlotType::MainRampBarracksWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance, BarracksBuildPointValue, 0U);
    RampWallDescriptorValue.RightDepotSlot = CreatePlacementSlot(
        EBuildPlacementSlotType::MainRampDepotRight, EBuildPlacementFootprintPolicy::StructureOnly,
        RightDepotBuildPointValue, 0U);
    return RampWallDescriptorValue;
}

}  // namespace

FRampWallDescriptor FTerranBuildPlacementService::GetRampWallDescriptor(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue) const
{
    if (!BuildPlacementContextValue.HasNaturalLocation())
    {
        return FRampWallDescriptor();
    }

    if (!FrameValue.IsValid() || FrameValue.Query == nullptr)
    {
        return CreateDeterministicRampWallDescriptor(BuildPlacementContextValue);
    }

    return CreateDiscoveredRampWallDescriptor(FrameValue, BuildPlacementContextValue);
}

Point2D FTerranBuildPlacementService::GetPrimaryStructureAnchor(
    const FGameStateDescriptor& GameStateDescriptorValue, const FBuildPlacementContext& BuildPlacementContextValue) const
{
    (void)GameStateDescriptorValue;

    const Point2D ForwardDirectionValue = GetForwardDirection(BuildPlacementContextValue);
    return Point2D(BuildPlacementContextValue.BaseLocation.x + (ForwardDirectionValue.x * 6.0f),
                   BuildPlacementContextValue.BaseLocation.y + (ForwardDirectionValue.y * 6.0f));
}

Point2D FTerranBuildPlacementService::GetArmyAssemblyPoint(
    const FGameStateDescriptor& GameStateDescriptorValue, const FBuildPlacementContext& BuildPlacementContextValue) const
{
    (void)GameStateDescriptorValue;

    if (BuildPlacementContextValue.RampWallDescriptor.bIsValid)
    {
        return BuildPlacementContextValue.RampWallDescriptor.OutsideStagingPoint;
    }

    const Point2D PrimaryAnchorValue =
        GetPrimaryStructureAnchor(GameStateDescriptorValue, BuildPlacementContextValue);
    const Point2D ForwardDirectionValue = GetForwardDirection(BuildPlacementContextValue);
    return Point2D(PrimaryAnchorValue.x + (ForwardDirectionValue.x * 8.0f),
                   PrimaryAnchorValue.y + (ForwardDirectionValue.y * 8.0f));
}

std::vector<FBuildPlacementSlot> FTerranBuildPlacementService::GetStructurePlacementSlots(
    const FGameStateDescriptor& GameStateDescriptorValue, const ABILITY_ID StructureAbilityId,
    const FBuildPlacementContext& BuildPlacementContextValue) const
{
    const Point2D PrimaryAnchorValue = GetPrimaryStructureAnchor(GameStateDescriptorValue, BuildPlacementContextValue);
    const Point2D ForwardDirectionValue = GetForwardDirection(BuildPlacementContextValue);
    const Point2D LateralDirectionValue = GetLateralDirection(ForwardDirectionValue);

    static const std::array<FPlacementOffsetDescriptor, 2> NaturalApproachDepotOffsetValues =
    {{
        {6.0f, 6.0f},
        {-6.0f, 6.0f},
    }};

    static const std::array<FPlacementOffsetDescriptor, 6> SupportDepotOffsetValues =
    {{
        {8.0f, 12.0f},
        {-8.0f, 12.0f},
        {8.0f, 20.0f},
        {-8.0f, 20.0f},
        {14.0f, 16.0f},
        {-14.0f, 16.0f},
    }};

    static const std::array<FPlacementOffsetDescriptor, 6> ProductionWithAddonOffsetValues =
    {{
        {8.0f, 14.0f},
        {-8.0f, 14.0f},
        {8.0f, 22.0f},
        {-8.0f, 22.0f},
        {16.0f, 18.0f},
        {-16.0f, 18.0f},
    }};

    const FRampWallDescriptor RampWallDescriptorValue =
        BuildPlacementContextValue.RampWallDescriptor.bIsValid ? BuildPlacementContextValue.RampWallDescriptor
                                                               : FRampWallDescriptor();

    std::vector<FBuildPlacementSlot> PlacementSlotsValue;

    switch (StructureAbilityId)
    {
        case ABILITY_ID::BUILD_SUPPLYDEPOT:
        {
            if (RampWallDescriptorValue.bIsValid)
            {
                PlacementSlotsValue.push_back(RampWallDescriptorValue.LeftDepotSlot);
                PlacementSlotsValue.push_back(RampWallDescriptorValue.RightDepotSlot);
            }

            AppendPlacementSlots(PrimaryAnchorValue, ForwardDirectionValue, LateralDirectionValue,
                                 NaturalApproachDepotOffsetValues, EBuildPlacementSlotType::NaturalApproachDepot,
                                 EBuildPlacementFootprintPolicy::StructureOnly, PlacementSlotsValue);
            AppendPlacementSlots(PrimaryAnchorValue, ForwardDirectionValue, LateralDirectionValue,
                                 SupportDepotOffsetValues, EBuildPlacementSlotType::MainSupportStructure,
                                 EBuildPlacementFootprintPolicy::StructureOnly, PlacementSlotsValue);
            break;
        }
        case ABILITY_ID::BUILD_BARRACKS:
        {
            if (RampWallDescriptorValue.bIsValid)
            {
                PlacementSlotsValue.push_back(RampWallDescriptorValue.BarracksSlot);
            }

            AppendPlacementSlots(PrimaryAnchorValue, ForwardDirectionValue, LateralDirectionValue,
                                 ProductionWithAddonOffsetValues,
                                 EBuildPlacementSlotType::MainProductionWithAddon,
                                 EBuildPlacementFootprintPolicy::RequiresAddonClearance, PlacementSlotsValue);
            break;
        }
        case ABILITY_ID::BUILD_FACTORY:
        case ABILITY_ID::BUILD_STARPORT:
            AppendPlacementSlots(PrimaryAnchorValue, ForwardDirectionValue, LateralDirectionValue,
                                 ProductionWithAddonOffsetValues,
                                 EBuildPlacementSlotType::MainProductionWithAddon,
                                 EBuildPlacementFootprintPolicy::RequiresAddonClearance, PlacementSlotsValue);
            break;
        default:
        {
            const uint8_t SlotOrdinalValue =
                GetNextSlotOrdinal(PlacementSlotsValue, EBuildPlacementSlotType::MainSupportStructure);
            PlacementSlotsValue.push_back(CreatePlacementSlot(
                EBuildPlacementSlotType::MainSupportStructure, EBuildPlacementFootprintPolicy::StructureOnly,
                PrimaryAnchorValue, SlotOrdinalValue));
            break;
        }
    }

    return PlacementSlotsValue;
}

}  // namespace sc2
