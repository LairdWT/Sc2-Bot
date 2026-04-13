#include "common/services/FTerranMainBaseLayoutRegistry.h"

#include <algorithm>
#include <cctype>
#include <cmath>

#include "common/catalogs/FMapLayoutDictionary.h"
#include "common/services/EBuildPlacementFootprintPolicy.h"
#include "common/services/EBuildPlacementSlotType.h"
#include "common/services/FRampOrientationConstants.h"
#include "sc2api/sc2_map_info.h"

namespace sc2
{
namespace
{

enum class EStartLocationBucket
{
    Unknown,
    UpperLeft,
    UpperRight,
    LowerLeft,
    LowerRight,
};

std::string NormalizeMapName(const std::string& MapNameValue)
{
    std::string NormalizedMapNameValue;
    NormalizedMapNameValue.reserve(MapNameValue.size());

    for (const char CharacterValue : MapNameValue)
    {
        if (!std::isalnum(static_cast<unsigned char>(CharacterValue)))
        {
            continue;
        }

        NormalizedMapNameValue.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(CharacterValue))));
    }

    return NormalizedMapNameValue;
}

bool IsBelShirVestigeMap(const std::string& NormalizedMapNameValue)
{
    return NormalizedMapNameValue.find("belshirvestige") != std::string::npos;
}

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

Point2D GetClockwiseLateralDirection(const Point2D& ForwardDirectionValue)
{
    return Point2D(ForwardDirectionValue.y, -ForwardDirectionValue.x);
}

Point2D ProjectPlacementOffset(const Point2D& AnchorPointValue, const Point2D& ForwardDirectionValue,
                               const Point2D& LateralDirectionValue, const Point2D& OffsetValue)
{
    return Point2D(AnchorPointValue.x + (LateralDirectionValue.x * OffsetValue.x) +
                       (ForwardDirectionValue.x * OffsetValue.y),
                   AnchorPointValue.y + (LateralDirectionValue.y * OffsetValue.x) +
                       (ForwardDirectionValue.y * OffsetValue.y));
}

bool TryGetPlayableBounds(const GameInfo* GameInfoPtrValue, const FBuildPlacementContext& BuildPlacementContextValue,
                         Point2D& OutPlayableMinValue, Point2D& OutPlayableMaxValue)
{
    if (GameInfoPtrValue != nullptr && GameInfoPtrValue->playable_max.x > GameInfoPtrValue->playable_min.x &&
        GameInfoPtrValue->playable_max.y > GameInfoPtrValue->playable_min.y)
    {
        OutPlayableMinValue = GameInfoPtrValue->playable_min;
        OutPlayableMaxValue = GameInfoPtrValue->playable_max;
        return true;
    }

    if (!BuildPlacementContextValue.HasPlayableBounds())
    {
        return false;
    }

    OutPlayableMinValue = BuildPlacementContextValue.PlayableMin;
    OutPlayableMaxValue = BuildPlacementContextValue.PlayableMax;
    return true;
}

EStartLocationBucket GetStartLocationBucket(const Point2D& BaseLocationValue, const Point2D& PlayableMinValue,
                                            const Point2D& PlayableMaxValue)
{
    const float CenterXValue = (PlayableMinValue.x + PlayableMaxValue.x) * 0.5f;
    const float CenterYValue = (PlayableMinValue.y + PlayableMaxValue.y) * 0.5f;
    const bool IsLeftSideValue = BaseLocationValue.x <= CenterXValue;
    const bool IsUpperSideValue = BaseLocationValue.y >= CenterYValue;

    if (IsLeftSideValue && IsUpperSideValue)
    {
        return EStartLocationBucket::UpperLeft;
    }

    if (!IsLeftSideValue && IsUpperSideValue)
    {
        return EStartLocationBucket::UpperRight;
    }

    if (IsLeftSideValue && !IsUpperSideValue)
    {
        return EStartLocationBucket::LowerLeft;
    }

    return EStartLocationBucket::LowerRight;
}

FBuildPlacementSlot CreatePlacementSlot(const EBuildPlacementSlotType SlotTypeValue,
                                        const EBuildPlacementFootprintPolicy FootprintPolicyValue,
                                        const Point2D& BuildPointValue, const uint8_t OrdinalValue)
{
    FBuildPlacementSlot BuildPlacementSlotValue;
    BuildPlacementSlotValue.SlotId.SlotType = SlotTypeValue;
    BuildPlacementSlotValue.SlotId.Ordinal = OrdinalValue;
    BuildPlacementSlotValue.FootprintPolicy = FootprintPolicyValue;
    BuildPlacementSlotValue.BuildPoint = BuildPointValue;
    return BuildPlacementSlotValue;
}

