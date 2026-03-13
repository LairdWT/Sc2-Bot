#include "common/services/FTerranBuildPlacementService.h"

#include <array>
#include <cmath>

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
        FBuildPlacementSlot BuildPlacementSlotValue;
        BuildPlacementSlotValue.SlotType = BuildPlacementSlotTypeValue;
        BuildPlacementSlotValue.FootprintPolicy = BuildPlacementFootprintPolicyValue;
        BuildPlacementSlotValue.BuildPoint =
            ProjectOffsetToWorld(AnchorValue, ForwardDirectionValue, LateralDirectionValue, PlacementOffsetValue);
        OutPlacementSlotsValue.push_back(BuildPlacementSlotValue);
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

}  // namespace

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

    static const std::array<FPlacementOffsetDescriptor, 1> MainRampDepotLeftOffsetValues =
    {{
        {3.0f, 1.0f},
    }};

    static const std::array<FPlacementOffsetDescriptor, 1> MainRampDepotRightOffsetValues =
    {{
        {-3.0f, 1.0f},
    }};

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

    static const std::array<FPlacementOffsetDescriptor, 1> MainRampBarracksOffsetValues =
    {{
        {0.0f, 5.0f},
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

    std::vector<FBuildPlacementSlot> PlacementSlotsValue;

    switch (StructureAbilityId)
    {
        case ABILITY_ID::BUILD_SUPPLYDEPOT:
        {
            const uint32_t CurrentSupplyDepotCountValue =
                GetObservedAndInConstructionBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_SUPPLYDEPOT);

            if (CurrentSupplyDepotCountValue == 0U)
            {
                AppendPlacementSlots(PrimaryAnchorValue, ForwardDirectionValue, LateralDirectionValue,
                                     MainRampDepotLeftOffsetValues, EBuildPlacementSlotType::MainRampDepotLeft,
                                     EBuildPlacementFootprintPolicy::StructureOnly, PlacementSlotsValue);
                AppendPlacementSlots(PrimaryAnchorValue, ForwardDirectionValue, LateralDirectionValue,
                                     MainRampDepotRightOffsetValues, EBuildPlacementSlotType::MainRampDepotRight,
                                     EBuildPlacementFootprintPolicy::StructureOnly, PlacementSlotsValue);
                AppendPlacementSlots(PrimaryAnchorValue, ForwardDirectionValue, LateralDirectionValue,
                                     NaturalApproachDepotOffsetValues, EBuildPlacementSlotType::NaturalApproachDepot,
                                     EBuildPlacementFootprintPolicy::StructureOnly, PlacementSlotsValue);
            }
            else if (CurrentSupplyDepotCountValue == 1U)
            {
                AppendPlacementSlots(PrimaryAnchorValue, ForwardDirectionValue, LateralDirectionValue,
                                     MainRampDepotRightOffsetValues, EBuildPlacementSlotType::MainRampDepotRight,
                                     EBuildPlacementFootprintPolicy::StructureOnly, PlacementSlotsValue);
                AppendPlacementSlots(PrimaryAnchorValue, ForwardDirectionValue, LateralDirectionValue,
                                     MainRampDepotLeftOffsetValues, EBuildPlacementSlotType::MainRampDepotLeft,
                                     EBuildPlacementFootprintPolicy::StructureOnly, PlacementSlotsValue);
                AppendPlacementSlots(PrimaryAnchorValue, ForwardDirectionValue, LateralDirectionValue,
                                     NaturalApproachDepotOffsetValues, EBuildPlacementSlotType::NaturalApproachDepot,
                                     EBuildPlacementFootprintPolicy::StructureOnly, PlacementSlotsValue);
            }

            AppendPlacementSlots(PrimaryAnchorValue, ForwardDirectionValue, LateralDirectionValue,
                                 SupportDepotOffsetValues, EBuildPlacementSlotType::MainSupportStructure,
                                 EBuildPlacementFootprintPolicy::StructureOnly, PlacementSlotsValue);
            break;
        }
        case ABILITY_ID::BUILD_BARRACKS:
        {
            const uint32_t CurrentBarracksCountValue =
                GetObservedAndInConstructionBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_BARRACKS);

            if (CurrentBarracksCountValue == 0U)
            {
                AppendPlacementSlots(PrimaryAnchorValue, ForwardDirectionValue, LateralDirectionValue,
                                     MainRampBarracksOffsetValues,
                                     EBuildPlacementSlotType::MainRampBarracksWithAddon,
                                     EBuildPlacementFootprintPolicy::RequiresAddonClearance, PlacementSlotsValue);
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
            FBuildPlacementSlot BuildPlacementSlotValue;
            BuildPlacementSlotValue.SlotType = EBuildPlacementSlotType::MainSupportStructure;
            BuildPlacementSlotValue.FootprintPolicy = EBuildPlacementFootprintPolicy::StructureOnly;
            BuildPlacementSlotValue.BuildPoint = PrimaryAnchorValue;
            PlacementSlotsValue.push_back(BuildPlacementSlotValue);
            break;
        }
    }

    return PlacementSlotsValue;
}

}  // namespace sc2
