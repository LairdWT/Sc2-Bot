#include "common/services/FTerranBuildPlacementService.h"

#include <algorithm>
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

bool IsTileInsidePlayableArea(const GameInfo& GameInfoValue, const Point2DI& TilePointValue)
{
    return TilePointValue.x >= static_cast<int>(std::floor(GameInfoValue.playable_min.x)) &&
           TilePointValue.x <= static_cast<int>(std::ceil(GameInfoValue.playable_max.x)) &&
           TilePointValue.y >= static_cast<int>(std::floor(GameInfoValue.playable_min.y)) &&
           TilePointValue.y <= static_cast<int>(std::ceil(GameInfoValue.playable_max.y)) &&
           TilePointValue.x >= 0 && TilePointValue.x < GameInfoValue.width &&
           TilePointValue.y >= 0 && TilePointValue.y < GameInfoValue.height;
}

size_t GetTileLinearIndex(const GameInfo& GameInfoValue, const Point2DI& TilePointValue)
{
    return static_cast<size_t>(TilePointValue.x + (TilePointValue.y * GameInfoValue.width));
}

Point2D GetTileCenterPoint(const Point2DI& TilePointValue)
{
    return Point2D(static_cast<float>(TilePointValue.x) + 0.5f, static_cast<float>(TilePointValue.y) + 0.5f);
}

bool DoesTileNeighborhoodContainHeightChange(const GameInfo& GameInfoValue, const HeightMap& HeightMapValue,
                                             const Point2DI& TilePointValue)
{
    float MinimumHeightValue = HeightMapValue.TerrainHeight(TilePointValue);
    float MaximumHeightValue = MinimumHeightValue;

    for (int YOffsetValue = -1; YOffsetValue <= 1; ++YOffsetValue)
    {
        for (int XOffsetValue = -1; XOffsetValue <= 1; ++XOffsetValue)
        {
            const Point2DI NeighborPointValue(TilePointValue.x + XOffsetValue, TilePointValue.y + YOffsetValue);
            if (!IsTileInsidePlayableArea(GameInfoValue, NeighborPointValue))
            {
                continue;
            }

            const float NeighborHeightValue = HeightMapValue.TerrainHeight(NeighborPointValue);
            MinimumHeightValue = std::min(MinimumHeightValue, NeighborHeightValue);
            MaximumHeightValue = std::max(MaximumHeightValue, NeighborHeightValue);
        }
    }

    constexpr float HeightDifferenceToleranceValue = 0.05f;
    return (MaximumHeightValue - MinimumHeightValue) > HeightDifferenceToleranceValue;
}

bool IsRampCandidateTile(const GameInfo& GameInfoValue, const PathingGrid& PathingGridValue,
                         const PlacementGrid& PlacementGridValue, const HeightMap& HeightMapValue,
                         const Point2DI& TilePointValue)
{
    return IsTileInsidePlayableArea(GameInfoValue, TilePointValue) &&
           PathingGridValue.IsPathable(TilePointValue) &&
           !PlacementGridValue.IsPlacable(TilePointValue) &&
           DoesTileNeighborhoodContainHeightChange(GameInfoValue, HeightMapValue, TilePointValue);
}

