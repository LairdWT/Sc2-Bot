#include "common/services/FTerranBuildPlacementService.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "common/terran_models.h"
#include "common/services/FTerranMainBaseLayoutRegistry.h"
#include "sc2api/sc2_map_info.h"
#include "sc2api/sc2_unit_filters.h"

namespace sc2
{
namespace
{

enum class EPlacementCandidateValidationMode : uint8_t
{
    StaticLayout,
    RuntimeQuery,
};

struct FPlacementOffsetDescriptor
{
    float LateralOffset;
    float ForwardOffset;
};

struct FResolvedLayoutPlacementSlot
{
    FBuildPlacementSlot BuildPlacementSlot;
    ABILITY_ID StructureAbilityId;
};

bool DoesStructureAbilityRequireAddonClearance(const ABILITY_ID StructureAbilityIdValue);
Point2D GetAddonFootprintCenter(const Point2D& StructureBuildPointValue);
bool DoesAddonFootprintSupportTerrain(const GameInfo& GameInfoValue, const Point2D& StructureBuildPointValue);
bool IsPlacementCandidateValid(const FFrameContext& FrameValue, const ABILITY_ID StructureAbilityIdValue,
                               const EBuildPlacementFootprintPolicy BuildPlacementFootprintPolicyValue,
                               const Point2D& CandidatePointValue,
                               const EPlacementCandidateValidationMode ValidationModeValue =
                                   EPlacementCandidateValidationMode::RuntimeQuery);
void AppendExactSlotsToLayoutDescriptor(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue,
    const std::vector<FBuildPlacementSlot>& TemplateBuildPlacementSlotsValue, const ABILITY_ID StructureAbilityIdValue,
    std::vector<FBuildPlacementSlot>& OutPlacementSlotsValue,
    std::vector<FResolvedLayoutPlacementSlot>& OutResolvedLayoutPlacementSlotsValue,
    const EPlacementCandidateValidationMode ValidationModeValue =
        EPlacementCandidateValidationMode::RuntimeQuery);
bool TryResolveExactProductionRailSlotGroup(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue,
    const std::vector<FBuildPlacementSlot>& TemplateBuildPlacementSlotsValue,
    const std::vector<FResolvedLayoutPlacementSlot>& SeedResolvedLayoutPlacementSlotsValue,
    std::vector<FBuildPlacementSlot>& OutResolvedBuildPlacementSlotsValue,
    std::vector<FResolvedLayoutPlacementSlot>& OutResolvedLayoutPlacementSlotsValue);
size_t GetProductionRailPivotSlotIndex(
    const std::vector<FBuildPlacementSlot>& TemplateBuildPlacementSlotsValue);
bool TryResolveProductionRailPivotSlot(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue,
    const std::vector<FBuildPlacementSlot>& TemplateBuildPlacementSlotsValue,
    const std::vector<FResolvedLayoutPlacementSlot>& SeedResolvedLayoutPlacementSlotsValue,
    const Point2D& TranslationOffsetValue, const size_t PivotSlotIndexValue,
    std::vector<FBuildPlacementSlot>& OutResolvedBuildPlacementSlotsValue,
    std::vector<FResolvedLayoutPlacementSlot>& OutResolvedLayoutPlacementSlotsValue);
bool RemoveResolvedLayoutPlacementSlotById(
    std::vector<FResolvedLayoutPlacementSlot>& ResolvedLayoutPlacementSlotsValue,
    const FBuildPlacementSlotId& BuildPlacementSlotIdValue);
bool RemovePlacementSlotById(std::vector<FBuildPlacementSlot>& BuildPlacementSlotsValue,
                             const FBuildPlacementSlotId& BuildPlacementSlotIdValue);
bool TryResolveDerivedProductionRailFromBarracksSeed(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue,
    const FBuildPlacementSlot& BarracksAnchorSlotValue,
    const std::vector<FResolvedLayoutPlacementSlot>& SeedResolvedLayoutPlacementSlotsValue,
    std::vector<FBuildPlacementSlot>& OutResolvedBuildPlacementSlotsValue,
    std::vector<FResolvedLayoutPlacementSlot>& OutResolvedLayoutPlacementSlotsValue);
bool TryResolveDerivedProductionRailSlotGroup(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue,
    const std::vector<FBuildPlacementSlot>& BarracksAnchorSlotsValue,
    const std::vector<FResolvedLayoutPlacementSlot>& SeedResolvedLayoutPlacementSlotsValue,
    std::vector<FBuildPlacementSlot>& OutResolvedBuildPlacementSlotsValue,
    std::vector<FResolvedLayoutPlacementSlot>& OutResolvedLayoutPlacementSlotsValue,
    FBuildPlacementSlotId& OutSourceBarracksSlotIdValue);
bool TryResolveAuthoredProductionRailSlotGroup(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue,
    const std::vector<FBuildPlacementSlot>& TemplateBuildPlacementSlotsValue,
    const std::vector<FResolvedLayoutPlacementSlot>& SeedResolvedLayoutPlacementSlotsValue,
    std::vector<FBuildPlacementSlot>& OutResolvedBuildPlacementSlotsValue,
    std::vector<FResolvedLayoutPlacementSlot>& OutResolvedLayoutPlacementSlotsValue,
    size_t& OutSearchOffsetIndexValue);
uint32_t GetAuthoredMainBaseLayoutResolutionScore(
    const FMainBaseLayoutDescriptor& MainBaseLayoutDescriptorValue);
bool TryGetPlacementPlayableBounds(const FFrameContext& FrameValue,
                                   const FBuildPlacementContext& BuildPlacementContextValue,
                                   Point2D& OutPlayableMinValue, Point2D& OutPlayableMaxValue);
Point2D GetSpawnVerticalMainBaseStepForBuildPlacement(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue);
void AppendResolvedPlacementSlotToLayoutDescriptor(
    const FBuildPlacementSlot& ResolvedBuildPlacementSlotValue, const ABILITY_ID StructureAbilityIdValue,
    std::vector<FBuildPlacementSlot>& OutPlacementSlotsValue,
    std::vector<FResolvedLayoutPlacementSlot>& OutResolvedLayoutPlacementSlotsValue);
bool TryResolveChainedPlacementSlotFromReference(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue,
    const Point2D& ReferenceBuildPointValue, const Point2D& RelativeStepValue,
    const EBuildPlacementSlotType BuildPlacementSlotTypeValue, const uint8_t SlotOrdinalValue,
    const ABILITY_ID StructureAbilityIdValue,
    const std::vector<FResolvedLayoutPlacementSlot>& ResolvedLayoutPlacementSlotsValue,
    FBuildPlacementSlot& OutResolvedBuildPlacementSlotValue);

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

Point2D GetClockwiseLateralDirection(const Point2D& ForwardDirectionValue)
{
    return Point2D(ForwardDirectionValue.y, -ForwardDirectionValue.x);
}

Point2D ClampPointToPlayableBounds(const FBuildPlacementContext& BuildPlacementContextValue,
                                   const Point2D& CandidatePointValue)
{
    if (!BuildPlacementContextValue.HasPlayableBounds())
    {
        return CandidatePointValue;
    }

    const float ClampedXValue =
        std::max(BuildPlacementContextValue.PlayableMin.x,
                 std::min(BuildPlacementContextValue.PlayableMax.x, CandidatePointValue.x));
    const float ClampedYValue =
        std::max(BuildPlacementContextValue.PlayableMin.y,
                 std::min(BuildPlacementContextValue.PlayableMax.y, CandidatePointValue.y));
    return Point2D(ClampedXValue, ClampedYValue);
}

FBuildPlacementSlot CreatePlacementSlot(const EBuildPlacementSlotType BuildPlacementSlotTypeValue,
                                        const EBuildPlacementFootprintPolicy BuildPlacementFootprintPolicyValue,
                                        const Point2D& BuildPointValue,
                                        const uint8_t SlotOrdinalValue);

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

    float NaturalEntranceProjectionDistanceValue = 18.0f;
    if (BuildPlacementContextValue.HasNaturalLocation())
    {
        const Point2D OutsideToNaturalVectorValue =
            BuildPlacementContextValue.NaturalLocation - BuildPlacementContextValue.RampWallDescriptor.OutsideStagingPoint;
        const float OutsideToNaturalDistanceSquaredValue =
            (OutsideToNaturalVectorValue.x * OutsideToNaturalVectorValue.x) +
            (OutsideToNaturalVectorValue.y * OutsideToNaturalVectorValue.y);
        const float OutsideToNaturalDistanceValue =
            OutsideToNaturalDistanceSquaredValue > 0.0f ? std::sqrt(OutsideToNaturalDistanceSquaredValue) : 0.0f;
        NaturalEntranceProjectionDistanceValue =
            std::max(6.0f, std::min(18.0f, OutsideToNaturalDistanceValue * 0.6f));
    }

    return ClampPointToPlayableBounds(
        BuildPlacementContextValue,
        BuildPlacementContextValue.RampWallDescriptor.OutsideStagingPoint +
            (NaturalEntranceDirectionValue * NaturalEntranceProjectionDistanceValue));
}

void PopulateNaturalEntranceLayoutDescriptor(const FBuildPlacementContext& BuildPlacementContextValue,
                                             FMainBaseLayoutDescriptor& MainBaseLayoutDescriptorValue)
{
    // Natural entrance wall: Depot-Bunker-Depot front row, Depot behind filling gap.
    // Depot = 2x2, Bunker = 3x3. Lateral offset 3.5 fits depot beside bunker with contact.
    constexpr float NaturalEntranceWallForwardOffsetValue = 4.0f;
    constexpr float NaturalEntranceDepotLateralOffsetValue = 3.5f;
    constexpr float NaturalEntranceBackDepotForwardOffsetValue = -2.5f;

    MainBaseLayoutDescriptorValue.NaturalEntranceWallDepotSlots.clear();
    MainBaseLayoutDescriptorValue.NaturalEntranceBunkerSlots.clear();
    MainBaseLayoutDescriptorValue.NaturalEntranceArmyRallyAnchorPoint = Point2D();
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

    const Point2D NaturalEntranceLateralDirectionValue = GetClockwiseLateralDirection(NaturalEntranceDirectionValue);
    const Point2D NaturalEntranceWallCenterPointValue = ClampPointToPlayableBounds(
        BuildPlacementContextValue,
        BuildPlacementContextValue.RampWallDescriptor.OutsideStagingPoint +
            (NaturalEntranceDirectionValue * NaturalEntranceWallForwardOffsetValue));
    MainBaseLayoutDescriptorValue.NaturalEntranceArmyRallyAnchorPoint =
        ComputeNaturalEntranceAssemblyAnchorPoint(BuildPlacementContextValue);

    // Front row: Left Depot — Bunker (center) — Right Depot
    MainBaseLayoutDescriptorValue.NaturalEntranceWallDepotSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::NaturalEntranceDepotLeft, EBuildPlacementFootprintPolicy::StructureOnly,
        ClampPointToPlayableBounds(
            BuildPlacementContextValue,
            Point2D(NaturalEntranceWallCenterPointValue.x +
                        (NaturalEntranceLateralDirectionValue.x * NaturalEntranceDepotLateralOffsetValue),
                    NaturalEntranceWallCenterPointValue.y +
                        (NaturalEntranceLateralDirectionValue.y * NaturalEntranceDepotLateralOffsetValue))),
        0U));
    MainBaseLayoutDescriptorValue.NaturalEntranceBunkerSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::NaturalEntranceBunker, EBuildPlacementFootprintPolicy::StructureOnly,
        NaturalEntranceWallCenterPointValue,
        0U));
    MainBaseLayoutDescriptorValue.NaturalEntranceWallDepotSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::NaturalEntranceDepotRight, EBuildPlacementFootprintPolicy::StructureOnly,
        ClampPointToPlayableBounds(
            BuildPlacementContextValue,
            Point2D(NaturalEntranceWallCenterPointValue.x -
                        (NaturalEntranceLateralDirectionValue.x * NaturalEntranceDepotLateralOffsetValue),
                    NaturalEntranceWallCenterPointValue.y -
                        (NaturalEntranceLateralDirectionValue.y * NaturalEntranceDepotLateralOffsetValue))),
        0U));

    // Back row: Center Depot behind bunker, filling gap
    MainBaseLayoutDescriptorValue.NaturalEntranceWallDepotSlots.push_back(CreatePlacementSlot(
        EBuildPlacementSlotType::NaturalEntranceDepotCenter, EBuildPlacementFootprintPolicy::StructureOnly,
        ClampPointToPlayableBounds(
            BuildPlacementContextValue,
            Point2D(NaturalEntranceWallCenterPointValue.x +
                        (NaturalEntranceDirectionValue.x * NaturalEntranceBackDepotForwardOffsetValue),
                    NaturalEntranceWallCenterPointValue.y +
                        (NaturalEntranceDirectionValue.y * NaturalEntranceBackDepotForwardOffsetValue))),
        0U));
}

void UpdateMaxForwardProjectionFromPlacementSlots(const std::vector<FBuildPlacementSlot>& BuildPlacementSlotsValue,
                                                  const Point2D& BaseLocationValue,
                                                  const Point2D& ForwardDirectionValue,
                                                  const float ExtraForwardClearanceValue,
                                                  float& MaxForwardProjectionValue)
{
    for (const FBuildPlacementSlot& BuildPlacementSlotValue : BuildPlacementSlotsValue)
    {
        const float ForwardProjectionValue =
            Dot2D(BuildPlacementSlotValue.BuildPoint - BaseLocationValue, ForwardDirectionValue) +
            ExtraForwardClearanceValue;
        if (ForwardProjectionValue > MaxForwardProjectionValue)
        {
            MaxForwardProjectionValue = ForwardProjectionValue;
        }
    }
}

