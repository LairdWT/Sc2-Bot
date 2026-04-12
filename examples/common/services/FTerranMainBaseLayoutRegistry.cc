#include "common/services/FTerranMainBaseLayoutRegistry.h"

#include <algorithm>
#include <cctype>
#include <cmath>

#include "common/services/EBuildPlacementFootprintPolicy.h"
#include "common/services/EBuildPlacementSlotType.h"
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

void PopulateBelShirUpperLeftProductionSlots(FMainBaseLayoutDescriptor& OutMainBaseLayoutDescriptorValue)
{
    OutMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        Point2D(31.7782f, 159.222f),
        0U));
    OutMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        Point2D(27.7782f, 160.222f),
        1U));
    OutMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        Point2D(21.7782f, 160.222f),
        2U));
    OutMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        Point2D(15.7782f, 160.222f),
        3U));
    OutMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        Point2D(31.7782f, 165.222f),
        4U));
    OutMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        Point2D(27.7782f, 166.222f),
        5U));
    OutMainBaseLayoutDescriptorValue.BarracksWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainBarracksWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        Point2D(31.7782f, 159.222f),
        0U));
    OutMainBaseLayoutDescriptorValue.FactoryWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainFactoryWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        Point2D(41.7782f, 167.222f),
        0U));
    OutMainBaseLayoutDescriptorValue.StarportWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainStarportWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        Point2D(38.7782f, 170.222f),
        0U));
}

void PopulateBelShirLowerRightProductionSlots(FMainBaseLayoutDescriptor& OutMainBaseLayoutDescriptorValue)
{
    OutMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        Point2D(166.222f, 40.7782f),
        0U));
    OutMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        Point2D(171.222f, 38.7782f),
        1U));
    OutMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        Point2D(177.222f, 38.7782f),
        2U));
    OutMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        Point2D(183.222f, 38.7782f),
        3U));
    OutMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        Point2D(166.222f, 34.7782f),
        4U));
    OutMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        Point2D(171.222f, 32.7782f),
        5U));
    OutMainBaseLayoutDescriptorValue.BarracksWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainBarracksWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        Point2D(166.222f, 40.7782f),
        0U));
    OutMainBaseLayoutDescriptorValue.FactoryWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainFactoryWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        Point2D(155.222f, 29.7782f),
        0U));
    OutMainBaseLayoutDescriptorValue.StarportWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainStarportWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        Point2D(177.222f, 38.7782f),
        0U));
}

void PopulateBelShirAuthoredProductionSlots(const EStartLocationBucket StartLocationBucketValue,
                                            const Point2D& PlayableMinValue,
                                            const Point2D& PlayableMaxValue,
                                            FMainBaseLayoutDescriptor& OutMainBaseLayoutDescriptorValue)
{
    switch (StartLocationBucketValue)
    {
        case EStartLocationBucket::UpperLeft:
            PopulateBelShirUpperLeftProductionSlots(OutMainBaseLayoutDescriptorValue);
            return;
        case EStartLocationBucket::LowerRight:
            PopulateBelShirLowerRightProductionSlots(OutMainBaseLayoutDescriptorValue);
            return;
        case EStartLocationBucket::UpperRight:
        {
            FMainBaseLayoutDescriptor LowerRightTemplateValue;
            PopulateBelShirLowerRightProductionSlots(LowerRightTemplateValue);
            MirrorPlacementSlotsAcrossHorizontalBounds(LowerRightTemplateValue.ProductionRailWithAddonSlots,
                                                       PlayableMinValue, PlayableMaxValue,
                                                       OutMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots);
            MirrorPlacementSlotsAcrossHorizontalBounds(LowerRightTemplateValue.BarracksWithAddonSlots,
                                                       PlayableMinValue, PlayableMaxValue,
                                                       OutMainBaseLayoutDescriptorValue.BarracksWithAddonSlots);
            MirrorPlacementSlotsAcrossHorizontalBounds(LowerRightTemplateValue.FactoryWithAddonSlots,
                                                       PlayableMinValue, PlayableMaxValue,
                                                       OutMainBaseLayoutDescriptorValue.FactoryWithAddonSlots);
            MirrorPlacementSlotsAcrossHorizontalBounds(LowerRightTemplateValue.StarportWithAddonSlots,
                                                       PlayableMinValue, PlayableMaxValue,
                                                       OutMainBaseLayoutDescriptorValue.StarportWithAddonSlots);
            return;
        }
        case EStartLocationBucket::LowerLeft:
        {
            FMainBaseLayoutDescriptor UpperLeftTemplateValue;
            PopulateBelShirUpperLeftProductionSlots(UpperLeftTemplateValue);
            MirrorPlacementSlotsAcrossHorizontalBounds(UpperLeftTemplateValue.ProductionRailWithAddonSlots,
                                                       PlayableMinValue, PlayableMaxValue,
                                                       OutMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots);
            MirrorPlacementSlotsAcrossHorizontalBounds(UpperLeftTemplateValue.BarracksWithAddonSlots,
                                                       PlayableMinValue, PlayableMaxValue,
                                                       OutMainBaseLayoutDescriptorValue.BarracksWithAddonSlots);
            MirrorPlacementSlotsAcrossHorizontalBounds(UpperLeftTemplateValue.FactoryWithAddonSlots,
                                                       PlayableMinValue, PlayableMaxValue,
                                                       OutMainBaseLayoutDescriptorValue.FactoryWithAddonSlots);
            MirrorPlacementSlotsAcrossHorizontalBounds(UpperLeftTemplateValue.StarportWithAddonSlots,
                                                       PlayableMinValue, PlayableMaxValue,
                                                       OutMainBaseLayoutDescriptorValue.StarportWithAddonSlots);
            return;
        }
        case EStartLocationBucket::Unknown:
        default:
            return;
    }
}