Point2D MirrorPointAcrossPlayableBounds(const Point2D& PointValue, const Point2D& PlayableMinValue,
                                        const Point2D& PlayableMaxValue)
{
    return Point2D(PlayableMinValue.x + PlayableMaxValue.x - PointValue.x,
                   PlayableMinValue.y + PlayableMaxValue.y - PointValue.y);
}

Point2D MirrorPointAcrossHorizontalBounds(const Point2D& PointValue, const Point2D& PlayableMinValue,
                                          const Point2D& PlayableMaxValue)
{
    return Point2D(PointValue.x, PlayableMinValue.y + PlayableMaxValue.y - PointValue.y);
}

void MirrorPlacementSlotsAcrossHorizontalBounds(const std::vector<FBuildPlacementSlot>& SourcePlacementSlotsValue,
                                                const Point2D& PlayableMinValue,
                                                const Point2D& PlayableMaxValue,
                                                std::vector<FBuildPlacementSlot>& OutPlacementSlotsValue)
{
    OutPlacementSlotsValue.clear();
    OutPlacementSlotsValue.reserve(SourcePlacementSlotsValue.size());

    for (const FBuildPlacementSlot& SourcePlacementSlotValue : SourcePlacementSlotsValue)
    {
        OutPlacementSlotsValue.push_back(CreatePlacementSlot(SourcePlacementSlotValue.SlotId.SlotType,
                                                             SourcePlacementSlotValue.FootprintPolicy,
                                                             MirrorPointAcrossHorizontalBounds(
                                                                 SourcePlacementSlotValue.BuildPoint,
                                                                 PlayableMinValue, PlayableMaxValue),
                                                             SourcePlacementSlotValue.SlotId.Ordinal));
    }
}

Point2D ComputeNaturalEntranceAssemblyAnchorPoint(const FBuildPlacementContext& BuildPlacementContextValue)
{
    if (!BuildPlacementContextValue.RampWallDescriptor.bIsValid)
    {
        return Point2D();
    }

    Point2D NaturalEntranceDirectionValue;
    if (BuildPlacementContextValue.HasNaturalLocation())
    {
        NaturalEntranceDirectionValue = GetNormalizedDirection(BuildPlacementContextValue.NaturalLocation -
                                                              BuildPlacementContextValue.RampWallDescriptor.OutsideStagingPoint);
    }
    else
    {
        NaturalEntranceDirectionValue = GetNormalizedDirection(
            BuildPlacementContextValue.RampWallDescriptor.OutsideStagingPoint -
            BuildPlacementContextValue.RampWallDescriptor.WallCenterPoint);
    }

    float NaturalEntranceRallyForwardOffsetValue = 18.0f;
    if (BuildPlacementContextValue.HasNaturalLocation())
    {
        const Point2D OutsideToNaturalVectorValue =
            BuildPlacementContextValue.NaturalLocation - BuildPlacementContextValue.RampWallDescriptor.OutsideStagingPoint;
        const float OutsideToNaturalDistanceSquaredValue =
            (OutsideToNaturalVectorValue.x * OutsideToNaturalVectorValue.x) +
            (OutsideToNaturalVectorValue.y * OutsideToNaturalVectorValue.y);
        const float OutsideToNaturalDistanceValue =
            OutsideToNaturalDistanceSquaredValue > 0.0f ? std::sqrt(OutsideToNaturalDistanceSquaredValue) : 0.0f;
        NaturalEntranceRallyForwardOffsetValue =
            std::max(6.0f, std::min(18.0f, OutsideToNaturalDistanceValue * 0.6f));
    }

    return BuildPlacementContextValue.RampWallDescriptor.OutsideStagingPoint +
           (NaturalEntranceDirectionValue * NaturalEntranceRallyForwardOffsetValue);
}