Point2D GetNaturalApproachMidpoint(const FMainBaseLayoutDescriptor& MainBaseLayoutDescriptorValue)
{
    if (MainBaseLayoutDescriptorValue.NaturalApproachDepotSlots.empty())
    {
        return Point2D();
    }

    float SumXValue = 0.0f;
    float SumYValue = 0.0f;
    for (const FBuildPlacementSlot& BuildPlacementSlotValue :
         MainBaseLayoutDescriptorValue.NaturalApproachDepotSlots)
    {
        SumXValue += BuildPlacementSlotValue.BuildPoint.x;
        SumYValue += BuildPlacementSlotValue.BuildPoint.y;
    }

    const float SlotCountValue =
        static_cast<float>(MainBaseLayoutDescriptorValue.NaturalApproachDepotSlots.size());
    return Point2D(SumXValue / SlotCountValue, SumYValue / SlotCountValue);
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

Point2D GetStructureFootprintHalfExtentsForAbility(const ABILITY_ID StructureAbilityIdValue)
{
    switch (StructureAbilityIdValue)
    {
        case ABILITY_ID::BUILD_SUPPLYDEPOT:
            return Point2D(1.0f, 1.0f);
        case ABILITY_ID::BUILD_COMMANDCENTER:
            return Point2D(2.5f, 2.5f);
        case ABILITY_ID::BUILD_BARRACKS:
        case ABILITY_ID::BUILD_FACTORY:
        case ABILITY_ID::BUILD_STARPORT:
        case ABILITY_ID::BUILD_REFINERY:
        default:
            return Point2D(1.5f, 1.5f);
    }
}

bool DoAxisAlignedFootprintsOverlap(const Point2D& CenterPointOneValue, const Point2D& HalfExtentsOneValue,
                                    const Point2D& CenterPointTwoValue, const Point2D& HalfExtentsTwoValue)
{
    constexpr float FootprintTouchToleranceValue = 0.05f;
    return std::fabs(CenterPointOneValue.x - CenterPointTwoValue.x) <
               ((HalfExtentsOneValue.x + HalfExtentsTwoValue.x) - FootprintTouchToleranceValue) &&
           std::fabs(CenterPointOneValue.y - CenterPointTwoValue.y) <
               ((HalfExtentsOneValue.y + HalfExtentsTwoValue.y) - FootprintTouchToleranceValue);
}

bool DoesPlacementSlotOverlapProtectedAddonFootprints(
    const FBuildPlacementSlot& CandidateBuildPlacementSlotValue, const ABILITY_ID StructureAbilityIdValue,
    const std::vector<FBuildPlacementSlot>& ProtectedProducerBuildPlacementSlotsValue)
{
    const Point2D CandidateFootprintHalfExtentsValue =
        GetStructureFootprintHalfExtentsForAbility(StructureAbilityIdValue);
    static const Point2D AddonFootprintHalfExtentsValue(1.0f, 1.0f);

    for (const FBuildPlacementSlot& ProtectedProducerBuildPlacementSlotValue :
         ProtectedProducerBuildPlacementSlotsValue)
    {
        const Point2D ProtectedAddonCenterPointValue =
            GetAddonFootprintCenter(ProtectedProducerBuildPlacementSlotValue.BuildPoint);
        if (DoAxisAlignedFootprintsOverlap(CandidateBuildPlacementSlotValue.BuildPoint,
                                           CandidateFootprintHalfExtentsValue,
                                           ProtectedAddonCenterPointValue,
                                           AddonFootprintHalfExtentsValue))
        {
            return true;
        }
    }

    return false;
}

void AppendPlacementSlotsAvoidingProtectedAddonFootprints(
    const std::vector<FBuildPlacementSlot>& SourceBuildPlacementSlotsValue, const ABILITY_ID StructureAbilityIdValue,
    const std::vector<FBuildPlacementSlot>& ProtectedProducerBuildPlacementSlotsValue,
    std::vector<FBuildPlacementSlot>& OutPlacementSlotsValue)
{
    for (const FBuildPlacementSlot& SourceBuildPlacementSlotValue : SourceBuildPlacementSlotsValue)
    {
        if (DoesPlacementSlotOverlapProtectedAddonFootprints(SourceBuildPlacementSlotValue,
                                                             StructureAbilityIdValue,
                                                             ProtectedProducerBuildPlacementSlotsValue))
        {
            continue;
        }

        OutPlacementSlotsValue.push_back(SourceBuildPlacementSlotValue);
    }
}

bool IsPointOnMainBaseSideOfWall(const FRampWallDescriptor& RampWallDescriptorValue, const Point2D& CandidatePointValue)
{
    if (!RampWallDescriptorValue.bIsValid)
    {
        return true;
    }

    const Point2D InsideDirectionValue =
        GetNormalizedDirection(RampWallDescriptorValue.InsideStagingPoint - RampWallDescriptorValue.WallCenterPoint);
    return Dot2D(CandidatePointValue - RampWallDescriptorValue.WallCenterPoint, InsideDirectionValue) >= 1.0f;
}

bool DoesCandidateRespectBaseClearance(const FBuildPlacementContext& BuildPlacementContextValue,
                                       const ABILITY_ID StructureAbilityIdValue,
                                       const Point2D& CandidatePointValue)
{
    switch (StructureAbilityIdValue)
    {
        case ABILITY_ID::BUILD_BARRACKS:
        case ABILITY_ID::BUILD_FACTORY:
        case ABILITY_ID::BUILD_STARPORT:
        {
            constexpr float MinimumBaseClearanceDistanceSquaredValue = 36.0f;
            return DistanceSquared2D(BuildPlacementContextValue.BaseLocation, CandidatePointValue) >=
                   MinimumBaseClearanceDistanceSquaredValue;
        }
        default:
            return true;
    }
}

bool IsProductionLayoutStructureAbility(const ABILITY_ID StructureAbilityIdValue)
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

bool IsNeutralMainBaseResourceUnit(const FBuildPlacementContext& BuildPlacementContextValue,
                                   const Unit& NeutralUnitValue)
{
    static const IsMineralPatch MineralPatchFilterValue;
    static const IsGeyser GeyserFilterValue;

    if (!MineralPatchFilterValue(NeutralUnitValue) && !GeyserFilterValue(NeutralUnitValue))
    {
        return false;
    }

    constexpr float MaximumMainBaseResourceDistanceSquaredValue = 196.0f;
    const Point2D ResourcePointValue(NeutralUnitValue.pos);
    const float DistanceToBaseSquaredValue =
        DistanceSquared2D(BuildPlacementContextValue.BaseLocation, ResourcePointValue);
    if (DistanceToBaseSquaredValue > MaximumMainBaseResourceDistanceSquaredValue)
    {
        return false;
    }

    if (!BuildPlacementContextValue.HasNaturalLocation())
    {
        return true;
    }

    const float DistanceToNaturalSquaredValue =
        DistanceSquared2D(BuildPlacementContextValue.NaturalLocation, ResourcePointValue);
    return DistanceToBaseSquaredValue < DistanceToNaturalSquaredValue;
}

bool TryGetMainBaseMineralCentroid(const FFrameContext& FrameValue,
                                   const FBuildPlacementContext& BuildPlacementContextValue,
                                   Point2D& OutMineralCentroidValue)
{
    if (FrameValue.Observation == nullptr)
    {
        return false;
    }

    static const IsMineralPatch MineralPatchFilterValue;
    float SumXValue = 0.0f;
    float SumYValue = 0.0f;
    uint32_t MineralCountValue = 0U;

    const Units NeutralUnitsValue = FrameValue.Observation->GetUnits(Unit::Alliance::Neutral);
    for (const Unit* NeutralUnitValue : NeutralUnitsValue)
    {
        if (NeutralUnitValue == nullptr || !MineralPatchFilterValue(*NeutralUnitValue) ||
            !IsNeutralMainBaseResourceUnit(BuildPlacementContextValue, *NeutralUnitValue))
        {
            continue;
        }

        SumXValue += NeutralUnitValue->pos.x;
        SumYValue += NeutralUnitValue->pos.y;
        ++MineralCountValue;
    }

    if (MineralCountValue == 0U)
    {
        return false;
    }

    OutMineralCentroidValue = Point2D(SumXValue / static_cast<float>(MineralCountValue),
                                      SumYValue / static_cast<float>(MineralCountValue));
    return true;
}

Point2D GetMainBaseDepthDirection(const FBuildPlacementContext& BuildPlacementContextValue)
{
    if (BuildPlacementContextValue.RampWallDescriptor.bIsValid)
    {
        return GetNormalizedDirection(BuildPlacementContextValue.RampWallDescriptor.InsideStagingPoint -
                                      BuildPlacementContextValue.RampWallDescriptor.WallCenterPoint);
    }

    return GetNormalizedDirection(BuildPlacementContextValue.BaseLocation -
                                  (BuildPlacementContextValue.HasNaturalLocation()
                                       ? BuildPlacementContextValue.NaturalLocation
                                       : BuildPlacementContextValue.BaseLocation + Point2D(1.0f, 0.0f)));
}

Point2D GetMainBaseLateralDirection(const FFrameContext& FrameValue,
                                    const FBuildPlacementContext& BuildPlacementContextValue,
                                    const Point2D& MainBaseDepthDirectionValue)
{
    Point2D LateralDirectionValue = GetClockwiseLateralDirection(MainBaseDepthDirectionValue);

    Point2D MainBaseMineralCentroidValue;
    if (!TryGetMainBaseMineralCentroid(FrameValue, BuildPlacementContextValue, MainBaseMineralCentroidValue))
    {
        return LateralDirectionValue;
    }

    const float MineralLateralAlignmentValue =
        Dot2D(MainBaseMineralCentroidValue - BuildPlacementContextValue.BaseLocation, LateralDirectionValue);
    if (MineralLateralAlignmentValue <= 0.0f)
    {
        return LateralDirectionValue;
    }

    return Point2D(-LateralDirectionValue.x, -LateralDirectionValue.y);
}

bool DoesCandidateRespectResourceClearance(const FFrameContext& FrameValue,
                                           const FBuildPlacementContext& BuildPlacementContextValue,
                                           const ABILITY_ID StructureAbilityIdValue,
                                           const Point2D& CandidatePointValue)
{
    if (FrameValue.Observation == nullptr || !IsProductionLayoutStructureAbility(StructureAbilityIdValue))
    {
        return true;
    }

    constexpr float MinimumStructureResourceClearanceDistanceSquaredValue = 25.0f;
    constexpr float MinimumAddonResourceClearanceDistanceSquaredValue = 20.25f;
    const bool RequiresAddonClearanceValue = DoesStructureAbilityRequireAddonClearance(StructureAbilityIdValue);
    const Point2D AddonFootprintCenterValue = RequiresAddonClearanceValue
                                                  ? GetAddonFootprintCenter(CandidatePointValue)
                                                  : Point2D();

    const Units NeutralUnitsValue = FrameValue.Observation->GetUnits(Unit::Alliance::Neutral);
    for (const Unit* NeutralUnitValue : NeutralUnitsValue)
    {
        if (NeutralUnitValue == nullptr ||
            !IsNeutralMainBaseResourceUnit(BuildPlacementContextValue, *NeutralUnitValue))
        {
            continue;
        }

        const Point2D ResourcePointValue(NeutralUnitValue->pos);
        if (DistanceSquared2D(CandidatePointValue, ResourcePointValue) <
            MinimumStructureResourceClearanceDistanceSquaredValue)
        {
            return false;
        }

        if (RequiresAddonClearanceValue &&
            DistanceSquared2D(AddonFootprintCenterValue, ResourcePointValue) <
                MinimumAddonResourceClearanceDistanceSquaredValue)
        {
            return false;
        }
    }

    return true;
}

bool DoesCandidateRespectMainBaseClearance(const FFrameContext& FrameValue,
                                           const FBuildPlacementContext& BuildPlacementContextValue,
                                           const ABILITY_ID StructureAbilityIdValue,
                                           const Point2D& CandidatePointValue)
{
    return DoesCandidateRespectBaseClearance(BuildPlacementContextValue, StructureAbilityIdValue,
                                             CandidatePointValue) &&
           DoesCandidateRespectResourceClearance(FrameValue, BuildPlacementContextValue,
                                                StructureAbilityIdValue, CandidatePointValue);
}

bool DoesCandidateAddonFootprintOverlapSlot(const Point2D& CandidateBuildPointValue,
                                            const FResolvedLayoutPlacementSlot& ResolvedLayoutPlacementSlotValue)
{
    const Point2D CandidateAddonCenterValue = GetAddonFootprintCenter(CandidateBuildPointValue);
    const Point2D ExistingAddonCenterValue =
        GetAddonFootprintCenter(ResolvedLayoutPlacementSlotValue.BuildPlacementSlot.BuildPoint);
    const Point2D AddonHalfExtentsValue(1.0f, 1.0f);
    const Point2D ExistingStructureHalfExtentsValue =
        GetStructureFootprintHalfExtentsForAbility(ResolvedLayoutPlacementSlotValue.StructureAbilityId);

    return DoAxisAlignedFootprintsOverlap(CandidateAddonCenterValue, AddonHalfExtentsValue,
                                          ResolvedLayoutPlacementSlotValue.BuildPlacementSlot.BuildPoint,
                                          ExistingStructureHalfExtentsValue) ||
           DoAxisAlignedFootprintsOverlap(CandidateAddonCenterValue, AddonHalfExtentsValue,
                                          ExistingAddonCenterValue, AddonHalfExtentsValue);
}

bool IsProductionLayoutAliasSlotType(const EBuildPlacementSlotType BuildPlacementSlotTypeValue)
{
    switch (BuildPlacementSlotTypeValue)
    {
        case EBuildPlacementSlotType::MainBarracksWithAddon:
        case EBuildPlacementSlotType::MainFactoryWithAddon:
        case EBuildPlacementSlotType::MainStarportWithAddon:
        case EBuildPlacementSlotType::MainProductionWithAddon:
            return true;
        default:
            return false;
    }
}

bool CanAliasResolvedLayoutPlacementSlot(
    const FBuildPlacementSlot& CandidateBuildPlacementSlotValue, const ABILITY_ID StructureAbilityIdValue,
    const FResolvedLayoutPlacementSlot& ResolvedLayoutPlacementSlotValue)
{
    if (StructureAbilityIdValue != ResolvedLayoutPlacementSlotValue.StructureAbilityId)
    {
        return false;
    }

    if (!IsProductionLayoutAliasSlotType(CandidateBuildPlacementSlotValue.SlotId.SlotType) ||
        !IsProductionLayoutAliasSlotType(ResolvedLayoutPlacementSlotValue.BuildPlacementSlot.SlotId.SlotType))
    {
        return false;
    }

    constexpr float AliasBuildPointToleranceSquaredValue = 0.01f;
    return DistanceSquared2D(CandidateBuildPlacementSlotValue.BuildPoint,
                             ResolvedLayoutPlacementSlotValue.BuildPlacementSlot.BuildPoint) <=
           AliasBuildPointToleranceSquaredValue;
}

bool DoesCandidatePlacementSlotConflictWithResolvedLayout(
    const FBuildPlacementSlot& CandidateBuildPlacementSlotValue, const ABILITY_ID StructureAbilityIdValue,
    const std::vector<FResolvedLayoutPlacementSlot>& ResolvedLayoutPlacementSlotsValue)
{
    const Point2D CandidateStructureHalfExtentsValue =
        GetStructureFootprintHalfExtentsForAbility(StructureAbilityIdValue);
    const bool RequiresAddonClearanceValue = DoesStructureAbilityRequireAddonClearance(StructureAbilityIdValue);

    for (const FResolvedLayoutPlacementSlot& ResolvedLayoutPlacementSlotValue : ResolvedLayoutPlacementSlotsValue)
    {
        if (CanAliasResolvedLayoutPlacementSlot(CandidateBuildPlacementSlotValue, StructureAbilityIdValue,
                                                ResolvedLayoutPlacementSlotValue))
        {
            continue;
        }

        const Point2D ExistingStructureHalfExtentsValue =
            GetStructureFootprintHalfExtentsForAbility(ResolvedLayoutPlacementSlotValue.StructureAbilityId);
        if (DoAxisAlignedFootprintsOverlap(CandidateBuildPlacementSlotValue.BuildPoint, CandidateStructureHalfExtentsValue,
                                           ResolvedLayoutPlacementSlotValue.BuildPlacementSlot.BuildPoint,
                                           ExistingStructureHalfExtentsValue))
        {
            return true;
        }

        if (!RequiresAddonClearanceValue)
        {
            continue;
        }

        if (DoesCandidateAddonFootprintOverlapSlot(CandidateBuildPlacementSlotValue.BuildPoint,
                                                   ResolvedLayoutPlacementSlotValue))
        {
            return true;
        }
    }

    return false;
}

const std::array<Point2DI, 25>& GetTemplateSearchOffsets()
{
    static const std::array<Point2DI, 25> TemplateSearchOffsetsValue =
    {{
        Point2DI(0, 0),
        Point2DI(0, 1),
        Point2DI(0, -1),
        Point2DI(1, 0),
        Point2DI(-1, 0),
        Point2DI(1, 1),
        Point2DI(1, -1),
        Point2DI(-1, 1),
        Point2DI(-1, -1),
        Point2DI(0, 2),
        Point2DI(0, -2),
        Point2DI(2, 0),
        Point2DI(-2, 0),
        Point2DI(2, 1),
        Point2DI(2, -1),
        Point2DI(-2, 1),
        Point2DI(-2, -1),
        Point2DI(1, 2),
        Point2DI(1, -2),
        Point2DI(-1, 2),
        Point2DI(-1, -2),
        Point2DI(2, 2),
        Point2DI(2, -2),
        Point2DI(-2, 2),
        Point2DI(-2, -2),
    }};

    return TemplateSearchOffsetsValue;
}

const std::vector<Point2DI>& GetLayoutAnchorSearchOffsets()
{
    static const std::vector<Point2DI> LayoutAnchorSearchOffsetsValue =
        []()
        {
            std::vector<Point2DI> SearchOffsetsValue;
            constexpr int MaximumSearchRadiusValue = 12;
            SearchOffsetsValue.reserve(static_cast<size_t>(((MaximumSearchRadiusValue * 2) + 1) *
                                                           ((MaximumSearchRadiusValue * 2) + 1)));
            SearchOffsetsValue.push_back(Point2DI(0, 0));

            for (int SearchRadiusValue = 1; SearchRadiusValue <= MaximumSearchRadiusValue; ++SearchRadiusValue)
            {
                for (int YOffsetValue = -SearchRadiusValue; YOffsetValue <= SearchRadiusValue; ++YOffsetValue)
                {
                    for (int XOffsetValue = -SearchRadiusValue; XOffsetValue <= SearchRadiusValue; ++XOffsetValue)
                    {
                        if (std::max(std::abs(XOffsetValue), std::abs(YOffsetValue)) != SearchRadiusValue)
                        {
                            continue;
                        }

                        SearchOffsetsValue.push_back(Point2DI(XOffsetValue, YOffsetValue));
                    }
                }
            }

            return SearchOffsetsValue;
        }();

    return LayoutAnchorSearchOffsetsValue;
}

Point2D GetMainBaseLayoutAnchorPoint(const FBuildPlacementContext& BuildPlacementContextValue,
                                     const Point2D& MainBaseDepthDirectionValue)
{
    if (BuildPlacementContextValue.RampWallDescriptor.bIsValid)
    {
        return BuildPlacementContextValue.RampWallDescriptor.WallCenterPoint +
               (MainBaseDepthDirectionValue * 6.0f);
    }

    const Point2D ForwardDirectionValue = GetForwardDirection(BuildPlacementContextValue);
    return BuildPlacementContextValue.BaseLocation + (ForwardDirectionValue * 6.0f) +
           (MainBaseDepthDirectionValue * 2.0f);
}

Point2D MirrorPointAcrossDepthAxis(const Point2D& PointValue, const Point2D& AnchorPointValue,
                                   const Point2D& MainBaseDepthDirectionValue,
                                   const Point2D& MainBaseLateralDirectionValue)
{
    const Point2D AnchorToPointValue = PointValue - AnchorPointValue;
    const float DepthOffsetValue = Dot2D(AnchorToPointValue, MainBaseDepthDirectionValue);
    const float LateralOffsetValue = Dot2D(AnchorToPointValue, MainBaseLateralDirectionValue);
    return AnchorPointValue + (MainBaseDepthDirectionValue * DepthOffsetValue) -
           (MainBaseLateralDirectionValue * LateralOffsetValue);
}

bool TryResolveExactPlacementSlot(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue,
    const FBuildPlacementSlot& TemplateBuildPlacementSlotValue, const ABILITY_ID StructureAbilityIdValue,
    const std::vector<FResolvedLayoutPlacementSlot>& ResolvedLayoutPlacementSlotsValue,
    FBuildPlacementSlot& OutResolvedBuildPlacementSlotValue,
    const EPlacementCandidateValidationMode ValidationModeValue =
        EPlacementCandidateValidationMode::RuntimeQuery)
{
    const Point2D CandidatePointValue =
        FrameValue.GameInfo != nullptr
            ? ClampToPlayable(*FrameValue.GameInfo, TemplateBuildPlacementSlotValue.BuildPoint)
            : TemplateBuildPlacementSlotValue.BuildPoint;
    if (!IsPointOnMainBaseSideOfWall(BuildPlacementContextValue.RampWallDescriptor, CandidatePointValue) ||
        !DoesCandidateRespectMainBaseClearance(FrameValue, BuildPlacementContextValue, StructureAbilityIdValue,
                                               CandidatePointValue))
    {
        return false;
    }

    FBuildPlacementSlot CandidateBuildPlacementSlotValue = TemplateBuildPlacementSlotValue;
    CandidateBuildPlacementSlotValue.BuildPoint = CandidatePointValue;

    if (FrameValue.GameInfo != nullptr &&
        !IsPlacementCandidateValid(FrameValue, StructureAbilityIdValue,
                                   CandidateBuildPlacementSlotValue.FootprintPolicy, CandidatePointValue,
                                   ValidationModeValue))
    {
        return false;
    }

    if (DoesCandidatePlacementSlotConflictWithResolvedLayout(CandidateBuildPlacementSlotValue,
                                                             StructureAbilityIdValue,
                                                             ResolvedLayoutPlacementSlotsValue))
    {
        return false;
    }

    OutResolvedBuildPlacementSlotValue = CandidateBuildPlacementSlotValue;
    return true;
}

bool TryResolveAuthoredPlacementSlot(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue,
    const FBuildPlacementSlot& TemplateBuildPlacementSlotValue, const ABILITY_ID StructureAbilityIdValue,
    const std::vector<FResolvedLayoutPlacementSlot>& ResolvedLayoutPlacementSlotsValue,
    FBuildPlacementSlot& OutResolvedBuildPlacementSlotValue,
    const EPlacementCandidateValidationMode ValidationModeValue =
        EPlacementCandidateValidationMode::RuntimeQuery)
{
    if (TryResolveExactPlacementSlot(FrameValue, BuildPlacementContextValue, TemplateBuildPlacementSlotValue,
                                     StructureAbilityIdValue, ResolvedLayoutPlacementSlotsValue,
                                     OutResolvedBuildPlacementSlotValue, ValidationModeValue))
    {
        return true;
    }

    if (FrameValue.GameInfo == nullptr)
    {
        return false;
    }

    const std::array<Point2DI, 25>& SearchOffsetsValue = GetTemplateSearchOffsets();
    for (size_t SearchOffsetIndexValue = 1U; SearchOffsetIndexValue < SearchOffsetsValue.size();
         ++SearchOffsetIndexValue)
    {
        const Point2DI& SearchOffsetValue = SearchOffsetsValue[SearchOffsetIndexValue];
        FBuildPlacementSlot CandidateBuildPlacementSlotValue = TemplateBuildPlacementSlotValue;
        CandidateBuildPlacementSlotValue.BuildPoint =
            Point2D(TemplateBuildPlacementSlotValue.BuildPoint.x + static_cast<float>(SearchOffsetValue.x),
                    TemplateBuildPlacementSlotValue.BuildPoint.y + static_cast<float>(SearchOffsetValue.y));
        if (TryResolveExactPlacementSlot(FrameValue, BuildPlacementContextValue, CandidateBuildPlacementSlotValue,
                                         StructureAbilityIdValue, ResolvedLayoutPlacementSlotsValue,
                                         OutResolvedBuildPlacementSlotValue, ValidationModeValue))
        {
            return true;
        }
    }

    return false;
}

bool TryResolveTemplatePlacementSlot(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue,
    const FBuildPlacementSlot& TemplateBuildPlacementSlotValue, const ABILITY_ID StructureAbilityIdValue,
    const std::vector<FResolvedLayoutPlacementSlot>& ResolvedLayoutPlacementSlotsValue,
    const bool AllowTemplateSearchValue, FBuildPlacementSlot& OutResolvedBuildPlacementSlotValue)
{
    if (TryResolveExactPlacementSlot(FrameValue, BuildPlacementContextValue, TemplateBuildPlacementSlotValue,
                                     StructureAbilityIdValue, ResolvedLayoutPlacementSlotsValue,
                                     OutResolvedBuildPlacementSlotValue))
    {
        return true;
    }

    if (!AllowTemplateSearchValue || FrameValue.GameInfo == nullptr)
    {
        return false;
    }

    const std::array<Point2DI, 25>& SearchOffsetsValue = GetTemplateSearchOffsets();
    for (size_t SearchOffsetIndexValue = 1U; SearchOffsetIndexValue < SearchOffsetsValue.size();
         ++SearchOffsetIndexValue)
    {
        const Point2DI& SearchOffsetValue = SearchOffsetsValue[SearchOffsetIndexValue];
        FBuildPlacementSlot CandidateBuildPlacementSlotValue = TemplateBuildPlacementSlotValue;
        CandidateBuildPlacementSlotValue.BuildPoint =
            Point2D(TemplateBuildPlacementSlotValue.BuildPoint.x + static_cast<float>(SearchOffsetValue.x),
                    TemplateBuildPlacementSlotValue.BuildPoint.y + static_cast<float>(SearchOffsetValue.y));
        if (TryResolveExactPlacementSlot(FrameValue, BuildPlacementContextValue, CandidateBuildPlacementSlotValue,
                                         StructureAbilityIdValue, ResolvedLayoutPlacementSlotsValue,
                                         OutResolvedBuildPlacementSlotValue))
        {
            return true;
        }
    }

    return false;
}

bool TryGetPlacementPlayableBounds(const FFrameContext& FrameValue,
                                   const FBuildPlacementContext& BuildPlacementContextValue,
                                   Point2D& OutPlayableMinValue, Point2D& OutPlayableMaxValue)
{
    if (FrameValue.GameInfo != nullptr &&
        FrameValue.GameInfo->playable_max.x > FrameValue.GameInfo->playable_min.x &&
        FrameValue.GameInfo->playable_max.y > FrameValue.GameInfo->playable_min.y)
    {
        OutPlayableMinValue = FrameValue.GameInfo->playable_min;
        OutPlayableMaxValue = FrameValue.GameInfo->playable_max;
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

Point2D GetSpawnVerticalMainBaseStepForBuildPlacement(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue)
{
    Point2D PlayableMinValue;
    Point2D PlayableMaxValue;
    if (!TryGetPlacementPlayableBounds(FrameValue, BuildPlacementContextValue, PlayableMinValue,
                                       PlayableMaxValue))
    {
        return Point2D(0.0f, 0.0f);
    }

    const float CenterXValue = (PlayableMinValue.x + PlayableMaxValue.x) * 0.5f;
    const float CenterYValue = (PlayableMinValue.y + PlayableMaxValue.y) * 0.5f;
    const bool IsLeftSideValue = BuildPlacementContextValue.BaseLocation.x <= CenterXValue;
    const bool IsUpperSideValue = BuildPlacementContextValue.BaseLocation.y >= CenterYValue;

    if (IsLeftSideValue && IsUpperSideValue)
    {
        return Point2D(0.0f, 6.0f);
    }

    if (!IsLeftSideValue && !IsUpperSideValue)
    {
        return Point2D(0.0f, -6.0f);
    }

    if (!IsLeftSideValue && IsUpperSideValue)
    {
        return Point2D(0.0f, 6.0f);
    }

    return Point2D(0.0f, -6.0f);
}

void AppendResolvedPlacementSlotToLayoutDescriptor(
    const FBuildPlacementSlot& ResolvedBuildPlacementSlotValue, const ABILITY_ID StructureAbilityIdValue,
    std::vector<FBuildPlacementSlot>& OutPlacementSlotsValue,
    std::vector<FResolvedLayoutPlacementSlot>& OutResolvedLayoutPlacementSlotsValue)
{
    OutPlacementSlotsValue.push_back(ResolvedBuildPlacementSlotValue);

    FResolvedLayoutPlacementSlot ResolvedLayoutPlacementSlotValue;
    ResolvedLayoutPlacementSlotValue.BuildPlacementSlot = ResolvedBuildPlacementSlotValue;
    ResolvedLayoutPlacementSlotValue.StructureAbilityId = StructureAbilityIdValue;
    OutResolvedLayoutPlacementSlotsValue.push_back(ResolvedLayoutPlacementSlotValue);
}

bool TryResolveChainedPlacementSlotFromReference(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue,
    const Point2D& ReferenceBuildPointValue, const Point2D& RelativeStepValue,
    const EBuildPlacementSlotType BuildPlacementSlotTypeValue, const uint8_t SlotOrdinalValue,
    const ABILITY_ID StructureAbilityIdValue,
    const std::vector<FResolvedLayoutPlacementSlot>& ResolvedLayoutPlacementSlotsValue,
    FBuildPlacementSlot& OutResolvedBuildPlacementSlotValue)
{
    const FBuildPlacementSlot TemplateBuildPlacementSlotValue = CreatePlacementSlot(
        BuildPlacementSlotTypeValue,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        ReferenceBuildPointValue + RelativeStepValue,
        SlotOrdinalValue);
    const std::vector<Point2DI>& SearchOffsetsValue = GetLayoutAnchorSearchOffsets();
    const size_t SearchOffsetCountValue =
        FrameValue.GameInfo != nullptr ? SearchOffsetsValue.size() : static_cast<size_t>(1U);
    constexpr int MaximumSearchDistanceValue = 6;

    for (size_t SearchOffsetIndexValue = 0U; SearchOffsetIndexValue < SearchOffsetCountValue;
         ++SearchOffsetIndexValue)
    {
        const Point2DI SearchOffsetValue = SearchOffsetsValue[SearchOffsetIndexValue];
        if (std::max(std::abs(SearchOffsetValue.x), std::abs(SearchOffsetValue.y)) >
            MaximumSearchDistanceValue)
        {
            continue;
        }

        FBuildPlacementSlot CandidateBuildPlacementSlotValue = TemplateBuildPlacementSlotValue;
        CandidateBuildPlacementSlotValue.BuildPoint =
            TemplateBuildPlacementSlotValue.BuildPoint +
            Point2D(static_cast<float>(SearchOffsetValue.x), static_cast<float>(SearchOffsetValue.y));
        if (TryResolveExactPlacementSlot(FrameValue, BuildPlacementContextValue,
                                         CandidateBuildPlacementSlotValue,
                                         StructureAbilityIdValue,
                                         ResolvedLayoutPlacementSlotsValue,
                                         OutResolvedBuildPlacementSlotValue,
                                         EPlacementCandidateValidationMode::RuntimeQuery))
        {
            return true;
        }
    }

    return false;
}

void TranslatePlacementSlots(const std::vector<FBuildPlacementSlot>& SourcePlacementSlotsValue,
                             const Point2D& TranslationOffsetValue,
                             std::vector<FBuildPlacementSlot>& OutPlacementSlotsValue)
{
    OutPlacementSlotsValue.clear();
    OutPlacementSlotsValue.reserve(SourcePlacementSlotsValue.size());

    for (const FBuildPlacementSlot& SourcePlacementSlotValue : SourcePlacementSlotsValue)
    {
        FBuildPlacementSlot TranslatedPlacementSlotValue = SourcePlacementSlotValue;
        TranslatedPlacementSlotValue.BuildPoint =
            SourcePlacementSlotValue.BuildPoint + TranslationOffsetValue;
        OutPlacementSlotsValue.push_back(TranslatedPlacementSlotValue);
    }
}

void MirrorPlacementSlotsAcrossDepthAxis(const std::vector<FBuildPlacementSlot>& SourcePlacementSlotsValue,
                                         const Point2D& AnchorPointValue,
                                         const Point2D& MainBaseDepthDirectionValue,
                                         const Point2D& MainBaseLateralDirectionValue,
                                         std::vector<FBuildPlacementSlot>& OutPlacementSlotsValue)
{
    OutPlacementSlotsValue.clear();
    OutPlacementSlotsValue.reserve(SourcePlacementSlotsValue.size());

    for (const FBuildPlacementSlot& SourcePlacementSlotValue : SourcePlacementSlotsValue)
    {
        FBuildPlacementSlot MirroredPlacementSlotValue = SourcePlacementSlotValue;
        MirroredPlacementSlotValue.BuildPoint = MirrorPointAcrossDepthAxis(
            SourcePlacementSlotValue.BuildPoint, AnchorPointValue, MainBaseDepthDirectionValue,
            MainBaseLateralDirectionValue);
        OutPlacementSlotsValue.push_back(MirroredPlacementSlotValue);
    }
}

FMainBaseLayoutDescriptor CreateMirroredAuthoredMainBaseLayoutDescriptor(
    const FMainBaseLayoutDescriptor& SourceMainBaseLayoutDescriptorValue,
    const Point2D& MainBaseDepthDirectionValue, const Point2D& MainBaseLateralDirectionValue)
{
    FMainBaseLayoutDescriptor MirroredMainBaseLayoutDescriptorValue;
    MirroredMainBaseLayoutDescriptorValue.LayoutAnchorPoint =
        SourceMainBaseLayoutDescriptorValue.LayoutAnchorPoint;
    MirroredMainBaseLayoutDescriptorValue.bUsesAuthoredProductionLayout =
        SourceMainBaseLayoutDescriptorValue.bUsesAuthoredProductionLayout;
    MirroredMainBaseLayoutDescriptorValue.ArmyAssemblyAnchorPoint = MirrorPointAcrossDepthAxis(
        SourceMainBaseLayoutDescriptorValue.ArmyAssemblyAnchorPoint,
        SourceMainBaseLayoutDescriptorValue.LayoutAnchorPoint, MainBaseDepthDirectionValue,
        MainBaseLateralDirectionValue);
    MirroredMainBaseLayoutDescriptorValue.NaturalEntranceArmyRallyAnchorPoint = MirrorPointAcrossDepthAxis(
        SourceMainBaseLayoutDescriptorValue.NaturalEntranceArmyRallyAnchorPoint,
        SourceMainBaseLayoutDescriptorValue.LayoutAnchorPoint, MainBaseDepthDirectionValue,
        MainBaseLateralDirectionValue);
    MirroredMainBaseLayoutDescriptorValue.ProductionClearanceAnchorPoint = MirrorPointAcrossDepthAxis(
        SourceMainBaseLayoutDescriptorValue.ProductionClearanceAnchorPoint,
        SourceMainBaseLayoutDescriptorValue.LayoutAnchorPoint, MainBaseDepthDirectionValue,
        MainBaseLateralDirectionValue);
    MirroredMainBaseLayoutDescriptorValue.bIsValid = SourceMainBaseLayoutDescriptorValue.bIsValid;

    MirrorPlacementSlotsAcrossDepthAxis(SourceMainBaseLayoutDescriptorValue.NaturalEntranceWallDepotSlots,
                                        SourceMainBaseLayoutDescriptorValue.LayoutAnchorPoint,
                                        MainBaseDepthDirectionValue, MainBaseLateralDirectionValue,
                                        MirroredMainBaseLayoutDescriptorValue.NaturalEntranceWallDepotSlots);
    MirrorPlacementSlotsAcrossDepthAxis(SourceMainBaseLayoutDescriptorValue.NaturalApproachDepotSlots,
                                        SourceMainBaseLayoutDescriptorValue.LayoutAnchorPoint,
                                        MainBaseDepthDirectionValue, MainBaseLateralDirectionValue,
                                        MirroredMainBaseLayoutDescriptorValue.NaturalApproachDepotSlots);
    MirrorPlacementSlotsAcrossDepthAxis(SourceMainBaseLayoutDescriptorValue.SupportDepotSlots,
                                        SourceMainBaseLayoutDescriptorValue.LayoutAnchorPoint,
                                        MainBaseDepthDirectionValue, MainBaseLateralDirectionValue,
                                        MirroredMainBaseLayoutDescriptorValue.SupportDepotSlots);
    MirrorPlacementSlotsAcrossDepthAxis(SourceMainBaseLayoutDescriptorValue.PeripheralDepotSlots,
                                        SourceMainBaseLayoutDescriptorValue.LayoutAnchorPoint,
                                        MainBaseDepthDirectionValue, MainBaseLateralDirectionValue,
                                        MirroredMainBaseLayoutDescriptorValue.PeripheralDepotSlots);
    MirrorPlacementSlotsAcrossDepthAxis(SourceMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots,
                                        SourceMainBaseLayoutDescriptorValue.LayoutAnchorPoint,
                                        MainBaseDepthDirectionValue, MainBaseLateralDirectionValue,
                                        MirroredMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots);
    MirrorPlacementSlotsAcrossDepthAxis(SourceMainBaseLayoutDescriptorValue.BarracksWithAddonSlots,
                                        SourceMainBaseLayoutDescriptorValue.LayoutAnchorPoint,
                                        MainBaseDepthDirectionValue, MainBaseLateralDirectionValue,
                                        MirroredMainBaseLayoutDescriptorValue.BarracksWithAddonSlots);
    MirrorPlacementSlotsAcrossDepthAxis(SourceMainBaseLayoutDescriptorValue.FactoryWithAddonSlots,
                                        SourceMainBaseLayoutDescriptorValue.LayoutAnchorPoint,
                                        MainBaseDepthDirectionValue, MainBaseLateralDirectionValue,
                                        MirroredMainBaseLayoutDescriptorValue.FactoryWithAddonSlots);
    MirrorPlacementSlotsAcrossDepthAxis(SourceMainBaseLayoutDescriptorValue.StarportWithAddonSlots,
                                        SourceMainBaseLayoutDescriptorValue.LayoutAnchorPoint,
                                        MainBaseDepthDirectionValue, MainBaseLateralDirectionValue,
                                        MirroredMainBaseLayoutDescriptorValue.StarportWithAddonSlots);
    return MirroredMainBaseLayoutDescriptorValue;
}

void ResolveAuthoredMainBaseLayoutDescriptor(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue,
    const FMainBaseLayoutDescriptor& TemplateMainBaseLayoutDescriptorValue,
    FMainBaseLayoutDescriptor& OutResolvedMainBaseLayoutDescriptorValue,
    std::vector<FResolvedLayoutPlacementSlot>& OutResolvedLayoutPlacementSlotsValue,
    size_t& OutProductionRailSearchOffsetIndexValue)
{
    OutResolvedMainBaseLayoutDescriptorValue.Reset();
    OutResolvedMainBaseLayoutDescriptorValue.LayoutAnchorPoint = TemplateMainBaseLayoutDescriptorValue.LayoutAnchorPoint;
    OutResolvedMainBaseLayoutDescriptorValue.bUsesAuthoredProductionLayout =
        TemplateMainBaseLayoutDescriptorValue.bUsesAuthoredProductionLayout;
    OutResolvedMainBaseLayoutDescriptorValue.ArmyAssemblyAnchorPoint =
        TemplateMainBaseLayoutDescriptorValue.ArmyAssemblyAnchorPoint;
    OutResolvedMainBaseLayoutDescriptorValue.NaturalEntranceArmyRallyAnchorPoint =
        TemplateMainBaseLayoutDescriptorValue.NaturalEntranceArmyRallyAnchorPoint;
    OutResolvedMainBaseLayoutDescriptorValue.ProductionClearanceAnchorPoint =
        TemplateMainBaseLayoutDescriptorValue.ProductionClearanceAnchorPoint;
    OutProductionRailSearchOffsetIndexValue = std::numeric_limits<size_t>::max();

    AppendExactSlotsToLayoutDescriptor(FrameValue, BuildPlacementContextValue,
                                       TemplateMainBaseLayoutDescriptorValue.NaturalEntranceWallDepotSlots,
                                       ABILITY_ID::BUILD_SUPPLYDEPOT,
                                       OutResolvedMainBaseLayoutDescriptorValue.NaturalEntranceWallDepotSlots,
                                       OutResolvedLayoutPlacementSlotsValue);
    std::vector<FBuildPlacementSlot> ResolvedProductionRailSlotsValue;
    std::vector<FResolvedLayoutPlacementSlot> ResolvedProductionRailLayoutPlacementSlotsValue;
    size_t ResolvedProductionRailSearchOffsetIndexValue = std::numeric_limits<size_t>::max();
    if (TryResolveAuthoredProductionRailSlotGroup(
            FrameValue, BuildPlacementContextValue,
            TemplateMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots,
            OutResolvedLayoutPlacementSlotsValue, ResolvedProductionRailSlotsValue,
            ResolvedProductionRailLayoutPlacementSlotsValue,
            ResolvedProductionRailSearchOffsetIndexValue))
    {
        OutResolvedMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots =
            ResolvedProductionRailSlotsValue;
        OutResolvedLayoutPlacementSlotsValue = ResolvedProductionRailLayoutPlacementSlotsValue;
        OutProductionRailSearchOffsetIndexValue = ResolvedProductionRailSearchOffsetIndexValue;
    }
    AppendExactSlotsToLayoutDescriptor(FrameValue, BuildPlacementContextValue,
                                       TemplateMainBaseLayoutDescriptorValue.BarracksWithAddonSlots,
                                       ABILITY_ID::BUILD_BARRACKS,
                                       OutResolvedMainBaseLayoutDescriptorValue.BarracksWithAddonSlots,
                                       OutResolvedLayoutPlacementSlotsValue,
                                       EPlacementCandidateValidationMode::StaticLayout);
    AppendExactSlotsToLayoutDescriptor(FrameValue, BuildPlacementContextValue,
                                       TemplateMainBaseLayoutDescriptorValue.FactoryWithAddonSlots,
                                       ABILITY_ID::BUILD_FACTORY,
                                       OutResolvedMainBaseLayoutDescriptorValue.FactoryWithAddonSlots,
                                       OutResolvedLayoutPlacementSlotsValue,
                                       EPlacementCandidateValidationMode::StaticLayout);
    AppendExactSlotsToLayoutDescriptor(FrameValue, BuildPlacementContextValue,
                                       TemplateMainBaseLayoutDescriptorValue.StarportWithAddonSlots,
                                       ABILITY_ID::BUILD_STARPORT,
                                       OutResolvedMainBaseLayoutDescriptorValue.StarportWithAddonSlots,
                                       OutResolvedLayoutPlacementSlotsValue,
                                       EPlacementCandidateValidationMode::StaticLayout);
    AppendExactSlotsToLayoutDescriptor(FrameValue, BuildPlacementContextValue,
                                       TemplateMainBaseLayoutDescriptorValue.NaturalApproachDepotSlots,
                                       ABILITY_ID::BUILD_SUPPLYDEPOT,
                                       OutResolvedMainBaseLayoutDescriptorValue.NaturalApproachDepotSlots,
                                       OutResolvedLayoutPlacementSlotsValue);
    AppendExactSlotsToLayoutDescriptor(FrameValue, BuildPlacementContextValue,
                                       TemplateMainBaseLayoutDescriptorValue.SupportDepotSlots,
                                       ABILITY_ID::BUILD_SUPPLYDEPOT,
                                       OutResolvedMainBaseLayoutDescriptorValue.SupportDepotSlots,
                                       OutResolvedLayoutPlacementSlotsValue);
    AppendExactSlotsToLayoutDescriptor(FrameValue, BuildPlacementContextValue,
                                       TemplateMainBaseLayoutDescriptorValue.PeripheralDepotSlots,
                                       ABILITY_ID::BUILD_SUPPLYDEPOT,
                                       OutResolvedMainBaseLayoutDescriptorValue.PeripheralDepotSlots,
                                       OutResolvedLayoutPlacementSlotsValue);

    const Point2D VerticalMainBaseStepValue =
        GetSpawnVerticalMainBaseStepForBuildPlacement(FrameValue, BuildPlacementContextValue);
    if (OutResolvedMainBaseLayoutDescriptorValue.FactoryWithAddonSlots.empty() &&
        BuildPlacementContextValue.RampWallDescriptor.bIsValid &&
        (VerticalMainBaseStepValue.x != 0.0f || VerticalMainBaseStepValue.y != 0.0f))
    {
        FBuildPlacementSlot ResolvedFactoryBuildPlacementSlotValue;
        if (TryResolveChainedPlacementSlotFromReference(
                FrameValue, BuildPlacementContextValue,
                BuildPlacementContextValue.RampWallDescriptor.BarracksSlot.BuildPoint,
                VerticalMainBaseStepValue,
                EBuildPlacementSlotType::MainFactoryWithAddon, 0U,
                ABILITY_ID::BUILD_FACTORY,
                OutResolvedLayoutPlacementSlotsValue,
                ResolvedFactoryBuildPlacementSlotValue))
        {
            AppendResolvedPlacementSlotToLayoutDescriptor(
                ResolvedFactoryBuildPlacementSlotValue, ABILITY_ID::BUILD_FACTORY,
                OutResolvedMainBaseLayoutDescriptorValue.FactoryWithAddonSlots,
                OutResolvedLayoutPlacementSlotsValue);
        }
    }

    if (OutResolvedMainBaseLayoutDescriptorValue.StarportWithAddonSlots.empty() &&
        !OutResolvedMainBaseLayoutDescriptorValue.FactoryWithAddonSlots.empty() &&
        (VerticalMainBaseStepValue.x != 0.0f || VerticalMainBaseStepValue.y != 0.0f))
    {
        FBuildPlacementSlot ResolvedStarportBuildPlacementSlotValue;
        if (TryResolveChainedPlacementSlotFromReference(
                FrameValue, BuildPlacementContextValue,
                OutResolvedMainBaseLayoutDescriptorValue.FactoryWithAddonSlots.front().BuildPoint,
                VerticalMainBaseStepValue,
                EBuildPlacementSlotType::MainStarportWithAddon, 0U,
                ABILITY_ID::BUILD_STARPORT,
                OutResolvedLayoutPlacementSlotsValue,
                ResolvedStarportBuildPlacementSlotValue))
        {
            AppendResolvedPlacementSlotToLayoutDescriptor(
                ResolvedStarportBuildPlacementSlotValue, ABILITY_ID::BUILD_STARPORT,
                OutResolvedMainBaseLayoutDescriptorValue.StarportWithAddonSlots,
                OutResolvedLayoutPlacementSlotsValue);
        }
    }

    OutResolvedMainBaseLayoutDescriptorValue.bIsValid =
        !OutResolvedMainBaseLayoutDescriptorValue.NaturalEntranceWallDepotSlots.empty() ||
        !OutResolvedMainBaseLayoutDescriptorValue.NaturalApproachDepotSlots.empty() ||
        !OutResolvedMainBaseLayoutDescriptorValue.SupportDepotSlots.empty() ||
        !OutResolvedMainBaseLayoutDescriptorValue.PeripheralDepotSlots.empty() ||
        !OutResolvedMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.empty() ||
        !OutResolvedMainBaseLayoutDescriptorValue.BarracksWithAddonSlots.empty() ||
        !OutResolvedMainBaseLayoutDescriptorValue.FactoryWithAddonSlots.empty() ||
        !OutResolvedMainBaseLayoutDescriptorValue.StarportWithAddonSlots.empty();
}

bool IsAuthoredMainBaseLayoutResolutionPreferred(
    const FMainBaseLayoutDescriptor& CandidateMainBaseLayoutDescriptorValue,
    const size_t CandidateProductionRailSearchOffsetIndexValue,
    const FMainBaseLayoutDescriptor& CurrentMainBaseLayoutDescriptorValue,
    const size_t CurrentProductionRailSearchOffsetIndexValue)
{
    const uint32_t CandidateResolutionScoreValue =
        GetAuthoredMainBaseLayoutResolutionScore(CandidateMainBaseLayoutDescriptorValue);
    const uint32_t CurrentResolutionScoreValue =
        GetAuthoredMainBaseLayoutResolutionScore(CurrentMainBaseLayoutDescriptorValue);
    if (CandidateResolutionScoreValue != CurrentResolutionScoreValue)
    {
        return CandidateResolutionScoreValue > CurrentResolutionScoreValue;
    }

    const bool CandidateHasProductionRailValue =
        !CandidateMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.empty();
    const bool CurrentHasProductionRailValue =
        !CurrentMainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.empty();
    if (CandidateHasProductionRailValue != CurrentHasProductionRailValue)
    {
        return CandidateHasProductionRailValue;
    }

    if (CandidateHasProductionRailValue &&
        CandidateProductionRailSearchOffsetIndexValue != CurrentProductionRailSearchOffsetIndexValue)
    {
        const std::vector<Point2DI>& LayoutSearchOffsetsValue = GetLayoutAnchorSearchOffsets();
        const Point2DI CandidateSearchOffsetValue =
            CandidateProductionRailSearchOffsetIndexValue < LayoutSearchOffsetsValue.size()
                ? LayoutSearchOffsetsValue[CandidateProductionRailSearchOffsetIndexValue]
                : Point2DI(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
        const Point2DI CurrentSearchOffsetValue =
            CurrentProductionRailSearchOffsetIndexValue < LayoutSearchOffsetsValue.size()
                ? LayoutSearchOffsetsValue[CurrentProductionRailSearchOffsetIndexValue]
                : Point2DI(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
        const int CandidateSearchDistanceValue =
            std::max(std::abs(CandidateSearchOffsetValue.x), std::abs(CandidateSearchOffsetValue.y));
        const int CurrentSearchDistanceValue =
            std::max(std::abs(CurrentSearchOffsetValue.x), std::abs(CurrentSearchOffsetValue.y));
        constexpr int PreferredSideSearchDistanceBiasValue = 2;
        return (CandidateSearchDistanceValue + PreferredSideSearchDistanceBiasValue) <
               CurrentSearchDistanceValue;
    }

    return false;
}

uint32_t GetAuthoredMainBaseLayoutResolutionScore(
    const FMainBaseLayoutDescriptor& MainBaseLayoutDescriptorValue)
{
    return (static_cast<uint32_t>(MainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.size()) * 100U) +
           (static_cast<uint32_t>(MainBaseLayoutDescriptorValue.BarracksWithAddonSlots.size()) * 10U) +
           (static_cast<uint32_t>(MainBaseLayoutDescriptorValue.FactoryWithAddonSlots.size()) * 10U) +
           (static_cast<uint32_t>(MainBaseLayoutDescriptorValue.StarportWithAddonSlots.size()) * 10U) +
           static_cast<uint32_t>(MainBaseLayoutDescriptorValue.NaturalEntranceWallDepotSlots.size()) +
           static_cast<uint32_t>(MainBaseLayoutDescriptorValue.PeripheralDepotSlots.size()) +
           static_cast<uint32_t>(MainBaseLayoutDescriptorValue.SupportDepotSlots.size()) +
           static_cast<uint32_t>(MainBaseLayoutDescriptorValue.NaturalApproachDepotSlots.size());
}

bool TryResolveProductionLayoutAnchorPoint(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue,
    const Point2D& RequestedLayoutAnchorPointValue, const Point2D& MainBaseDepthDirectionValue,
    const Point2D& MainBaseLateralDirectionValue,
    const std::vector<FResolvedLayoutPlacementSlot>& SeedResolvedLayoutPlacementSlotsValue,
    Point2D& OutResolvedLayoutAnchorPointValue)
{
    if (FrameValue.GameInfo == nullptr)
    {
        OutResolvedLayoutAnchorPointValue = RequestedLayoutAnchorPointValue;
        return true;
    }

    const std::vector<Point2DI>& SearchOffsetsValue = GetLayoutAnchorSearchOffsets();
    for (const Point2DI& SearchOffsetValue : SearchOffsetsValue)
    {
        const Point2D CandidateLayoutAnchorPointValue(
            RequestedLayoutAnchorPointValue.x + static_cast<float>(SearchOffsetValue.x),
            RequestedLayoutAnchorPointValue.y + static_cast<float>(SearchOffsetValue.y));
        std::vector<FResolvedLayoutPlacementSlot> CandidateResolvedLayoutPlacementSlotsValue =
            SeedResolvedLayoutPlacementSlotsValue;

        const FBuildPlacementSlot MainBarracksPlacementSlotValue = CreatePlacementSlot(
            EBuildPlacementSlotType::MainBarracksWithAddon,
            EBuildPlacementFootprintPolicy::RequiresAddonClearance,
            ProjectOffsetToWorld(CandidateLayoutAnchorPointValue, MainBaseDepthDirectionValue,
                                 MainBaseLateralDirectionValue, {8.0f, 0.0f}),
            0U);
        FBuildPlacementSlot ResolvedMainBarracksPlacementSlotValue;
        if (!TryResolveExactPlacementSlot(FrameValue, BuildPlacementContextValue, MainBarracksPlacementSlotValue,
                                          ABILITY_ID::BUILD_BARRACKS,
                                          CandidateResolvedLayoutPlacementSlotsValue,
                                          ResolvedMainBarracksPlacementSlotValue))
        {
            continue;
        }

        CandidateResolvedLayoutPlacementSlotsValue.push_back(
            {ResolvedMainBarracksPlacementSlotValue, ABILITY_ID::BUILD_BARRACKS});

        const FBuildPlacementSlot MainFactoryPlacementSlotValue = CreatePlacementSlot(
            EBuildPlacementSlotType::MainFactoryWithAddon,
            EBuildPlacementFootprintPolicy::RequiresAddonClearance,
            ProjectOffsetToWorld(CandidateLayoutAnchorPointValue, MainBaseDepthDirectionValue,
                                 MainBaseLateralDirectionValue, {4.0f, 8.0f}),
            0U);
        FBuildPlacementSlot ResolvedMainFactoryPlacementSlotValue;
        if (!TryResolveExactPlacementSlot(FrameValue, BuildPlacementContextValue, MainFactoryPlacementSlotValue,
                                          ABILITY_ID::BUILD_FACTORY,
                                          CandidateResolvedLayoutPlacementSlotsValue,
                                          ResolvedMainFactoryPlacementSlotValue))
        {
            continue;
        }

        CandidateResolvedLayoutPlacementSlotsValue.push_back(
            {ResolvedMainFactoryPlacementSlotValue, ABILITY_ID::BUILD_FACTORY});

        const FBuildPlacementSlot MainStarportPlacementSlotValue = CreatePlacementSlot(
            EBuildPlacementSlotType::MainStarportWithAddon,
            EBuildPlacementFootprintPolicy::RequiresAddonClearance,
            ProjectOffsetToWorld(CandidateLayoutAnchorPointValue, MainBaseDepthDirectionValue,
                                 MainBaseLateralDirectionValue, {4.0f, 14.0f}),
            0U);
        FBuildPlacementSlot ResolvedMainStarportPlacementSlotValue;
        if (!TryResolveExactPlacementSlot(FrameValue, BuildPlacementContextValue, MainStarportPlacementSlotValue,
                                          ABILITY_ID::BUILD_STARPORT,
                                          CandidateResolvedLayoutPlacementSlotsValue,
                                          ResolvedMainStarportPlacementSlotValue))
        {
            continue;
        }

        OutResolvedLayoutAnchorPointValue = CandidateLayoutAnchorPointValue;
        return true;
    }

    return false;
}

void AppendExactSlotsToLayoutDescriptor(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue,
    const std::vector<FBuildPlacementSlot>& TemplateBuildPlacementSlotsValue, const ABILITY_ID StructureAbilityIdValue,
    std::vector<FBuildPlacementSlot>& OutPlacementSlotsValue,
    std::vector<FResolvedLayoutPlacementSlot>& OutResolvedLayoutPlacementSlotsValue,
    const EPlacementCandidateValidationMode ValidationModeValue)
{
    for (const FBuildPlacementSlot& TemplateBuildPlacementSlotValue : TemplateBuildPlacementSlotsValue)
    {
        FBuildPlacementSlot CandidateBuildPlacementSlotValue = TemplateBuildPlacementSlotValue;
        CandidateBuildPlacementSlotValue.SlotId.Ordinal = static_cast<uint8_t>(OutPlacementSlotsValue.size());

        FBuildPlacementSlot ResolvedBuildPlacementSlotValue;
        if (!TryResolveExactPlacementSlot(FrameValue, BuildPlacementContextValue,
                                          CandidateBuildPlacementSlotValue, StructureAbilityIdValue,
                                          OutResolvedLayoutPlacementSlotsValue,
                                          ResolvedBuildPlacementSlotValue,
                                          ValidationModeValue))
        {
            continue;
        }

        ResolvedBuildPlacementSlotValue.SlotId = CandidateBuildPlacementSlotValue.SlotId;
        OutPlacementSlotsValue.push_back(ResolvedBuildPlacementSlotValue);

        FResolvedLayoutPlacementSlot ResolvedLayoutPlacementSlotValue;
        ResolvedLayoutPlacementSlotValue.BuildPlacementSlot = ResolvedBuildPlacementSlotValue;
        ResolvedLayoutPlacementSlotValue.StructureAbilityId = StructureAbilityIdValue;
        OutResolvedLayoutPlacementSlotsValue.push_back(ResolvedLayoutPlacementSlotValue);
    }
}

ABILITY_ID GetProductionRailRepresentativeAbilityId(const uint8_t SlotOrdinalValue)
{
    switch (SlotOrdinalValue)
    {
        case 0U:
            return ABILITY_ID::BUILD_BARRACKS;
        case 1U:
            return ABILITY_ID::BUILD_FACTORY;
        case 2U:
            return ABILITY_ID::BUILD_STARPORT;
        default:
            return ABILITY_ID::BUILD_BARRACKS;
    }
}

bool TryResolveExactProductionRailSlotGroup(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue,
    const std::vector<FBuildPlacementSlot>& TemplateBuildPlacementSlotsValue,
    const std::vector<FResolvedLayoutPlacementSlot>& SeedResolvedLayoutPlacementSlotsValue,
    std::vector<FBuildPlacementSlot>& OutResolvedBuildPlacementSlotsValue,
    std::vector<FResolvedLayoutPlacementSlot>& OutResolvedLayoutPlacementSlotsValue)
{
    return TryResolveProductionRailPivotSlot(FrameValue, BuildPlacementContextValue,
                                             TemplateBuildPlacementSlotsValue,
                                             SeedResolvedLayoutPlacementSlotsValue,
                                             Point2D(0.0f, 0.0f),
                                             GetProductionRailPivotSlotIndex(
                                                 TemplateBuildPlacementSlotsValue),
                                             OutResolvedBuildPlacementSlotsValue,
                                             OutResolvedLayoutPlacementSlotsValue);
}

size_t GetProductionRailPivotSlotIndex(
    const std::vector<FBuildPlacementSlot>& TemplateBuildPlacementSlotsValue)
{
    for (size_t SlotIndexValue = 0U; SlotIndexValue < TemplateBuildPlacementSlotsValue.size(); ++SlotIndexValue)
    {
        if (TemplateBuildPlacementSlotsValue[SlotIndexValue].SlotId.Ordinal == 1U)
        {
            return SlotIndexValue;
        }
    }

    return 0U;
}

bool TryResolveProductionRailPivotSlot(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue,
    const std::vector<FBuildPlacementSlot>& TemplateBuildPlacementSlotsValue,
    const std::vector<FResolvedLayoutPlacementSlot>& SeedResolvedLayoutPlacementSlotsValue,
    const Point2D& TranslationOffsetValue, const size_t PivotSlotIndexValue,
    std::vector<FBuildPlacementSlot>& OutResolvedBuildPlacementSlotsValue,
    std::vector<FResolvedLayoutPlacementSlot>& OutResolvedLayoutPlacementSlotsValue)
{
    if (TemplateBuildPlacementSlotsValue.empty() || PivotSlotIndexValue >= TemplateBuildPlacementSlotsValue.size())
    {
        OutResolvedBuildPlacementSlotsValue.clear();
        OutResolvedLayoutPlacementSlotsValue = SeedResolvedLayoutPlacementSlotsValue;
        return TemplateBuildPlacementSlotsValue.empty();
    }

    OutResolvedBuildPlacementSlotsValue.assign(TemplateBuildPlacementSlotsValue.size(), FBuildPlacementSlot());
    OutResolvedLayoutPlacementSlotsValue = SeedResolvedLayoutPlacementSlotsValue;
    OutResolvedLayoutPlacementSlotsValue.reserve(SeedResolvedLayoutPlacementSlotsValue.size() +
                                                 TemplateBuildPlacementSlotsValue.size());

    FBuildPlacementSlot PivotTemplateBuildPlacementSlotValue =
        TemplateBuildPlacementSlotsValue[PivotSlotIndexValue];
    PivotTemplateBuildPlacementSlotValue.BuildPoint =
        PivotTemplateBuildPlacementSlotValue.BuildPoint + TranslationOffsetValue;
    const ABILITY_ID PivotStructureAbilityIdValue =
        GetProductionRailRepresentativeAbilityId(PivotTemplateBuildPlacementSlotValue.SlotId.Ordinal);
    FBuildPlacementSlot ResolvedPivotBuildPlacementSlotValue;
    if (!TryResolveExactPlacementSlot(FrameValue, BuildPlacementContextValue,
                                      PivotTemplateBuildPlacementSlotValue,
                                      PivotStructureAbilityIdValue,
                                      OutResolvedLayoutPlacementSlotsValue,
                                      ResolvedPivotBuildPlacementSlotValue,
                                      EPlacementCandidateValidationMode::StaticLayout))
    {
        OutResolvedBuildPlacementSlotsValue.clear();
        OutResolvedLayoutPlacementSlotsValue.clear();
        return false;
    }

    ResolvedPivotBuildPlacementSlotValue.SlotId = PivotTemplateBuildPlacementSlotValue.SlotId;
    OutResolvedBuildPlacementSlotsValue[PivotSlotIndexValue] = ResolvedPivotBuildPlacementSlotValue;

    FResolvedLayoutPlacementSlot ResolvedPivotLayoutPlacementSlotValue;
    ResolvedPivotLayoutPlacementSlotValue.BuildPlacementSlot = ResolvedPivotBuildPlacementSlotValue;
    ResolvedPivotLayoutPlacementSlotValue.StructureAbilityId = PivotStructureAbilityIdValue;
    OutResolvedLayoutPlacementSlotsValue.push_back(ResolvedPivotLayoutPlacementSlotValue);

    for (size_t SlotIndexValue = PivotSlotIndexValue; SlotIndexValue > 0U; --SlotIndexValue)
    {
        const size_t PreviousSlotIndexValue = SlotIndexValue - 1U;
        const FBuildPlacementSlot& CurrentTemplateBuildPlacementSlotValue =
            TemplateBuildPlacementSlotsValue[SlotIndexValue];
        FBuildPlacementSlot PreviousTemplateBuildPlacementSlotValue =
            TemplateBuildPlacementSlotsValue[PreviousSlotIndexValue];
        const Point2D RelativeOffsetValue =
            PreviousTemplateBuildPlacementSlotValue.BuildPoint - CurrentTemplateBuildPlacementSlotValue.BuildPoint;
        PreviousTemplateBuildPlacementSlotValue.BuildPoint =
            OutResolvedBuildPlacementSlotsValue[SlotIndexValue].BuildPoint + RelativeOffsetValue;
        const ABILITY_ID StructureAbilityIdValue =
            GetProductionRailRepresentativeAbilityId(PreviousTemplateBuildPlacementSlotValue.SlotId.Ordinal);
        FBuildPlacementSlot ResolvedBuildPlacementSlotValue;
        if (!TryResolveExactPlacementSlot(FrameValue, BuildPlacementContextValue,
                                          PreviousTemplateBuildPlacementSlotValue,
                                          StructureAbilityIdValue,
                                          OutResolvedLayoutPlacementSlotsValue,
                                          ResolvedBuildPlacementSlotValue,
                                          EPlacementCandidateValidationMode::StaticLayout))
        {
            OutResolvedBuildPlacementSlotsValue.clear();
            OutResolvedLayoutPlacementSlotsValue.clear();
            return false;
        }

        ResolvedBuildPlacementSlotValue.SlotId = PreviousTemplateBuildPlacementSlotValue.SlotId;
        OutResolvedBuildPlacementSlotsValue[PreviousSlotIndexValue] = ResolvedBuildPlacementSlotValue;

        FResolvedLayoutPlacementSlot ResolvedLayoutPlacementSlotValue;
        ResolvedLayoutPlacementSlotValue.BuildPlacementSlot = ResolvedBuildPlacementSlotValue;
        ResolvedLayoutPlacementSlotValue.StructureAbilityId = StructureAbilityIdValue;
        OutResolvedLayoutPlacementSlotsValue.push_back(ResolvedLayoutPlacementSlotValue);
    }

    size_t LastResolvedSlotIndexValue = PivotSlotIndexValue;

    for (size_t SlotIndexValue = PivotSlotIndexValue + 1U;
         SlotIndexValue < TemplateBuildPlacementSlotsValue.size();
         ++SlotIndexValue)
    {
        const FBuildPlacementSlot& PreviousTemplateBuildPlacementSlotValue =
            TemplateBuildPlacementSlotsValue[SlotIndexValue - 1U];
        FBuildPlacementSlot CurrentTemplateBuildPlacementSlotValue =
            TemplateBuildPlacementSlotsValue[SlotIndexValue];
        const Point2D RelativeOffsetValue =
            CurrentTemplateBuildPlacementSlotValue.BuildPoint - PreviousTemplateBuildPlacementSlotValue.BuildPoint;
        CurrentTemplateBuildPlacementSlotValue.BuildPoint =
            OutResolvedBuildPlacementSlotsValue[SlotIndexValue - 1U].BuildPoint + RelativeOffsetValue;
        const ABILITY_ID StructureAbilityIdValue =
            GetProductionRailRepresentativeAbilityId(CurrentTemplateBuildPlacementSlotValue.SlotId.Ordinal);
        FBuildPlacementSlot ResolvedBuildPlacementSlotValue;
        if (!TryResolveExactPlacementSlot(FrameValue, BuildPlacementContextValue,
                                          CurrentTemplateBuildPlacementSlotValue,
                                          StructureAbilityIdValue,
                                          OutResolvedLayoutPlacementSlotsValue,
                                          ResolvedBuildPlacementSlotValue,
                                          EPlacementCandidateValidationMode::StaticLayout))
        {
            break;
        }

        ResolvedBuildPlacementSlotValue.SlotId = CurrentTemplateBuildPlacementSlotValue.SlotId;
        OutResolvedBuildPlacementSlotsValue[SlotIndexValue] = ResolvedBuildPlacementSlotValue;
        LastResolvedSlotIndexValue = SlotIndexValue;

        FResolvedLayoutPlacementSlot ResolvedLayoutPlacementSlotValue;
        ResolvedLayoutPlacementSlotValue.BuildPlacementSlot = ResolvedBuildPlacementSlotValue;
        ResolvedLayoutPlacementSlotValue.StructureAbilityId = StructureAbilityIdValue;
        OutResolvedLayoutPlacementSlotsValue.push_back(ResolvedLayoutPlacementSlotValue);
    }

    OutResolvedBuildPlacementSlotsValue.resize(LastResolvedSlotIndexValue + 1U);
    return !OutResolvedBuildPlacementSlotsValue.empty();
}

bool RemoveResolvedLayoutPlacementSlotById(
    std::vector<FResolvedLayoutPlacementSlot>& ResolvedLayoutPlacementSlotsValue,
    const FBuildPlacementSlotId& BuildPlacementSlotIdValue)
{
    for (std::vector<FResolvedLayoutPlacementSlot>::iterator ResolvedLayoutPlacementSlotIteratorValue =
             ResolvedLayoutPlacementSlotsValue.begin();
         ResolvedLayoutPlacementSlotIteratorValue != ResolvedLayoutPlacementSlotsValue.end();
         ++ResolvedLayoutPlacementSlotIteratorValue)
    {
        if (ResolvedLayoutPlacementSlotIteratorValue->BuildPlacementSlot.SlotId != BuildPlacementSlotIdValue)
        {
            continue;
        }

        ResolvedLayoutPlacementSlotsValue.erase(ResolvedLayoutPlacementSlotIteratorValue);
        return true;
    }

    return false;
}

bool RemovePlacementSlotById(std::vector<FBuildPlacementSlot>& BuildPlacementSlotsValue,
                             const FBuildPlacementSlotId& BuildPlacementSlotIdValue)
{
    for (std::vector<FBuildPlacementSlot>::iterator BuildPlacementSlotIteratorValue =
             BuildPlacementSlotsValue.begin();
         BuildPlacementSlotIteratorValue != BuildPlacementSlotsValue.end();
         ++BuildPlacementSlotIteratorValue)
    {
        if (BuildPlacementSlotIteratorValue->SlotId != BuildPlacementSlotIdValue)
        {
            continue;
        }

        BuildPlacementSlotsValue.erase(BuildPlacementSlotIteratorValue);
        return true;
    }

    return false;
}

bool TryResolveDerivedProductionRailFromBarracksSeed(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue,
    const FBuildPlacementSlot& BarracksAnchorSlotValue,
    const std::vector<FResolvedLayoutPlacementSlot>& SeedResolvedLayoutPlacementSlotsValue,
    std::vector<FBuildPlacementSlot>& OutResolvedBuildPlacementSlotsValue,
    std::vector<FResolvedLayoutPlacementSlot>& OutResolvedLayoutPlacementSlotsValue)
{
    if (!BuildPlacementContextValue.RampWallDescriptor.bIsValid)
    {
        OutResolvedBuildPlacementSlotsValue.clear();
        OutResolvedLayoutPlacementSlotsValue.clear();
        return false;
    }

    const Point2D RailStepOffsetValue =
        BarracksAnchorSlotValue.BuildPoint - BuildPlacementContextValue.RampWallDescriptor.BarracksSlot.BuildPoint;
    if (DistanceSquared2D(Point2D(0.0f, 0.0f), RailStepOffsetValue) <= 4.0f)
    {
        OutResolvedBuildPlacementSlotsValue.clear();
        OutResolvedLayoutPlacementSlotsValue.clear();
        return false;
    }

    OutResolvedBuildPlacementSlotsValue.clear();
    OutResolvedBuildPlacementSlotsValue.reserve(3U);
    OutResolvedLayoutPlacementSlotsValue = SeedResolvedLayoutPlacementSlotsValue;
    OutResolvedLayoutPlacementSlotsValue.reserve(SeedResolvedLayoutPlacementSlotsValue.size() + 3U);

    const FBuildPlacementSlot BarracksTemplateBuildPlacementSlotValue = CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        BarracksAnchorSlotValue.BuildPoint, 0U);
    FBuildPlacementSlot ResolvedBarracksBuildPlacementSlotValue;
    if (!TryResolveExactPlacementSlot(FrameValue, BuildPlacementContextValue,
                                      BarracksTemplateBuildPlacementSlotValue,
                                      ABILITY_ID::BUILD_BARRACKS,
                                      OutResolvedLayoutPlacementSlotsValue,
                                      ResolvedBarracksBuildPlacementSlotValue,
                                      EPlacementCandidateValidationMode::StaticLayout))
    {
        OutResolvedBuildPlacementSlotsValue.clear();
        OutResolvedLayoutPlacementSlotsValue.clear();
        return false;
    }

    ResolvedBarracksBuildPlacementSlotValue.SlotId = BarracksTemplateBuildPlacementSlotValue.SlotId;
    OutResolvedBuildPlacementSlotsValue.push_back(ResolvedBarracksBuildPlacementSlotValue);

    FResolvedLayoutPlacementSlot ResolvedBarracksLayoutPlacementSlotValue;
    ResolvedBarracksLayoutPlacementSlotValue.BuildPlacementSlot = ResolvedBarracksBuildPlacementSlotValue;
    ResolvedBarracksLayoutPlacementSlotValue.StructureAbilityId = ABILITY_ID::BUILD_BARRACKS;
    OutResolvedLayoutPlacementSlotsValue.push_back(ResolvedBarracksLayoutPlacementSlotValue);

    const FBuildPlacementSlot FactoryTemplateBuildPlacementSlotValue = CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        ResolvedBarracksBuildPlacementSlotValue.BuildPoint + RailStepOffsetValue, 1U);
    FBuildPlacementSlot ResolvedFactoryBuildPlacementSlotValue;
    if (!TryResolveAuthoredPlacementSlot(FrameValue, BuildPlacementContextValue,
                                         FactoryTemplateBuildPlacementSlotValue,
                                         ABILITY_ID::BUILD_FACTORY,
                                         OutResolvedLayoutPlacementSlotsValue,
                                         ResolvedFactoryBuildPlacementSlotValue,
                                         EPlacementCandidateValidationMode::StaticLayout))
    {
        OutResolvedBuildPlacementSlotsValue.clear();
        OutResolvedLayoutPlacementSlotsValue.clear();
        return false;
    }

    ResolvedFactoryBuildPlacementSlotValue.SlotId = FactoryTemplateBuildPlacementSlotValue.SlotId;
    OutResolvedBuildPlacementSlotsValue.push_back(ResolvedFactoryBuildPlacementSlotValue);

    FResolvedLayoutPlacementSlot ResolvedFactoryLayoutPlacementSlotValue;
    ResolvedFactoryLayoutPlacementSlotValue.BuildPlacementSlot = ResolvedFactoryBuildPlacementSlotValue;
    ResolvedFactoryLayoutPlacementSlotValue.StructureAbilityId = ABILITY_ID::BUILD_FACTORY;
    OutResolvedLayoutPlacementSlotsValue.push_back(ResolvedFactoryLayoutPlacementSlotValue);

    const FBuildPlacementSlot StarportTemplateBuildPlacementSlotValue = CreatePlacementSlot(
        EBuildPlacementSlotType::MainProductionWithAddon,
        EBuildPlacementFootprintPolicy::RequiresAddonClearance,
        ResolvedFactoryBuildPlacementSlotValue.BuildPoint + RailStepOffsetValue, 2U);
    FBuildPlacementSlot ResolvedStarportBuildPlacementSlotValue;
    if (!TryResolveAuthoredPlacementSlot(FrameValue, BuildPlacementContextValue,
                                         StarportTemplateBuildPlacementSlotValue,
                                         ABILITY_ID::BUILD_STARPORT,
                                         OutResolvedLayoutPlacementSlotsValue,
                                         ResolvedStarportBuildPlacementSlotValue,
                                         EPlacementCandidateValidationMode::StaticLayout))
    {
        OutResolvedBuildPlacementSlotsValue.clear();
        OutResolvedLayoutPlacementSlotsValue.clear();
        return false;
    }

    ResolvedStarportBuildPlacementSlotValue.SlotId = StarportTemplateBuildPlacementSlotValue.SlotId;
    OutResolvedBuildPlacementSlotsValue.push_back(ResolvedStarportBuildPlacementSlotValue);

    FResolvedLayoutPlacementSlot ResolvedStarportLayoutPlacementSlotValue;
    ResolvedStarportLayoutPlacementSlotValue.BuildPlacementSlot = ResolvedStarportBuildPlacementSlotValue;
    ResolvedStarportLayoutPlacementSlotValue.StructureAbilityId = ABILITY_ID::BUILD_STARPORT;
    OutResolvedLayoutPlacementSlotsValue.push_back(ResolvedStarportLayoutPlacementSlotValue);

    return true;
}

bool TryResolveDerivedProductionRailSlotGroup(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue,
    const std::vector<FBuildPlacementSlot>& BarracksAnchorSlotsValue,
    const std::vector<FResolvedLayoutPlacementSlot>& SeedResolvedLayoutPlacementSlotsValue,
    std::vector<FBuildPlacementSlot>& OutResolvedBuildPlacementSlotsValue,
    std::vector<FResolvedLayoutPlacementSlot>& OutResolvedLayoutPlacementSlotsValue,
    FBuildPlacementSlotId& OutSourceBarracksSlotIdValue)
{
    for (const FBuildPlacementSlot& BarracksAnchorSlotValue : BarracksAnchorSlotsValue)
    {
        std::vector<FResolvedLayoutPlacementSlot> CandidateResolvedLayoutPlacementSlotsValue =
            SeedResolvedLayoutPlacementSlotsValue;
        RemoveResolvedLayoutPlacementSlotById(CandidateResolvedLayoutPlacementSlotsValue,
                                              BarracksAnchorSlotValue.SlotId);

        if (!TryResolveDerivedProductionRailFromBarracksSeed(FrameValue, BuildPlacementContextValue,
                                                             BarracksAnchorSlotValue,
                                                             CandidateResolvedLayoutPlacementSlotsValue,
                                                             OutResolvedBuildPlacementSlotsValue,
                                                             OutResolvedLayoutPlacementSlotsValue))
        {
            continue;
        }

        OutSourceBarracksSlotIdValue = BarracksAnchorSlotValue.SlotId;
        return true;
    }

    OutResolvedBuildPlacementSlotsValue.clear();
    OutResolvedLayoutPlacementSlotsValue.clear();
    OutSourceBarracksSlotIdValue = FBuildPlacementSlotId();
    return false;
}

bool TryResolveAuthoredProductionRailSlotGroup(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue,
    const std::vector<FBuildPlacementSlot>& TemplateBuildPlacementSlotsValue,
    const std::vector<FResolvedLayoutPlacementSlot>& SeedResolvedLayoutPlacementSlotsValue,
    std::vector<FBuildPlacementSlot>& OutResolvedBuildPlacementSlotsValue,
    std::vector<FResolvedLayoutPlacementSlot>& OutResolvedLayoutPlacementSlotsValue,
    size_t& OutSearchOffsetIndexValue)
{
    if (TemplateBuildPlacementSlotsValue.empty())
    {
        OutResolvedBuildPlacementSlotsValue.clear();
        OutResolvedLayoutPlacementSlotsValue = SeedResolvedLayoutPlacementSlotsValue;
        OutSearchOffsetIndexValue = 0U;
        return true;
    }

    if (TryResolveProductionRailPivotSlot(
            FrameValue, BuildPlacementContextValue,
            TemplateBuildPlacementSlotsValue,
            SeedResolvedLayoutPlacementSlotsValue,
            Point2D(0.0f, 0.0f),
            GetProductionRailPivotSlotIndex(TemplateBuildPlacementSlotsValue),
            OutResolvedBuildPlacementSlotsValue,
            OutResolvedLayoutPlacementSlotsValue))
    {
        OutSearchOffsetIndexValue = 0U;
        return true;
    }

    OutResolvedBuildPlacementSlotsValue.clear();
    OutResolvedLayoutPlacementSlotsValue.clear();
    OutSearchOffsetIndexValue = std::numeric_limits<size_t>::max();
    return false;
}

void AppendTemplateSlotsToLayoutDescriptor(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue,
    const Point2D& LayoutAnchorPointValue, const Point2D& MainBaseDepthDirectionValue,
    const Point2D& MainBaseLateralDirectionValue,
    const std::vector<FPlacementOffsetDescriptor>& PlacementOffsetsValue,
    const bool AllowTemplateSearchValue,
    const EBuildPlacementSlotType BuildPlacementSlotTypeValue,
    const EBuildPlacementFootprintPolicy BuildPlacementFootprintPolicyValue,
    const ABILITY_ID StructureAbilityIdValue, std::vector<FBuildPlacementSlot>& OutPlacementSlotsValue,
    std::vector<FResolvedLayoutPlacementSlot>& OutResolvedLayoutPlacementSlotsValue)
{
    for (const FPlacementOffsetDescriptor& PlacementOffsetValue : PlacementOffsetsValue)
    {
        const uint8_t SlotOrdinalValue = static_cast<uint8_t>(OutPlacementSlotsValue.size());
        const FBuildPlacementSlot TemplateBuildPlacementSlotValue = CreatePlacementSlot(
            BuildPlacementSlotTypeValue, BuildPlacementFootprintPolicyValue,
            ProjectOffsetToWorld(LayoutAnchorPointValue, MainBaseDepthDirectionValue,
                                 MainBaseLateralDirectionValue, PlacementOffsetValue),
            SlotOrdinalValue);

        FBuildPlacementSlot ResolvedBuildPlacementSlotValue;
        if (!TryResolveTemplatePlacementSlot(FrameValue, BuildPlacementContextValue,
                                             TemplateBuildPlacementSlotValue, StructureAbilityIdValue,
                                             OutResolvedLayoutPlacementSlotsValue,
                                             AllowTemplateSearchValue, ResolvedBuildPlacementSlotValue))
        {
            continue;
        }

        OutPlacementSlotsValue.push_back(ResolvedBuildPlacementSlotValue);

        FResolvedLayoutPlacementSlot ResolvedLayoutPlacementSlotValue;
        ResolvedLayoutPlacementSlotValue.BuildPlacementSlot = ResolvedBuildPlacementSlotValue;
        ResolvedLayoutPlacementSlotValue.StructureAbilityId = StructureAbilityIdValue;
        OutResolvedLayoutPlacementSlotsValue.push_back(ResolvedLayoutPlacementSlotValue);
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

bool DoesAddonFootprintOverlapDepot(const Point2D& BarracksPositionValue, const Point2D& DepotPositionValue)
{
    const Point2D AddonCenterValue = GetAddonFootprintCenter(BarracksPositionValue);
    static constexpr float AddonHalfExtentValue = 1.0f;
    static constexpr float DepotHalfExtentValue = 1.0f;
    const float DeltaXValue = std::abs(AddonCenterValue.x - DepotPositionValue.x);
    const float DeltaYValue = std::abs(AddonCenterValue.y - DepotPositionValue.y);
    return DeltaXValue <= (AddonHalfExtentValue + DepotHalfExtentValue) &&
           DeltaYValue <= (AddonHalfExtentValue + DepotHalfExtentValue);
}

bool DoesStructureFootprintSupportTerrain(const GameInfo& GameInfoValue, const ABILITY_ID StructureAbilityIdValue,
                                          const Point2D& StructureBuildPointValue)
{
    const PlacementGrid PlacementGridValue(GameInfoValue);
    const Point2D StructureFootprintHalfExtentsValue =
        GetStructureFootprintHalfExtentsForAbility(StructureAbilityIdValue);

    constexpr float SampleEpsilonValue = 0.001f;
    for (float XOffsetValue = (-StructureFootprintHalfExtentsValue.x + 0.5f);
         XOffsetValue <= (StructureFootprintHalfExtentsValue.x - 0.5f + SampleEpsilonValue);
         XOffsetValue += 1.0f)
    {
        for (float YOffsetValue = (-StructureFootprintHalfExtentsValue.y + 0.5f);
             YOffsetValue <= (StructureFootprintHalfExtentsValue.y - 0.5f + SampleEpsilonValue);
             YOffsetValue += 1.0f)
        {
            const Point2D SamplePointValue(StructureBuildPointValue.x + XOffsetValue,
                                           StructureBuildPointValue.y + YOffsetValue);
            if (!PlacementGridValue.IsPlacable(SamplePointValue))
            {
                return false;
            }
        }
    }

    return true;
}

bool DoesPointSatisfyFootprintPolicy(const FFrameContext& FrameValue, const Point2D& BuildPointValue,
                                     const ABILITY_ID StructureAbilityIdValue,
                                     const EBuildPlacementFootprintPolicy BuildPlacementFootprintPolicyValue,
                                     const EPlacementCandidateValidationMode ValidationModeValue =
                                         EPlacementCandidateValidationMode::RuntimeQuery)
{
    if (FrameValue.GameInfo == nullptr ||
        !DoesStructureFootprintSupportTerrain(*FrameValue.GameInfo, StructureAbilityIdValue, BuildPointValue))
    {
        return false;
    }

    switch (BuildPlacementFootprintPolicyValue)
    {
        case EBuildPlacementFootprintPolicy::StructureOnly:
            return true;
        case EBuildPlacementFootprintPolicy::RequiresAddonClearance:
            if (FrameValue.GameInfo == nullptr || !DoesStructureAbilityRequireAddonClearance(StructureAbilityIdValue))
            {
                return false;
            }

            if (ValidationModeValue == EPlacementCandidateValidationMode::StaticLayout)
            {
                return true;
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
                               const Point2D& CandidatePointValue,
                               const EPlacementCandidateValidationMode ValidationModeValue)
{
    if (FrameValue.GameInfo == nullptr)
    {
        return false;
    }

    const Point2D ClampedCandidatePointValue = ClampToPlayable(*FrameValue.GameInfo, CandidatePointValue);
    if (!DoesPointSatisfyFootprintPolicy(FrameValue, ClampedCandidatePointValue, StructureAbilityIdValue,
                                         BuildPlacementFootprintPolicyValue, ValidationModeValue))
    {
        return false;
    }

    if (ValidationModeValue == EPlacementCandidateValidationMode::RuntimeQuery &&
        FrameValue.Query != nullptr &&
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
                                                   const GameInfo* GameInfoPtrValue,
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

    // Validate addon clearance for the barracks position. The addon footprint is at
    // (+2.5, -0.5) from the barracks center. Try the barracks-in-middle position first,
    // then shifted candidates if addon terrain is blocked.
    if (GameInfoPtrValue != nullptr)
    {
        const Point2D BarracksInMiddleValue = BarracksBuildPointValue;
        static constexpr float BarracksAddonShiftDistanceValue = 2.0f;
        const std::array<Point2D, 3> BarracksCandidatePositionsValue =
        {{
            BarracksInMiddleValue,
            BarracksInMiddleValue + Point2D(-BarracksAddonShiftDistanceValue, 0.0f),
            BarracksInMiddleValue + Point2D(BarracksAddonShiftDistanceValue, 0.0f),
        }};

        bool FoundValidBarracksPositionValue = false;
        for (const Point2D& BarracksCandidateValue : BarracksCandidatePositionsValue)
        {
            if (DoesAddonFootprintSupportTerrain(*GameInfoPtrValue, BarracksCandidateValue))
            {
                BarracksBuildPointValue = BarracksCandidateValue;
                FoundValidBarracksPositionValue = true;
                break;
            }
        }

        if (!FoundValidBarracksPositionValue)
        {
            // No addon-terrain-valid position exists. Place the barracks at the
            // original wall position anyway; the addon will need to be built on a
            // production rail barracks instead.
            BarracksBuildPointValue = BarracksInMiddleValue;
        }
    }
    else
    {
        // Fallback when GameInfo is unavailable: use the python-sc2 X-axis heuristic.
        const float MaximumDepotXValue =
            std::max(CornerDepotPointOneValue.x, CornerDepotPointTwoValue.x);
        if ((BarracksBuildPointValue.x + 1.0f) <= MaximumDepotXValue)
        {
            BarracksBuildPointValue = BarracksBuildPointValue + Point2D(-2.0f, 0.0f);
        }
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
                                                       FrameValue.GameInfo,
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

    // Validate addon position at the barracks using Query->Placement as ground truth.
    // Use BUILD_SUPPLYDEPOT (2x2, same as addon) at the addon center as a proxy,
    // since the actual barracks unit does not yet exist for a direct addon query.
    bool IsAddonPositionValidValue = false;
    if (FrameValue.Query != nullptr)
    {
        const Point2D OriginalAddonCenterValue = GetAddonFootprintCenter(BarracksBuildPointValue);
        IsAddonPositionValidValue =
            FrameValue.Query->Placement(ABILITY_ID::BUILD_SUPPLYDEPOT, OriginalAddonCenterValue);

        if (!IsAddonPositionValidValue)
        {
            // The addon position is rejected by the game engine. Scan shifted barracks positions
            // along the wall lateral axis to find one where the addon lands on valid terrain.
            const Point2D RampDirectionValue =
                GetNormalizedDirection(OutsideStagingPointValue - InsideStagingPointValue);
            const Point2D WallLateralDirectionValue = GetLateralDirection(RampDirectionValue);

            // Collect batch placement queries for shifted candidates.
            // Test shifts along the lateral axis, depth axis, and diagonal combinations.
            struct FBarracksCandidateEntry
            {
                Point2D BarracksPointValue;
                Point2D AddonCenterPointValue;
            };

            std::vector<FBarracksCandidateEntry> BarracksCandidateEntriesValue;
            static constexpr float LateralShiftValues[] = {-1.0f, 1.0f, -2.0f, 2.0f, -3.0f, 3.0f};
            static constexpr float DepthShiftValues[] = {0.0f, -1.0f, 1.0f};

            for (const float DepthShiftValue : DepthShiftValues)
            {
                for (const float LateralShiftValue : LateralShiftValues)
                {
                    const Point2D ShiftedBarracksPointValue =
                        BarracksBuildPointValue +
                        (WallLateralDirectionValue * LateralShiftValue) +
                        (RampDirectionValue * DepthShiftValue);
                    const Point2D ShiftedAddonCenterValue =
                        GetAddonFootprintCenter(ShiftedBarracksPointValue);

                    FBarracksCandidateEntry CandidateEntryValue;
                    CandidateEntryValue.BarracksPointValue = ShiftedBarracksPointValue;
                    CandidateEntryValue.AddonCenterPointValue = ShiftedAddonCenterValue;
                    BarracksCandidateEntriesValue.push_back(CandidateEntryValue);
                }
            }

            // Build batch placement queries: for each candidate, test both the barracks
            // position and the addon position (via BUILD_SUPPLYDEPOT proxy).
            std::vector<QueryInterface::PlacementQuery> BatchPlacementQueriesValue;
            BatchPlacementQueriesValue.reserve(BarracksCandidateEntriesValue.size() * 2U);
            for (const FBarracksCandidateEntry& CandidateEntryValue : BarracksCandidateEntriesValue)
            {
                BatchPlacementQueriesValue.emplace_back(
                    ABILITY_ID::BUILD_BARRACKS, CandidateEntryValue.BarracksPointValue);
                BatchPlacementQueriesValue.emplace_back(
                    ABILITY_ID::BUILD_SUPPLYDEPOT, CandidateEntryValue.AddonCenterPointValue);
            }

            const std::vector<bool> BatchPlacementResultsValue =
                FrameValue.Query->Placement(BatchPlacementQueriesValue);

            // Find the first candidate where both barracks and addon positions are valid.
            for (size_t CandidateIndexValue = 0U;
                 CandidateIndexValue < BarracksCandidateEntriesValue.size(); ++CandidateIndexValue)
            {
                const size_t BarracksQueryIndexValue = CandidateIndexValue * 2U;
                const size_t AddonQueryIndexValue = BarracksQueryIndexValue + 1U;

                if (BarracksQueryIndexValue >= BatchPlacementResultsValue.size() ||
                    AddonQueryIndexValue >= BatchPlacementResultsValue.size())
                {
                    break;
                }

                if (BatchPlacementResultsValue[BarracksQueryIndexValue] &&
                    BatchPlacementResultsValue[AddonQueryIndexValue])
                {
                    BarracksBuildPointValue =
                        BarracksCandidateEntriesValue[CandidateIndexValue].BarracksPointValue;
                    IsAddonPositionValidValue = true;

#if _DEBUG
                    std::cout << "[WALL_ADDON_FIX] Shifted barracks to ("
                              << BarracksBuildPointValue.x << "," << BarracksBuildPointValue.y
                              << ") addon=("
                              << BarracksCandidateEntriesValue[CandidateIndexValue].AddonCenterPointValue.x
                              << ","
                              << BarracksCandidateEntriesValue[CandidateIndexValue].AddonCenterPointValue.y
                              << ")" << std::endl;
#endif
                    break;
                }
            }

#if _DEBUG
            if (!IsAddonPositionValidValue)
            {
                std::cout << "[WALL_ADDON_SCAN] No addon-valid barracks position found in "
                          << BarracksCandidateEntriesValue.size() << " candidates. "
                          << "Original barracks=(" << BarracksBuildPointValue.x << ","
                          << BarracksBuildPointValue.y << ") addon=("
                          << OriginalAddonCenterValue.x << "," << OriginalAddonCenterValue.y
                          << ")" << std::endl;

                // Log all candidate results for hardcoding reference.
                for (size_t DiagIndexValue = 0U;
                     DiagIndexValue < BarracksCandidateEntriesValue.size(); ++DiagIndexValue)
                {
                    const size_t BarracksResultIndexValue = DiagIndexValue * 2U;
                    const size_t AddonResultIndexValue = BarracksResultIndexValue + 1U;
                    if (AddonResultIndexValue >= BatchPlacementResultsValue.size())
                    {
                        break;
                    }

                    std::cout << "[WALL_ADDON_SCAN]   Candidate " << DiagIndexValue
                              << " barracks=("
                              << BarracksCandidateEntriesValue[DiagIndexValue].BarracksPointValue.x
                              << ","
                              << BarracksCandidateEntriesValue[DiagIndexValue].BarracksPointValue.y
                              << ") addon=("
                              << BarracksCandidateEntriesValue[DiagIndexValue].AddonCenterPointValue.x
                              << ","
                              << BarracksCandidateEntriesValue[DiagIndexValue].AddonCenterPointValue.y
                              << ") barracksOk="
                              << BatchPlacementResultsValue[BarracksResultIndexValue]
                              << " addonOk="
                              << BatchPlacementResultsValue[AddonResultIndexValue]
                              << std::endl;
                }
            }
#endif
        }
    }

    // Re-validate barracks placement after potential shift.
    if (!IsPlacementCandidateValid(FrameValue, ABILITY_ID::BUILD_BARRACKS,
                                   EBuildPlacementFootprintPolicy::RequiresAddonClearance,
                                   BarracksBuildPointValue))
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

#if _DEBUG
    {
        const Point2D AddonCenterValue = GetAddonFootprintCenter(BarracksBuildPointValue);
        std::cout << "[WALL_POSITIONS] LeftDepot=(" << LeftDepotBuildPointValue.x << ","
                  << LeftDepotBuildPointValue.y << ") Barracks=(" << BarracksBuildPointValue.x << ","
                  << BarracksBuildPointValue.y << ") RightDepot=(" << RightDepotBuildPointValue.x << ","
                  << RightDepotBuildPointValue.y << ") AddonCenter=(" << AddonCenterValue.x << ","
                  << AddonCenterValue.y << ") WallCenter=(" << WallCenterPointValue.x << ","
                  << WallCenterPointValue.y << ") AddonValid=" << IsAddonPositionValidValue
                  << std::endl;

        std::cout << "[WALL_SPAWN] BaseLocation=("
                  << BuildPlacementContextValue.BaseLocation.x << ","
                  << BuildPlacementContextValue.BaseLocation.y << ") MapName="
                  << BuildPlacementContextValue.MapName << std::endl;
    }
#endif

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

FMainBaseLayoutDescriptor FTerranBuildPlacementService::GetMainBaseLayoutDescriptor(
    const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue) const
{
    FMainBaseLayoutDescriptor MainBaseLayoutDescriptorValue;
    const Point2D MainBaseDepthDirectionValue = GetMainBaseDepthDirection(BuildPlacementContextValue);
    Point2D MainBaseLateralDirectionValue =
        GetMainBaseLateralDirection(FrameValue, BuildPlacementContextValue, MainBaseDepthDirectionValue);
    const Point2D RequestedLayoutAnchorPointValue =
        GetMainBaseLayoutAnchorPoint(BuildPlacementContextValue, MainBaseDepthDirectionValue);

    const std::vector<FPlacementOffsetDescriptor> NaturalApproachDepotOffsetValues =
    {
        {12.0f, -4.0f},
        {-12.0f, -4.0f},
    };
    const std::vector<FPlacementOffsetDescriptor> SupportDepotOffsetValues =
    {
        {20.0f, 0.0f},
        {-20.0f, 0.0f},
        {24.0f, 8.0f},
        {-24.0f, 8.0f},
        {18.0f, 16.0f},
        {-18.0f, 16.0f},
    };
    const std::vector<FPlacementOffsetDescriptor> PeripheralDepotOffsetValues =
    {
        {28.0f, 20.0f},
        {-28.0f, 20.0f},
        {32.0f, 28.0f},
        {-32.0f, 28.0f},
        {12.0f, 28.0f},
        {-12.0f, 28.0f},
        {0.0f, 32.0f},
        {0.0f, -8.0f},
    };
    const std::vector<FPlacementOffsetDescriptor> BarracksOffsetValues =
    {
        {8.0f, 0.0f},
        {-8.0f, 0.0f},
        {12.0f, 8.0f},
        {-12.0f, 8.0f},
    };
    const std::vector<FPlacementOffsetDescriptor> FactoryOffsetValues =
    {
        {4.0f, 8.0f},
        {-4.0f, 8.0f},
        {8.0f, 16.0f},
        {-8.0f, 16.0f},
    };
    const std::vector<FPlacementOffsetDescriptor> StarportOffsetValues =
    {
        {0.0f, 16.0f},
        {12.0f, 16.0f},
        {4.0f, 24.0f},
        {-4.0f, 24.0f},
    };

    std::vector<FResolvedLayoutPlacementSlot> ResolvedLayoutPlacementSlotsValue;
    if (BuildPlacementContextValue.RampWallDescriptor.bIsValid)
    {
        FResolvedLayoutPlacementSlot LeftWallPlacementSlotValue;
        LeftWallPlacementSlotValue.BuildPlacementSlot = BuildPlacementContextValue.RampWallDescriptor.LeftDepotSlot;
        LeftWallPlacementSlotValue.StructureAbilityId = ABILITY_ID::BUILD_SUPPLYDEPOT;
        ResolvedLayoutPlacementSlotsValue.push_back(LeftWallPlacementSlotValue);

        FResolvedLayoutPlacementSlot BarracksWallPlacementSlotValue;
        BarracksWallPlacementSlotValue.BuildPlacementSlot = BuildPlacementContextValue.RampWallDescriptor.BarracksSlot;
        BarracksWallPlacementSlotValue.StructureAbilityId = ABILITY_ID::BUILD_BARRACKS;
        ResolvedLayoutPlacementSlotsValue.push_back(BarracksWallPlacementSlotValue);

        FResolvedLayoutPlacementSlot RightWallPlacementSlotValue;
        RightWallPlacementSlotValue.BuildPlacementSlot = BuildPlacementContextValue.RampWallDescriptor.RightDepotSlot;
        RightWallPlacementSlotValue.StructureAbilityId = ABILITY_ID::BUILD_SUPPLYDEPOT;
        ResolvedLayoutPlacementSlotsValue.push_back(RightWallPlacementSlotValue);
    }

    const FTerranMainBaseLayoutRegistry TerranMainBaseLayoutRegistryValue;
    FMainBaseLayoutDescriptor AuthoredMainBaseLayoutDescriptorValue;
    const bool HasAuthoredMainBaseLayoutValue = TerranMainBaseLayoutRegistryValue.TryGetAuthoredMainBaseLayout(
        FrameValue.GameInfo, BuildPlacementContextValue, AuthoredMainBaseLayoutDescriptorValue);

    MainBaseLayoutDescriptorValue.LayoutAnchorPoint =
        HasAuthoredMainBaseLayoutValue ? AuthoredMainBaseLayoutDescriptorValue.LayoutAnchorPoint
                                       : RequestedLayoutAnchorPointValue;

    if (HasAuthoredMainBaseLayoutValue)
    {
        FMainBaseLayoutDescriptor PrimaryResolvedAuthoredMainBaseLayoutDescriptorValue;
        std::vector<FResolvedLayoutPlacementSlot> PrimaryResolvedAuthoredPlacementSlotsValue =
            ResolvedLayoutPlacementSlotsValue;
        size_t PrimaryProductionRailSearchOffsetIndexValue = std::numeric_limits<size_t>::max();
        ResolveAuthoredMainBaseLayoutDescriptor(FrameValue, BuildPlacementContextValue,
                                                AuthoredMainBaseLayoutDescriptorValue,
                                                PrimaryResolvedAuthoredMainBaseLayoutDescriptorValue,
                                                PrimaryResolvedAuthoredPlacementSlotsValue,
                                                PrimaryProductionRailSearchOffsetIndexValue);

        const FMainBaseLayoutDescriptor MirroredAuthoredMainBaseLayoutDescriptorValue =
            CreateMirroredAuthoredMainBaseLayoutDescriptor(AuthoredMainBaseLayoutDescriptorValue,
                                                           MainBaseDepthDirectionValue,
                                                           MainBaseLateralDirectionValue);
        FMainBaseLayoutDescriptor MirroredResolvedAuthoredMainBaseLayoutDescriptorValue;
        std::vector<FResolvedLayoutPlacementSlot> MirroredResolvedAuthoredPlacementSlotsValue =
            ResolvedLayoutPlacementSlotsValue;
        size_t MirroredProductionRailSearchOffsetIndexValue = std::numeric_limits<size_t>::max();
        ResolveAuthoredMainBaseLayoutDescriptor(FrameValue, BuildPlacementContextValue,
                                                MirroredAuthoredMainBaseLayoutDescriptorValue,
                                                MirroredResolvedAuthoredMainBaseLayoutDescriptorValue,
                                                MirroredResolvedAuthoredPlacementSlotsValue,
                                                MirroredProductionRailSearchOffsetIndexValue);

        if (IsAuthoredMainBaseLayoutResolutionPreferred(
                MirroredResolvedAuthoredMainBaseLayoutDescriptorValue,
                MirroredProductionRailSearchOffsetIndexValue,
                PrimaryResolvedAuthoredMainBaseLayoutDescriptorValue,
                PrimaryProductionRailSearchOffsetIndexValue))
        {
            MainBaseLayoutDescriptorValue = MirroredResolvedAuthoredMainBaseLayoutDescriptorValue;
            ResolvedLayoutPlacementSlotsValue = MirroredResolvedAuthoredPlacementSlotsValue;
        }
        else
        {
            MainBaseLayoutDescriptorValue = PrimaryResolvedAuthoredMainBaseLayoutDescriptorValue;
            ResolvedLayoutPlacementSlotsValue = PrimaryResolvedAuthoredPlacementSlotsValue;
        }
    }
    else
    {
        MainBaseLayoutDescriptorValue.bUsesAuthoredProductionLayout = false;
    }

    const bool HasAuthoredProductionSlotsValue = !MainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.empty() ||
                                                 !MainBaseLayoutDescriptorValue.BarracksWithAddonSlots.empty() ||
                                                 !MainBaseLayoutDescriptorValue.FactoryWithAddonSlots.empty() ||
                                                 !MainBaseLayoutDescriptorValue.StarportWithAddonSlots.empty();

    Point2D ResolvedLayoutAnchorPointValue = MainBaseLayoutDescriptorValue.LayoutAnchorPoint;
    bool HasResolvedProductionLayoutAnchorValue = HasAuthoredProductionSlotsValue;
    if (!HasResolvedProductionLayoutAnchorValue)
    {
        HasResolvedProductionLayoutAnchorValue = TryResolveProductionLayoutAnchorPoint(
            FrameValue, BuildPlacementContextValue, RequestedLayoutAnchorPointValue, MainBaseDepthDirectionValue,
            MainBaseLateralDirectionValue, ResolvedLayoutPlacementSlotsValue, ResolvedLayoutAnchorPointValue);
        if (!HasResolvedProductionLayoutAnchorValue)
        {
            const Point2D AlternateMainBaseLateralDirectionValue(-MainBaseLateralDirectionValue.x,
                                                                 -MainBaseLateralDirectionValue.y);
            if (TryResolveProductionLayoutAnchorPoint(FrameValue, BuildPlacementContextValue,
                                                      RequestedLayoutAnchorPointValue,
                                                      MainBaseDepthDirectionValue,
                                                      AlternateMainBaseLateralDirectionValue,
                                                      ResolvedLayoutPlacementSlotsValue,
                                                      ResolvedLayoutAnchorPointValue))
            {
                MainBaseLateralDirectionValue = AlternateMainBaseLateralDirectionValue;
                HasResolvedProductionLayoutAnchorValue = true;
            }
        }
    }

    if (HasResolvedProductionLayoutAnchorValue)
    {
        MainBaseLayoutDescriptorValue.LayoutAnchorPoint = ResolvedLayoutAnchorPointValue;
    }

    // Only populate natural entrance from discovery if the authored layout
    // did not already provide natural entrance slots. The authored path sets
    // depot + bunker slots; overwriting them loses that configuration.
    if (MainBaseLayoutDescriptorValue.NaturalEntranceWallDepotSlots.empty() &&
        MainBaseLayoutDescriptorValue.NaturalEntranceBunkerSlots.empty())
    {
        PopulateNaturalEntranceLayoutDescriptor(BuildPlacementContextValue, MainBaseLayoutDescriptorValue);
    }
    if (MainBaseLayoutDescriptorValue.NaturalEntranceArmyRallyAnchorPoint.x != 0.0f ||
        MainBaseLayoutDescriptorValue.NaturalEntranceArmyRallyAnchorPoint.y != 0.0f)
    {
        MainBaseLayoutDescriptorValue.ArmyAssemblyAnchorPoint =
            MainBaseLayoutDescriptorValue.NaturalEntranceArmyRallyAnchorPoint;
    }
    else
    {
        const Point2D ForwardDirectionValue = GetForwardDirection(BuildPlacementContextValue);
        MainBaseLayoutDescriptorValue.ArmyAssemblyAnchorPoint = ClampPointToPlayableBounds(
            BuildPlacementContextValue,
            Point2D(BuildPlacementContextValue.BaseLocation.x + (ForwardDirectionValue.x * 16.0f),
                    BuildPlacementContextValue.BaseLocation.y + (ForwardDirectionValue.y * 16.0f)));
    }

    MainBaseLayoutDescriptorValue.ProductionClearanceAnchorPoint = ClampPointToPlayableBounds(
        BuildPlacementContextValue,
        Point2D(MainBaseLayoutDescriptorValue.LayoutAnchorPoint.x + (MainBaseDepthDirectionValue.x * 8.0f),
                MainBaseLayoutDescriptorValue.LayoutAnchorPoint.y + (MainBaseDepthDirectionValue.y * 8.0f)));

    const bool AllowProductionTemplateSearchValue = !HasResolvedProductionLayoutAnchorValue;

    AppendTemplateSlotsToLayoutDescriptor(FrameValue, BuildPlacementContextValue,
                                          MainBaseLayoutDescriptorValue.LayoutAnchorPoint,
                                          MainBaseDepthDirectionValue,
                                          MainBaseLateralDirectionValue,
                                          NaturalApproachDepotOffsetValues,
                                          true,
                                          EBuildPlacementSlotType::NaturalApproachDepot,
                                          EBuildPlacementFootprintPolicy::StructureOnly,
                                          ABILITY_ID::BUILD_SUPPLYDEPOT,
                                          MainBaseLayoutDescriptorValue.NaturalApproachDepotSlots,
                                          ResolvedLayoutPlacementSlotsValue);
    AppendTemplateSlotsToLayoutDescriptor(FrameValue, BuildPlacementContextValue,
                                          MainBaseLayoutDescriptorValue.LayoutAnchorPoint,
                                          MainBaseDepthDirectionValue,
                                          MainBaseLateralDirectionValue,
                                          SupportDepotOffsetValues,
                                          true,
                                          EBuildPlacementSlotType::MainSupportDepot,
                                          EBuildPlacementFootprintPolicy::StructureOnly,
                                          ABILITY_ID::BUILD_SUPPLYDEPOT,
                                          MainBaseLayoutDescriptorValue.SupportDepotSlots,
                                          ResolvedLayoutPlacementSlotsValue);
    AppendTemplateSlotsToLayoutDescriptor(FrameValue, BuildPlacementContextValue,
                                          MainBaseLayoutDescriptorValue.LayoutAnchorPoint,
                                          MainBaseDepthDirectionValue,
                                          MainBaseLateralDirectionValue,
                                          PeripheralDepotOffsetValues,
                                          true,
                                          EBuildPlacementSlotType::MainPeripheralDepot,
                                          EBuildPlacementFootprintPolicy::StructureOnly,
                                          ABILITY_ID::BUILD_SUPPLYDEPOT,
                                          MainBaseLayoutDescriptorValue.PeripheralDepotSlots,
                                          ResolvedLayoutPlacementSlotsValue);
    const bool AllowBarracksTemplateSearchValue = AllowProductionTemplateSearchValue ||
        (MainBaseLayoutDescriptorValue.BarracksWithAddonSlots.empty() &&
         MainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.empty());
    AppendTemplateSlotsToLayoutDescriptor(FrameValue, BuildPlacementContextValue,
                                          MainBaseLayoutDescriptorValue.LayoutAnchorPoint,
                                          MainBaseDepthDirectionValue,
                                          MainBaseLateralDirectionValue,
                                          BarracksOffsetValues,
                                          AllowBarracksTemplateSearchValue,
                                          EBuildPlacementSlotType::MainBarracksWithAddon,
                                          EBuildPlacementFootprintPolicy::RequiresAddonClearance,
                                          ABILITY_ID::BUILD_BARRACKS,
                                          MainBaseLayoutDescriptorValue.BarracksWithAddonSlots,
                                          ResolvedLayoutPlacementSlotsValue);
    if (MainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.empty() &&
        BuildPlacementContextValue.RampWallDescriptor.bIsValid &&
        !MainBaseLayoutDescriptorValue.BarracksWithAddonSlots.empty())
    {
        std::vector<FBuildPlacementSlot> DerivedProductionRailSlotsValue;
        std::vector<FResolvedLayoutPlacementSlot> DerivedResolvedLayoutPlacementSlotsValue;
        FBuildPlacementSlotId SourceBarracksSlotIdValue;
        if (TryResolveDerivedProductionRailSlotGroup(FrameValue, BuildPlacementContextValue,
                                                     MainBaseLayoutDescriptorValue.BarracksWithAddonSlots,
                                                     ResolvedLayoutPlacementSlotsValue,
                                                     DerivedProductionRailSlotsValue,
                                                     DerivedResolvedLayoutPlacementSlotsValue,
                                                     SourceBarracksSlotIdValue))
        {
            MainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots = DerivedProductionRailSlotsValue;
            ResolvedLayoutPlacementSlotsValue = DerivedResolvedLayoutPlacementSlotsValue;
            (void)SourceBarracksSlotIdValue;
        }
    }
    AppendTemplateSlotsToLayoutDescriptor(FrameValue, BuildPlacementContextValue,
                                          MainBaseLayoutDescriptorValue.LayoutAnchorPoint,
                                          MainBaseDepthDirectionValue,
                                          MainBaseLateralDirectionValue,
                                          FactoryOffsetValues,
                                          AllowProductionTemplateSearchValue,
                                          EBuildPlacementSlotType::MainFactoryWithAddon,
                                          EBuildPlacementFootprintPolicy::RequiresAddonClearance,
                                          ABILITY_ID::BUILD_FACTORY,
                                          MainBaseLayoutDescriptorValue.FactoryWithAddonSlots,
                                          ResolvedLayoutPlacementSlotsValue);
    AppendTemplateSlotsToLayoutDescriptor(FrameValue, BuildPlacementContextValue,
                                          MainBaseLayoutDescriptorValue.LayoutAnchorPoint,
                                          MainBaseDepthDirectionValue,
                                          MainBaseLateralDirectionValue,
                                          StarportOffsetValues,
                                          AllowProductionTemplateSearchValue,
                                          EBuildPlacementSlotType::MainStarportWithAddon,
                                          EBuildPlacementFootprintPolicy::RequiresAddonClearance,
                                          ABILITY_ID::BUILD_STARPORT,
                                          MainBaseLayoutDescriptorValue.StarportWithAddonSlots,
                                          ResolvedLayoutPlacementSlotsValue);

    MainBaseLayoutDescriptorValue.bIsValid =
        !MainBaseLayoutDescriptorValue.NaturalEntranceWallDepotSlots.empty() ||
        !MainBaseLayoutDescriptorValue.NaturalApproachDepotSlots.empty() ||
        !MainBaseLayoutDescriptorValue.SupportDepotSlots.empty() ||
        !MainBaseLayoutDescriptorValue.PeripheralDepotSlots.empty() ||
        !MainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots.empty() ||
        !MainBaseLayoutDescriptorValue.BarracksWithAddonSlots.empty() ||
        !MainBaseLayoutDescriptorValue.FactoryWithAddonSlots.empty() ||
        !MainBaseLayoutDescriptorValue.StarportWithAddonSlots.empty();
    return MainBaseLayoutDescriptorValue;
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
    const FMainBaseLayoutDescriptor MainBaseLayoutDescriptorValue =
        BuildPlacementContextValue.MainBaseLayoutDescriptor.bIsValid
            ? BuildPlacementContextValue.MainBaseLayoutDescriptor
            : GetMainBaseLayoutDescriptor(FFrameContext(), BuildPlacementContextValue);

    if (MainBaseLayoutDescriptorValue.ArmyAssemblyAnchorPoint.x != 0.0f ||
        MainBaseLayoutDescriptorValue.ArmyAssemblyAnchorPoint.y != 0.0f)
    {
        return ClampPointToPlayableBounds(BuildPlacementContextValue,
                                          MainBaseLayoutDescriptorValue.ArmyAssemblyAnchorPoint);
    }

    if (BuildPlacementContextValue.RampWallDescriptor.bIsValid)
    {
        return ComputeNaturalEntranceAssemblyAnchorPoint(BuildPlacementContextValue);
    }

    const Point2D PrimaryAnchorValue =
        GetPrimaryStructureAnchor(GameStateDescriptorValue, BuildPlacementContextValue);
    const Point2D ForwardDirectionValue = GetForwardDirection(BuildPlacementContextValue);
    return ClampPointToPlayableBounds(BuildPlacementContextValue,
                                      Point2D(PrimaryAnchorValue.x + (ForwardDirectionValue.x * 12.0f),
                                              PrimaryAnchorValue.y + (ForwardDirectionValue.y * 12.0f)));
}

Point2D FTerranBuildPlacementService::GetProductionRallyPoint(
    const FGameStateDescriptor& GameStateDescriptorValue, const FBuildPlacementContext& BuildPlacementContextValue) const
{
    (void)GameStateDescriptorValue;

    const FMainBaseLayoutDescriptor MainBaseLayoutDescriptorValue =
        BuildPlacementContextValue.MainBaseLayoutDescriptor.bIsValid
            ? BuildPlacementContextValue.MainBaseLayoutDescriptor
            : GetMainBaseLayoutDescriptor(FFrameContext(), BuildPlacementContextValue);

    if (MainBaseLayoutDescriptorValue.NaturalEntranceArmyRallyAnchorPoint.x != 0.0f ||
        MainBaseLayoutDescriptorValue.NaturalEntranceArmyRallyAnchorPoint.y != 0.0f)
    {
        return ClampPointToPlayableBounds(BuildPlacementContextValue,
                                          MainBaseLayoutDescriptorValue.NaturalEntranceArmyRallyAnchorPoint);
    }

    return GetArmyAssemblyPoint(GameStateDescriptorValue, BuildPlacementContextValue);
}

std::vector<FBuildPlacementSlot> FTerranBuildPlacementService::GetStructurePlacementSlots(
    const FGameStateDescriptor& GameStateDescriptorValue, const ABILITY_ID StructureAbilityId,
    const FBuildPlacementContext& BuildPlacementContextValue) const
{
    const Point2D PrimaryAnchorValue = GetPrimaryStructureAnchor(GameStateDescriptorValue, BuildPlacementContextValue);
    const FRampWallDescriptor RampWallDescriptorValue =
        BuildPlacementContextValue.RampWallDescriptor.bIsValid ? BuildPlacementContextValue.RampWallDescriptor
                                                               : FRampWallDescriptor();
    const FMainBaseLayoutDescriptor MainBaseLayoutDescriptorValue =
        BuildPlacementContextValue.MainBaseLayoutDescriptor.bIsValid
            ? BuildPlacementContextValue.MainBaseLayoutDescriptor
            : GetMainBaseLayoutDescriptor(FFrameContext(), BuildPlacementContextValue);

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

            PlacementSlotsValue.insert(PlacementSlotsValue.end(),
                                       MainBaseLayoutDescriptorValue.NaturalEntranceWallDepotSlots.begin(),
                                       MainBaseLayoutDescriptorValue.NaturalEntranceWallDepotSlots.end());
            PlacementSlotsValue.insert(PlacementSlotsValue.end(),
                                       MainBaseLayoutDescriptorValue.NaturalApproachDepotSlots.begin(),
                                       MainBaseLayoutDescriptorValue.NaturalApproachDepotSlots.end());
            PlacementSlotsValue.insert(PlacementSlotsValue.end(),
                                       MainBaseLayoutDescriptorValue.SupportDepotSlots.begin(),
                                       MainBaseLayoutDescriptorValue.SupportDepotSlots.end());
            PlacementSlotsValue.insert(PlacementSlotsValue.end(),
                                       MainBaseLayoutDescriptorValue.PeripheralDepotSlots.begin(),
                                       MainBaseLayoutDescriptorValue.PeripheralDepotSlots.end());
            break;
        }
        case ABILITY_ID::BUILD_BARRACKS:
        {
            std::vector<FBuildPlacementSlot> ProtectedBarracksBuildPlacementSlotsValue;
            if (RampWallDescriptorValue.bIsValid)
            {
                PlacementSlotsValue.push_back(RampWallDescriptorValue.BarracksSlot);
                ProtectedBarracksBuildPlacementSlotsValue.push_back(RampWallDescriptorValue.BarracksSlot);
            }

            AppendPlacementSlotsAvoidingProtectedAddonFootprints(
                MainBaseLayoutDescriptorValue.BarracksWithAddonSlots,
                StructureAbilityId,
                ProtectedBarracksBuildPlacementSlotsValue,
                PlacementSlotsValue);

            ProtectedBarracksBuildPlacementSlotsValue.insert(ProtectedBarracksBuildPlacementSlotsValue.end(),
                                                             MainBaseLayoutDescriptorValue.BarracksWithAddonSlots.begin(),
                                                             MainBaseLayoutDescriptorValue.BarracksWithAddonSlots.end());
            AppendPlacementSlotsAvoidingProtectedAddonFootprints(
                MainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots,
                StructureAbilityId,
                ProtectedBarracksBuildPlacementSlotsValue,
                PlacementSlotsValue);
            break;
        }
        case ABILITY_ID::BUILD_FACTORY:
        {
            AppendPlacementSlotsAvoidingProtectedAddonFootprints(
                MainBaseLayoutDescriptorValue.FactoryWithAddonSlots,
                StructureAbilityId,
                MainBaseLayoutDescriptorValue.BarracksWithAddonSlots,
                PlacementSlotsValue);
            AppendPlacementSlotsAvoidingProtectedAddonFootprints(
                MainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots,
                StructureAbilityId,
                MainBaseLayoutDescriptorValue.BarracksWithAddonSlots,
                PlacementSlotsValue);
            break;
        }
        case ABILITY_ID::BUILD_STARPORT:
        {
            AppendPlacementSlotsAvoidingProtectedAddonFootprints(
                MainBaseLayoutDescriptorValue.StarportWithAddonSlots,
                StructureAbilityId,
                MainBaseLayoutDescriptorValue.FactoryWithAddonSlots,
                PlacementSlotsValue);
            AppendPlacementSlotsAvoidingProtectedAddonFootprints(
                MainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots,
                StructureAbilityId,
                MainBaseLayoutDescriptorValue.FactoryWithAddonSlots,
                PlacementSlotsValue);
            break;
        }
        case ABILITY_ID::BUILD_BUNKER:
        {
            PlacementSlotsValue.insert(PlacementSlotsValue.end(),
                                       MainBaseLayoutDescriptorValue.NaturalEntranceBunkerSlots.begin(),
                                       MainBaseLayoutDescriptorValue.NaturalEntranceBunkerSlots.end());
            if (PlacementSlotsValue.empty())
            {
                PlacementSlotsValue.push_back(CreatePlacementSlot(
                    EBuildPlacementSlotType::MainSupportStructure, EBuildPlacementFootprintPolicy::StructureOnly,
                    PrimaryAnchorValue, 0U));
            }
            break;
        }
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