std::vector<std::vector<Point2DI>> DiscoverRampTileGroups(const GameInfo& GameInfoValue,
                                                          const PathingGrid& PathingGridValue,
                                                          const PlacementGrid& PlacementGridValue,
                                                          const HeightMap& HeightMapValue)
{
    std::vector<std::vector<Point2DI>> RampTileGroupsValue;
    std::vector<uint8_t> VisitedTileValues(static_cast<size_t>(GameInfoValue.width * GameInfoValue.height), 0U);

    for (int TileYValue = 0; TileYValue < GameInfoValue.height; ++TileYValue)
    {
        for (int TileXValue = 0; TileXValue < GameInfoValue.width; ++TileXValue)
        {
            const Point2DI StartTilePointValue(TileXValue, TileYValue);
            if (!IsRampCandidateTile(GameInfoValue, PathingGridValue, PlacementGridValue, HeightMapValue,
                                     StartTilePointValue))
            {
                continue;
            }

            const size_t StartTileIndexValue = GetTileLinearIndex(GameInfoValue, StartTilePointValue);
            if (VisitedTileValues[StartTileIndexValue] != 0U)
            {
                continue;
            }

            std::vector<Point2DI> PendingTilePointsValue;
            std::vector<Point2DI> RampTileGroupValue;
            PendingTilePointsValue.push_back(StartTilePointValue);
            VisitedTileValues[StartTileIndexValue] = 1U;

            for (size_t PendingTileIndexValue = 0U; PendingTileIndexValue < PendingTilePointsValue.size();
                 ++PendingTileIndexValue)
            {
                const Point2DI CurrentTilePointValue = PendingTilePointsValue[PendingTileIndexValue];
                RampTileGroupValue.push_back(CurrentTilePointValue);

                for (int YOffsetValue = -1; YOffsetValue <= 1; ++YOffsetValue)
                {
                    for (int XOffsetValue = -1; XOffsetValue <= 1; ++XOffsetValue)
                    {
                        if (XOffsetValue == 0 && YOffsetValue == 0)
                        {
                            continue;
                        }

                        const Point2DI NeighborTilePointValue(CurrentTilePointValue.x + XOffsetValue,
                                                              CurrentTilePointValue.y + YOffsetValue);
                        if (!IsRampCandidateTile(GameInfoValue, PathingGridValue, PlacementGridValue, HeightMapValue,
                                                 NeighborTilePointValue))
                        {
                            continue;
                        }

                        const size_t NeighborTileIndexValue =
                            GetTileLinearIndex(GameInfoValue, NeighborTilePointValue);
                        if (VisitedTileValues[NeighborTileIndexValue] != 0U)
                        {
                            continue;
                        }

                        VisitedTileValues[NeighborTileIndexValue] = 1U;
                        PendingTilePointsValue.push_back(NeighborTilePointValue);
                    }
                }
            }

            constexpr size_t MinimumRampTileCountValue = 8U;
            if (RampTileGroupValue.size() >= MinimumRampTileCountValue)
            {
                RampTileGroupsValue.push_back(RampTileGroupValue);
            }
        }
    }

    return RampTileGroupsValue;
}

void CollectRampUpperAndLowerTiles(const std::vector<Point2DI>& RampTileGroupValue, const HeightMap& HeightMapValue,
                                   std::vector<Point2DI>& OutUpperTilePointsValue,
                                   std::vector<Point2DI>& OutLowerTilePointsValue)
{
    OutUpperTilePointsValue.clear();
    OutLowerTilePointsValue.clear();
    if (RampTileGroupValue.empty())
    {
        return;
    }

    float MinimumHeightValue = HeightMapValue.TerrainHeight(RampTileGroupValue.front());
    float MaximumHeightValue = MinimumHeightValue;

    for (const Point2DI& RampTilePointValue : RampTileGroupValue)
    {
        const float TileHeightValue = HeightMapValue.TerrainHeight(RampTilePointValue);
        MinimumHeightValue = std::min(MinimumHeightValue, TileHeightValue);
        MaximumHeightValue = std::max(MaximumHeightValue, TileHeightValue);
    }

    constexpr float HeightEqualityToleranceValue = 0.05f;
    for (const Point2DI& RampTilePointValue : RampTileGroupValue)
    {
        const float TileHeightValue = HeightMapValue.TerrainHeight(RampTilePointValue);
        if (std::fabs(TileHeightValue - MaximumHeightValue) <= HeightEqualityToleranceValue)
        {
            OutUpperTilePointsValue.push_back(RampTilePointValue);
        }

        if (std::fabs(TileHeightValue - MinimumHeightValue) <= HeightEqualityToleranceValue)
        {
            OutLowerTilePointsValue.push_back(RampTilePointValue);
        }
    }
}

Point2D GetAverageTileCenterPoint(const std::vector<Point2DI>& TilePointsValue)
{
    if (TilePointsValue.empty())
    {
        return Point2D();
    }

    float SumXValue = 0.0f;
    float SumYValue = 0.0f;
    for (const Point2DI& TilePointValue : TilePointsValue)
    {
        const Point2D TileCenterPointValue = GetTileCenterPoint(TilePointValue);
        SumXValue += TileCenterPointValue.x;
        SumYValue += TileCenterPointValue.y;
    }

    const float PointCountValue = static_cast<float>(TilePointsValue.size());
    return Point2D(SumXValue / PointCountValue, SumYValue / PointCountValue);
}