void PopulateProductionSlotsFromRampWall(const FRampWallDescriptor& RampWallDescriptorValue,
                                         const Point2D& BaseLocationValue,
                                         FMainBaseLayoutDescriptor& OutMainBaseLayoutDescriptorValue)
{
    // Production column layout: depth step and stagger step are read from
    // FRampOrientationConstants based on ramp orientation. When FMapSpawnLayout
    // data is available, these values come from compiled map data instead.
    // Column extends along Y-axis into base, lateral in -X for addon clearance.

    const Point2D BarracksPositionValue = RampWallDescriptorValue.BarracksSlot.BuildPoint;

    // Determine column parameters from ramp orientation
    const ERampOrientation RampOrientationValue = DetermineRampOrientation(
        RampWallDescriptorValue.WallCenterPoint, RampWallDescriptorValue.InsideStagingPoint);
    const float DepthStepValue = GetProductionColumnDepthStep(RampOrientationValue);
    const float StaggerStepValue = GetProductionColumnStaggerStep(RampOrientationValue);
    constexpr float LateralStepValue = ProductionColumnLateralStepValue;

    // Column 1: Barracks(wall) → Factory → Starport, staggered 1 unit per row away from ramp
    const Point2D FactoryPositionValue = Point2D(
        BarracksPositionValue.x + StaggerStepValue,
        BarracksPositionValue.y + DepthStepValue);
    const Point2D StarportPositionValue = Point2D(
        BarracksPositionValue.x + (StaggerStepValue * 2.0f),
        BarracksPositionValue.y + (DepthStepValue * 2.0f));

    // Column 2: 2nd Barracks → 3rd Barracks, offset -6 X from column 1
    const Point2D SecondBarracksPositionValue = Point2D(
        BarracksPositionValue.x + LateralStepValue,
        BarracksPositionValue.y);
    const Point2D ThirdBarracksPositionValue = Point2D(
        SecondBarracksPositionValue.x,
        SecondBarracksPositionValue.y + DepthStepValue);

    // Column 3: 4th and 5th production, offset -6 X from column 2
    const Point2D FourthProductionPositionValue = Point2D(
        SecondBarracksPositionValue.x + LateralStepValue,
        BarracksPositionValue.y);
    const Point2D FifthProductionPositionValue = Point2D(
        FourthProductionPositionValue.x,
        FourthProductionPositionValue.y + DepthStepValue);

    // Engineering bays: behind starport, deeper into base, continuing stagger
    const Point2D FirstEngBayPositionValue = Point2D(
        BarracksPositionValue.x + (StaggerStepValue * 3.0f),
        BarracksPositionValue.y + (DepthStepValue * 3.0f));
    const Point2D SecondEngBayPositionValue = Point2D(
        FirstEngBayPositionValue.x + LateralStepValue,
        FirstEngBayPositionValue.y);

    OutMainBaseLayoutDescriptorValue.FactoryWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainFactoryWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        FactoryPositionValue, 0U));
    OutMainBaseLayoutDescriptorValue.StarportWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainStarportWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        StarportPositionValue, 0U));
    OutMainBaseLayoutDescriptorValue.BarracksWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainBarracksWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        SecondBarracksPositionValue, 0U));

    // Production rail: generic addon-cleared slots for additional barracks
    OutMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        SecondBarracksPositionValue, 0U));
    OutMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        ThirdBarracksPositionValue, 1U));
    OutMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        FourthProductionPositionValue, 2U));
    OutMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        FifthProductionPositionValue, 3U));

    // Engineering bay slots (structure-only, no addon)
    OutMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::StructureOnly,
        FirstEngBayPositionValue, 4U));
    OutMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::StructureOnly,
        SecondEngBayPositionValue, 5U));
}

void PopulateNaturalEntranceLayout(const FBuildPlacementContext& BuildPlacementContextValue,
                                   FMainBaseLayoutDescriptor& OutMainBaseLayoutDescriptorValue)
{
    // Natural entrance wall: Depot-Bunker-Depot front row, Depot behind filling gap.
    // Depot = 2x2, Bunker = 3x3. Lateral offset 3.5 fits depot beside bunker with contact.
    constexpr float NaturalEntranceWallForwardOffsetValue = 4.0f;
    constexpr float NaturalEntranceDepotLateralOffsetValue = 3.5f;
    constexpr float NaturalEntranceBackDepotForwardOffsetValue = -2.5f;
    if (!BuildPlacementContextValue.RampWallDescriptor.bIsValid)
    {
        return;
    }

    Point2D NaturalEntranceDirectionValue;
    if (BuildPlacementContextValue.HasNaturalLocation())
    {
        NaturalEntranceDirectionValue = GetNormalizedDirection(BuildPlacementContextValue.NaturalLocation -
                                                              BuildPlacementContextValue.RampWallDescriptor.OutsideStagingPoint);
    }
    else
    {
        NaturalEntranceDirectionValue = GetNormalizedDirection(
            BuildPlacementContextValue.RampWallDescriptor.OutsideStagingPoint -
            BuildPlacementContextValue.RampWallDescriptor.WallCenterPoint);
    }

    const Point2D NaturalEntranceLateralDirectionValue =
        GetClockwiseLateralDirection(NaturalEntranceDirectionValue);
    const Point2D NaturalEntranceWallCenterPointValue =
        BuildPlacementContextValue.RampWallDescriptor.OutsideStagingPoint +
        (NaturalEntranceDirectionValue * NaturalEntranceWallForwardOffsetValue);
    OutMainBaseLayoutDescriptorValue.NaturalEntranceArmyRallyAnchorPoint =
        ComputeNaturalEntranceAssemblyAnchorPoint(BuildPlacementContextValue);

    // Front row: Left Depot — Bunker (center) — Right Depot
    OutMainBaseLayoutDescriptorValue.NaturalEntranceWallDepotSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::NaturalEntranceDepotLeft,
        EBuildPlacementFootprintPolicy::StructureOnly,
        Point2D(NaturalEntranceWallCenterPointValue.x +
                    (NaturalEntranceLateralDirectionValue.x * NaturalEntranceDepotLateralOffsetValue),
                NaturalEntranceWallCenterPointValue.y +
                    (NaturalEntranceLateralDirectionValue.y * NaturalEntranceDepotLateralOffsetValue)),
        0U));
    OutMainBaseLayoutDescriptorValue.NaturalEntranceBunkerSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::NaturalEntranceBunker,
        EBuildPlacementFootprintPolicy::StructureOnly,
        NaturalEntranceWallCenterPointValue,
        0U));
    OutMainBaseLayoutDescriptorValue.NaturalEntranceWallDepotSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::NaturalEntranceDepotRight,
        EBuildPlacementFootprintPolicy::StructureOnly,
        Point2D(NaturalEntranceWallCenterPointValue.x -
                    (NaturalEntranceLateralDirectionValue.x * NaturalEntranceDepotLateralOffsetValue),
                NaturalEntranceWallCenterPointValue.y -
                    (NaturalEntranceLateralDirectionValue.y * NaturalEntranceDepotLateralOffsetValue)),
        0U));

    // Back row: Center Depot behind bunker, filling gap
    OutMainBaseLayoutDescriptorValue.NaturalEntranceWallDepotSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::NaturalEntranceDepotCenter,
        EBuildPlacementFootprintPolicy::StructureOnly,
        Point2D(NaturalEntranceWallCenterPointValue.x +
                    (NaturalEntranceDirectionValue.x * NaturalEntranceBackDepotForwardOffsetValue),
                NaturalEntranceWallCenterPointValue.y +
                    (NaturalEntranceDirectionValue.y * NaturalEntranceBackDepotForwardOffsetValue)),
        0U));
}

}  // namespace