void PopulateNaturalEntranceLayout(const FBuildPlacementContext& BuildPlacementContextValue,
                                   FMainBaseLayoutDescriptor& OutMainBaseLayoutDescriptorValue)
{
    constexpr float NaturalEntranceWallForwardOffsetValue = 4.0f;
    constexpr float NaturalEntranceWallLateralOffsetValue = 4.0f;
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
    OutMainBaseLayoutDescriptorValue.NaturalEntranceWallDepotSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::NaturalEntranceDepotLeft,
        EBuildPlacementFootprintPolicy::StructureOnly,
        NaturalEntranceWallCenterPointValue +
            (NaturalEntranceLateralDirectionValue * NaturalEntranceWallLateralOffsetValue),
        0U));
    OutMainBaseLayoutDescriptorValue.NaturalEntranceWallDepotSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::NaturalEntranceDepotRight,
        EBuildPlacementFootprintPolicy::StructureOnly,
        NaturalEntranceWallCenterPointValue -
            (NaturalEntranceLateralDirectionValue * NaturalEntranceWallLateralOffsetValue),
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
    const std::string NormalizedMapNameValue = NormalizeMapName(MapNameValue);
    if (!IsBelShirVestigeMap(NormalizedMapNameValue))
    {
        return false;
    }

    Point2D PlayableMinValue;
    Point2D PlayableMaxValue;
    if (!TryGetPlayableBounds(GameInfoPtrValue, BuildPlacementContextValue, PlayableMinValue, PlayableMaxValue))
    {
        return false;
    }

    const EStartLocationBucket StartLocationBucketValue =
        GetStartLocationBucket(BuildPlacementContextValue.BaseLocation, PlayableMinValue, PlayableMaxValue);
    if (StartLocationBucketValue == EStartLocationBucket::Unknown)
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
    const Point2D MainBaseLateralDirectionValue = GetClockwiseLateralDirection(MainBaseDepthDirectionValue);
    const Point2D LayoutAnchorPointValue =
        BuildPlacementContextValue.RampWallDescriptor.WallCenterPoint + (MainBaseDepthDirectionValue * 6.0f);
    OutMainBaseLayoutDescriptorValue.LayoutAnchorPoint = LayoutAnchorPointValue;
    OutMainBaseLayoutDescriptorValue.bUsesAuthoredProductionLayout = true;
    PopulateNaturalEntranceLayout(BuildPlacementContextValue, OutMainBaseLayoutDescriptorValue);
    OutMainBaseLayoutDescriptorValue.ArmyAssemblyAnchorPoint =
        OutMainBaseLayoutDescriptorValue.NaturalEntranceArmyRallyAnchorPoint;
    OutMainBaseLayoutDescriptorValue.ProductionClearanceAnchorPoint =
        LayoutAnchorPointValue + (MainBaseDepthDirectionValue * 8.0f);
    PopulateBelShirAuthoredProductionSlots(StartLocationBucketValue, PlayableMinValue, PlayableMaxValue,
                                          OutMainBaseLayoutDescriptorValue);
    OutMainBaseLayoutDescriptorValue.bIsValid = true;
    return true;
}

}  // namespace sc2