bool TryGetRampUpperWallPoints(const std::vector<Point2DI>& UpperTilePointsValue,
                               const Point2D& LowerRampCenterPointValue, Point2D& OutUpperWallPointOneValue,
                               Point2D& OutUpperWallPointTwoValue)
{
    if (UpperTilePointsValue.size() < 2U)
    {
        return false;
    }

    std::vector<Point2DI> SortedUpperTilePointsValue = UpperTilePointsValue;
    std::sort(SortedUpperTilePointsValue.begin(), SortedUpperTilePointsValue.end(),
              [&LowerRampCenterPointValue](const Point2DI& LeftTilePointValue,
                                           const Point2DI& RightTilePointValue)
              {
                  return DistanceSquared2D(GetTileCenterPoint(LeftTilePointValue), LowerRampCenterPointValue) >
                         DistanceSquared2D(GetTileCenterPoint(RightTilePointValue), LowerRampCenterPointValue);
              });

    OutUpperWallPointOneValue = GetTileCenterPoint(SortedUpperTilePointsValue[0]);
    OutUpperWallPointTwoValue = GetTileCenterPoint(SortedUpperTilePointsValue[1]);
    return true;
}

bool TryGetCircleIntersectionPoints(const Point2D& CircleCenterPointOneValue,
                                    const Point2D& CircleCenterPointTwoValue, const float RadiusValue,
                                    Point2D& OutIntersectionPointOneValue,
                                    Point2D& OutIntersectionPointTwoValue)
{
    const float CenterDistanceSquaredValue =
        DistanceSquared2D(CircleCenterPointOneValue, CircleCenterPointTwoValue);
    if (CenterDistanceSquaredValue <= 0.0001f)
    {
        return false;
    }

    const float CenterDistanceValue = std::sqrt(CenterDistanceSquaredValue);
    const float MaximumIntersectionDistanceValue = RadiusValue * 2.0f;
    if (CenterDistanceValue > MaximumIntersectionDistanceValue + 0.001f)
    {
        return false;
    }

    const Point2D MidPointValue = (CircleCenterPointOneValue + CircleCenterPointTwoValue) / 2.0f;
    const float HalfCenterDistanceValue = CenterDistanceValue * 0.5f;
    const float OffsetDistanceSquaredValue =
        (RadiusValue * RadiusValue) - (HalfCenterDistanceValue * HalfCenterDistanceValue);
    if (OffsetDistanceSquaredValue < -0.001f)
    {
        return false;
    }

    const float OffsetDistanceValue = std::sqrt(std::max(0.0f, OffsetDistanceSquaredValue));
    const Point2D NormalizedCenterDirectionValue =
        GetNormalizedDirection(CircleCenterPointTwoValue - CircleCenterPointOneValue);
    const Point2D PerpendicularDirectionValue(-NormalizedCenterDirectionValue.y, NormalizedCenterDirectionValue.x);
    OutIntersectionPointOneValue = MidPointValue + (PerpendicularDirectionValue * OffsetDistanceValue);
    OutIntersectionPointTwoValue = MidPointValue - (PerpendicularDirectionValue * OffsetDistanceValue);
    return true;
}

Point2D SelectIntersectionFarthestFromReference(const Point2D& IntersectionPointOneValue,
                                                const Point2D& IntersectionPointTwoValue,
                                                const Point2D& ReferencePointValue)
{
    return DistanceSquared2D(IntersectionPointOneValue, ReferencePointValue) >=
                   DistanceSquared2D(IntersectionPointTwoValue, ReferencePointValue)
               ? IntersectionPointOneValue
               : IntersectionPointTwoValue;
}

bool IsPlacementCandidateValid(const FFrameContext& FrameValue, const ABILITY_ID StructureAbilityIdValue,
                               const EBuildPlacementFootprintPolicy BuildPlacementFootprintPolicyValue,
                               const Point2D& CandidatePointValue)
{
    if (FrameValue.GameInfo == nullptr)
    {
        return false;
    }

    const Point2D ClampedCandidatePointValue = ClampToPlayable(*FrameValue.GameInfo, CandidatePointValue);
    if (!DoesPointSatisfyFootprintPolicy(FrameValue, ClampedCandidatePointValue, StructureAbilityIdValue,
                                         BuildPlacementFootprintPolicyValue))
    {
        return false;
    }

    if (FrameValue.Query != nullptr &&
        !FrameValue.Query->Placement(StructureAbilityIdValue, ClampedCandidatePointValue))
    {
        return false;
    }

    return true;
}

