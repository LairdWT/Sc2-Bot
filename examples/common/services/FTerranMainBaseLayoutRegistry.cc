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

Point2D GetSpawnVerticalMainBaseStep(const EStartLocationBucket StartLocationBucketValue)
{
    switch (StartLocationBucketValue)
    {
        case EStartLocationBucket::UpperLeft:
            return Point2D(0.0f, 4.0f);
        case EStartLocationBucket::LowerRight:
            return Point2D(0.0f, -4.0f);
        case EStartLocationBucket::UpperRight:
            return Point2D(0.0f, 4.0f);
        case EStartLocationBucket::LowerLeft:
            return Point2D(0.0f, -4.0f);
        case EStartLocationBucket::Unknown:
        default:
            return Point2D(0.0f, 0.0f);
    }
}

Point2D GetSpawnHorizontalMainBaseStep(const EStartLocationBucket StartLocationBucketValue)
{
    switch (StartLocationBucketValue)
    {
        case EStartLocationBucket::UpperLeft:
            return Point2D(-6.0f, 0.0f);
        case EStartLocationBucket::LowerRight:
            return Point2D(6.0f, 0.0f);
        case EStartLocationBucket::UpperRight:
            return Point2D(6.0f, 0.0f);
        case EStartLocationBucket::LowerLeft:
            return Point2D(-6.0f, 0.0f);
        case EStartLocationBucket::Unknown:
        default:
            return Point2D(0.0f, 0.0f);
    }
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
    if (StartLocationBucketValue != EStartLocationBucket::UpperLeft &&
        StartLocationBucketValue != EStartLocationBucket::LowerRight)
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
    const Point2D WallBarracksBuildPointValue =
        BuildPlacementContextValue.RampWallDescriptor.BarracksSlot.BuildPoint;

    const Point2D VerticalMainBaseStepValue =
        GetSpawnVerticalMainBaseStep(StartLocationBucketValue);
    const Point2D HorizontalMainBaseStepValue =
        GetSpawnHorizontalMainBaseStep(StartLocationBucketValue);
    const Point2D ProductionRailBarracksBuildPointValue =
        WallBarracksBuildPointValue + HorizontalMainBaseStepValue;
    const Point2D ProductionRailFactoryBuildPointValue =
        ProductionRailBarracksBuildPointValue + HorizontalMainBaseStepValue;
    const Point2D ProductionRailStarportBuildPointValue =
        ProductionRailFactoryBuildPointValue + HorizontalMainBaseStepValue;
    const Point2D OpeningFactoryBuildPointValue =
        WallBarracksBuildPointValue + VerticalMainBaseStepValue;
    const Point2D OpeningStarportBuildPointValue =
        OpeningFactoryBuildPointValue + VerticalMainBaseStepValue;

    OutMainBaseLayoutDescriptorValue.LayoutAnchorPoint = LayoutAnchorPointValue;
    OutMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        ProductionRailBarracksBuildPointValue,
        0U));
    OutMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        ProductionRailFactoryBuildPointValue,
        1U));
    OutMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        ProductionRailStarportBuildPointValue,
        2U));
    OutMainBaseLayoutDescriptorValue.BarracksWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainBarracksWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        ProductionRailBarracksBuildPointValue,
        0U));
    OutMainBaseLayoutDescriptorValue.FactoryWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainFactoryWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        OpeningFactoryBuildPointValue,
        0U));
    OutMainBaseLayoutDescriptorValue.StarportWithAddonSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::MainStarportWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        OpeningStarportBuildPointValue,
        0U));
    OutMainBaseLayoutDescriptorValue.bIsValid = true;
    return true;
}

}  // namespace sc2