bool FTerranMainBaseLayoutRegistry::TryGetAuthoredMainBaseLayout(
    const GameInfo* GameInfoPtrValue, const FBuildPlacementContext& BuildPlacementContextValue,
    FMainBaseLayoutDescriptor& OutMainBaseLayoutDescriptorValue) const
{
    OutMainBaseLayoutDescriptorValue.Reset();

    const std::string MapNameValue =
        GameInfoPtrValue != nullptr && !GameInfoPtrValue->map_name.empty() ? GameInfoPtrValue->map_name
                                                                           : BuildPlacementContextValue.MapName;
    const std::string NormalizedMapNameValue = FMapLayoutDictionary::NormalizeMapName(MapNameValue);
    const FMapDescriptor* MapDescriptorPtrValue = FMapLayoutDictionary::TryGetMapByName(NormalizedMapNameValue);
    if (MapDescriptorPtrValue == nullptr)
    {
        return false;
    }

    const FMapSpawnLayout* SpawnLayoutPtrValue =
        FMapLayoutDictionary::TryGetSpawnLayout(*MapDescriptorPtrValue, BuildPlacementContextValue.BaseLocation);
    if (SpawnLayoutPtrValue == nullptr)
    {
        return false;
    }

    if (!BuildPlacementContextValue.RampWallDescriptor.bIsValid)
    {
        return false;
    }

    const Point2D MainBaseDepthDirectionValue = GetNormalizedDirection(
        BuildPlacementContextValue.RampWallDescriptor.InsideStagingPoint -
        BuildPlacementContextValue.RampWallDescriptor.WallCenterPoint);
    const Point2D LayoutAnchorPointValue =
        BuildPlacementContextValue.RampWallDescriptor.WallCenterPoint + (MainBaseDepthDirectionValue * 6.0f);
    OutMainBaseLayoutDescriptorValue.LayoutAnchorPoint = LayoutAnchorPointValue;
    OutMainBaseLayoutDescriptorValue.bUsesAuthoredProductionLayout = true;
    PopulateNaturalEntranceLayout(BuildPlacementContextValue, OutMainBaseLayoutDescriptorValue);
    OutMainBaseLayoutDescriptorValue.ArmyAssemblyAnchorPoint =
        OutMainBaseLayoutDescriptorValue.NaturalEntranceArmyRallyAnchorPoint;
    OutMainBaseLayoutDescriptorValue.ProductionClearanceAnchorPoint =
        LayoutAnchorPointValue + (MainBaseDepthDirectionValue * 8.0f);
    PopulateProductionSlotsFromRampWall(BuildPlacementContextValue.RampWallDescriptor,
                                         BuildPlacementContextValue.BaseLocation,
                                         OutMainBaseLayoutDescriptorValue);
    OutMainBaseLayoutDescriptorValue.bIsValid = true;
    return true;
}

}  // namespace sc2