bool DoesRampGroupMatchSelectionPass(const size_t UpperTileCountValue, const size_t SelectionPassIndexValue)
{
    switch (SelectionPassIndexValue)
    {
        case 0U:
            return UpperTileCountValue == 2U || UpperTileCountValue == 5U;
        case 1U:
            return UpperTileCountValue == 4U || UpperTileCountValue == 9U;
        case 2U:
            return UpperTileCountValue >= 2U;
        default:
            return false;
    }
}

float GetRampGroupSelectionScore(const FBuildPlacementContext& BuildPlacementContextValue,
                                 const Point2D& UpperRampCenterPointValue,
                                 const Point2D& LowerRampCenterPointValue)
{
    const Point2D NaturalDirectionValue = GetForwardDirection(BuildPlacementContextValue);
    const Point2D RampDirectionValue =
        GetNormalizedDirection(LowerRampCenterPointValue - UpperRampCenterPointValue);
    const float AlignmentValue = Dot2D(RampDirectionValue, NaturalDirectionValue);
    constexpr float MinimumAlignmentValue = 0.2f;
    if (AlignmentValue < MinimumAlignmentValue)
    {
        return std::numeric_limits<float>::max();
    }

    const float BaseDistanceSquaredValue =
        DistanceSquared2D(UpperRampCenterPointValue, BuildPlacementContextValue.BaseLocation);
    const float NaturalDistanceSquaredValue =
        DistanceSquared2D(LowerRampCenterPointValue, BuildPlacementContextValue.NaturalLocation);
    return BaseDistanceSquaredValue + (NaturalDistanceSquaredValue * 0.1f) +
           ((1.0f - AlignmentValue) * 25.0f);
}

bool TrySelectMainRampTileGroup(const FBuildPlacementContext& BuildPlacementContextValue,
                                const std::vector<std::vector<Point2DI>>& RampTileGroupsValue,
                                const HeightMap& HeightMapValue,
                                std::vector<Point2DI>& OutSelectedRampTileGroupValue)
{
    float BestSelectionScoreValue = std::numeric_limits<float>::max();
    bool bFoundRampGroupValue = false;

    for (size_t SelectionPassIndexValue = 0U; SelectionPassIndexValue < 3U; ++SelectionPassIndexValue)
    {
        BestSelectionScoreValue = std::numeric_limits<float>::max();
        bFoundRampGroupValue = false;

        for (const std::vector<Point2DI>& RampTileGroupValue : RampTileGroupsValue)
        {
            std::vector<Point2DI> UpperTilePointsValue;
            std::vector<Point2DI> LowerTilePointsValue;
            CollectRampUpperAndLowerTiles(RampTileGroupValue, HeightMapValue, UpperTilePointsValue,
                                          LowerTilePointsValue);
            if (UpperTilePointsValue.empty() || LowerTilePointsValue.empty() ||
                !DoesRampGroupMatchSelectionPass(UpperTilePointsValue.size(), SelectionPassIndexValue))
            {
                continue;
            }

            const Point2D UpperRampCenterPointValue = GetAverageTileCenterPoint(UpperTilePointsValue);
            const Point2D LowerRampCenterPointValue = GetAverageTileCenterPoint(LowerTilePointsValue);
            const float SelectionScoreValue =
                GetRampGroupSelectionScore(BuildPlacementContextValue, UpperRampCenterPointValue,
                                           LowerRampCenterPointValue);
            if (SelectionScoreValue >= BestSelectionScoreValue)
            {
                continue;
            }

            BestSelectionScoreValue = SelectionScoreValue;
            OutSelectedRampTileGroupValue = RampTileGroupValue;
            bFoundRampGroupValue = true;
        }

        if (bFoundRampGroupValue)
        {
            return true;
        }
    }

    return false;
}

bool TryCreateRampWallCandidatePointsFromTileGroup(const std::vector<Point2DI>& RampTileGroupValue,
                                                   const HeightMap& HeightMapValue,
                                                   Point2D& OutWallCenterPointValue,
                                                   Point2D& OutInsideStagingPointValue,
                                                   Point2D& OutOutsideStagingPointValue,
                                                   Point2D& OutLeftDepotBuildPointValue,
                                                   Point2D& OutBarracksBuildPointValue,
                                                   Point2D& OutRightDepotBuildPointValue)
{
    std::vector<Point2DI> UpperTilePointsValue;
    std::vector<Point2DI> LowerTilePointsValue;
    CollectRampUpperAndLowerTiles(RampTileGroupValue, HeightMapValue, UpperTilePointsValue,
                                  LowerTilePointsValue);
    if (UpperTilePointsValue.size() < 2U || LowerTilePointsValue.empty())
    {
        return false;
    }

    const Point2D UpperRampCenterPointValue = GetAverageTileCenterPoint(UpperTilePointsValue);
    const Point2D LowerRampCenterPointValue = GetAverageTileCenterPoint(LowerTilePointsValue);

    Point2D UpperWallPointOneValue;
    Point2D UpperWallPointTwoValue;
    if (!TryGetRampUpperWallPoints(UpperTilePointsValue, LowerRampCenterPointValue, UpperWallPointOneValue,
                                   UpperWallPointTwoValue))
    {
        return false;
    }

    Point2D DepotIntersectionPointOneValue;
    Point2D DepotIntersectionPointTwoValue;
    if (!TryGetCircleIntersectionPoints(UpperWallPointOneValue, UpperWallPointTwoValue,
                                        std::sqrt(2.5f), DepotIntersectionPointOneValue,
                                        DepotIntersectionPointTwoValue))
    {
        return false;
    }

    const Point2D DepotMiddlePointValue = SelectIntersectionFarthestFromReference(
        DepotIntersectionPointOneValue, DepotIntersectionPointTwoValue, LowerRampCenterPointValue);

    Point2D BarracksIntersectionPointOneValue;
    Point2D BarracksIntersectionPointTwoValue;
    if (!TryGetCircleIntersectionPoints(UpperWallPointOneValue, UpperWallPointTwoValue,
                                        std::sqrt(5.0f), BarracksIntersectionPointOneValue,
                                        BarracksIntersectionPointTwoValue))
    {
        return false;
    }

    Point2D BarracksBuildPointValue = SelectIntersectionFarthestFromReference(
        BarracksIntersectionPointOneValue, BarracksIntersectionPointTwoValue, LowerRampCenterPointValue);

    const Point2D RampUpperMidPointValue = (UpperWallPointOneValue + UpperWallPointTwoValue) / 2.0f;
    Point2D CornerDepotPointOneValue;
    Point2D CornerDepotPointTwoValue;
    if (!TryGetCircleIntersectionPoints(RampUpperMidPointValue, DepotMiddlePointValue, std::sqrt(5.0f),
                                        CornerDepotPointOneValue, CornerDepotPointTwoValue))
    {
        return false;
    }

    const float MaximumDepotXValue = std::max(CornerDepotPointOneValue.x, CornerDepotPointTwoValue.x);
    if ((BarracksBuildPointValue.x + 1.0f) <= MaximumDepotXValue)
    {
        BarracksBuildPointValue = BarracksBuildPointValue + Point2D(-2.0f, 0.0f);
    }

    const Point2D RampDirectionValue =
        GetNormalizedDirection(LowerRampCenterPointValue - UpperRampCenterPointValue);
    const Point2D LateralDirectionValue = GetLateralDirection(RampDirectionValue);
    const float CornerDepotOneLateralValue =
        Dot2D(CornerDepotPointOneValue - DepotMiddlePointValue, LateralDirectionValue);
    const float CornerDepotTwoLateralValue =
        Dot2D(CornerDepotPointTwoValue - DepotMiddlePointValue, LateralDirectionValue);
    if (CornerDepotOneLateralValue >= CornerDepotTwoLateralValue)
    {
        OutLeftDepotBuildPointValue = CornerDepotPointOneValue;
        OutRightDepotBuildPointValue = CornerDepotPointTwoValue;
    }
    else
    {
        OutLeftDepotBuildPointValue = CornerDepotPointTwoValue;
        OutRightDepotBuildPointValue = CornerDepotPointOneValue;
    }

    OutWallCenterPointValue = DepotMiddlePointValue;
    OutInsideStagingPointValue = DepotMiddlePointValue - (RampDirectionValue * 4.0f);
    OutOutsideStagingPointValue = DepotMiddlePointValue + (RampDirectionValue * 6.0f);
    OutBarracksBuildPointValue = BarracksBuildPointValue;
    return true;
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
    FRampWallDescriptor InvalidRampWallDescriptorValue;
    const Point2D ForwardDirectionValue = GetForwardDirection(BuildPlacementContextValue);
    const Point2D FallbackRampCenterPointValue =
        BuildPlacementContextValue.BaseLocation + (ForwardDirectionValue * 8.0f);
    InvalidRampWallDescriptorValue.WallCenterPoint = FallbackRampCenterPointValue;
    InvalidRampWallDescriptorValue.InsideStagingPoint =
        FallbackRampCenterPointValue - (ForwardDirectionValue * 4.0f);
    InvalidRampWallDescriptorValue.OutsideStagingPoint =
        FallbackRampCenterPointValue + (ForwardDirectionValue * 6.0f);

    if (FrameValue.GameInfo == nullptr)
    {
        return InvalidRampWallDescriptorValue;
    }

    const PathingGrid PathingGridValue(*FrameValue.GameInfo);
    const PlacementGrid PlacementGridValue(*FrameValue.GameInfo);
    const HeightMap HeightMapValue(*FrameValue.GameInfo);
    const std::vector<std::vector<Point2DI>> RampTileGroupsValue =
        DiscoverRampTileGroups(*FrameValue.GameInfo, PathingGridValue, PlacementGridValue, HeightMapValue);

    std::vector<Point2DI> SelectedRampTileGroupValue;
    if (!TrySelectMainRampTileGroup(BuildPlacementContextValue, RampTileGroupsValue, HeightMapValue,
                                    SelectedRampTileGroupValue))
    {
        return InvalidRampWallDescriptorValue;
    }

    Point2D WallCenterPointValue;
    Point2D InsideStagingPointValue;
    Point2D OutsideStagingPointValue;
    Point2D LeftDepotBuildPointValue;
    Point2D BarracksBuildPointValue;
    Point2D RightDepotBuildPointValue;
    if (!TryCreateRampWallCandidatePointsFromTileGroup(SelectedRampTileGroupValue, HeightMapValue,
                                                       WallCenterPointValue, InsideStagingPointValue,
                                                       OutsideStagingPointValue, LeftDepotBuildPointValue,
                                                       BarracksBuildPointValue, RightDepotBuildPointValue))
    {
        return InvalidRampWallDescriptorValue;
    }

    InvalidRampWallDescriptorValue.WallCenterPoint = WallCenterPointValue;
    InvalidRampWallDescriptorValue.InsideStagingPoint = InsideStagingPointValue;
    InvalidRampWallDescriptorValue.OutsideStagingPoint = OutsideStagingPointValue;
    if (!IsPlacementCandidateValid(FrameValue, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                   EBuildPlacementFootprintPolicy::StructureOnly,
                                   LeftDepotBuildPointValue) ||
        !IsPlacementCandidateValid(FrameValue, ABILITY_ID::BUILD_BARRACKS,
                                   EBuildPlacementFootprintPolicy::RequiresAddonClearance,
                                   BarracksBuildPointValue) ||
        !IsPlacementCandidateValid(FrameValue, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                   EBuildPlacementFootprintPolicy::StructureOnly,
                                   RightDepotBuildPointValue))
    {
        return InvalidRampWallDescriptorValue;
    }

    FRampWallDescriptor RampWallDescriptorValue;
    RampWallDescriptorValue.bIsValid = true;
    RampWallDescriptorValue.WallCenterPoint = WallCenterPointValue;
    RampWallDescriptorValue.InsideStagingPoint = InsideStagingPointValue;
    RampWallDescriptorValue.OutsideStagingPoint = OutsideStagingPointValue;
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

    if (FrameValue.GameInfo == nullptr)
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
