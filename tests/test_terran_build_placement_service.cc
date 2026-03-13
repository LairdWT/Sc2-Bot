#include "test_terran_build_placement_service.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "common/services/EBuildPlacementFootprintPolicy.h"
#include "common/services/EBuildPlacementSlotType.h"
#include "common/services/FBuildPlacementContext.h"
#include "common/services/FBuildPlacementSlot.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/services/FTerranBuildPlacementService.h"

namespace sc2
{
namespace
{

bool Check(const bool ConditionValue, bool& SuccessValue, const std::string& MessageValue)
{
    if (!ConditionValue)
    {
        SuccessValue = false;
        std::cerr << "    " << MessageValue << std::endl;
    }

    return ConditionValue;
}

bool ArePointsEqual(const Point2D& LeftValue, const Point2D& RightValue)
{
    constexpr float CoordinateToleranceValue = 0.001f;
    return std::fabs(LeftValue.x - RightValue.x) <= CoordinateToleranceValue &&
           std::fabs(LeftValue.y - RightValue.y) <= CoordinateToleranceValue;
}

bool ContainsSlot(const std::vector<FBuildPlacementSlot>& BuildPlacementSlotsValue,
                  const EBuildPlacementSlotType BuildPlacementSlotTypeValue,
                  const EBuildPlacementFootprintPolicy BuildPlacementFootprintPolicyValue,
                  const Point2D& BuildPointValue)
{
    for (const FBuildPlacementSlot& BuildPlacementSlotValue : BuildPlacementSlotsValue)
    {
        if (BuildPlacementSlotValue.SlotId.SlotType == BuildPlacementSlotTypeValue &&
            BuildPlacementSlotValue.FootprintPolicy == BuildPlacementFootprintPolicyValue &&
            ArePointsEqual(BuildPlacementSlotValue.BuildPoint, BuildPointValue))
        {
            return true;
        }
    }

    return false;
}

}  // namespace

bool TestTerranBuildPlacementService(int ArgC, char** ArgV)
{
    (void)ArgC;
    (void)ArgV;

    bool SuccessValue = true;

    FGameStateDescriptor GameStateDescriptorValue;
    FBuildPlacementContext BuildPlacementContextValue;
    BuildPlacementContextValue.BaseLocation = Point2D(50.0f, 50.0f);
    BuildPlacementContextValue.NaturalLocation = Point2D(70.0f, 50.0f);
    FTerranBuildPlacementService BuildPlacementServiceValue;
    BuildPlacementContextValue.RampWallDescriptor =
        BuildPlacementServiceValue.GetRampWallDescriptor(FFrameContext(), BuildPlacementContextValue);

    const Point2D PrimaryAnchorValue =
        BuildPlacementServiceValue.GetPrimaryStructureAnchor(GameStateDescriptorValue, BuildPlacementContextValue);
    Check(ArePointsEqual(PrimaryAnchorValue, Point2D(56.0f, 50.0f)), SuccessValue,
          "Primary structure anchor should bias forward toward the natural approach.");
    Check(BuildPlacementContextValue.RampWallDescriptor.bIsValid, SuccessValue,
          "Placement context should cache a valid ramp-wall descriptor when the natural direction is known.");
    Check(ArePointsEqual(BuildPlacementContextValue.RampWallDescriptor.InsideStagingPoint, Point2D(54.5f, 50.0f)),
          SuccessValue, "Ramp-wall discovery should publish the inside staging point.");
    Check(ArePointsEqual(BuildPlacementContextValue.RampWallDescriptor.OutsideStagingPoint, Point2D(64.5f, 50.0f)),
          SuccessValue, "Ramp-wall discovery should publish the outside staging point.");
    const Point2D ArmyAssemblyPointValue =
        BuildPlacementServiceValue.GetArmyAssemblyPoint(GameStateDescriptorValue, BuildPlacementContextValue);
    Check(ArePointsEqual(ArmyAssemblyPointValue, BuildPlacementContextValue.RampWallDescriptor.OutsideStagingPoint),
          SuccessValue, "Army assembly should use the ramp-wall outside staging point when the wall is valid.");

    const std::vector<FBuildPlacementSlot> SupplyDepotSlotsValue =
        BuildPlacementServiceValue.GetStructurePlacementSlots(GameStateDescriptorValue,
                                                              ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                              BuildPlacementContextValue);
    Check(SupplyDepotSlotsValue.size() == 10U, SuccessValue,
          "Supply depot placement should expose ramp, natural-approach, and support slots.");
    Check(!SupplyDepotSlotsValue.empty() &&
              SupplyDepotSlotsValue.front().SlotId.SlotType == EBuildPlacementSlotType::MainRampDepotLeft,
          SuccessValue, "The first supply depot should prefer the main-ramp depot slot.");
    Check(!SupplyDepotSlotsValue.empty() &&
              SupplyDepotSlotsValue.front().FootprintPolicy == EBuildPlacementFootprintPolicy::StructureOnly,
          SuccessValue, "Supply depot slots should not reserve addon clearance.");
    Check(ContainsSlot(SupplyDepotSlotsValue, EBuildPlacementSlotType::MainRampDepotLeft,
                       EBuildPlacementFootprintPolicy::StructureOnly, Point2D(57.0f, 53.0f)),
          SuccessValue, "Supply depot placement should expose the left ramp wall slot from the descriptor.");
    Check(ContainsSlot(SupplyDepotSlotsValue, EBuildPlacementSlotType::MainRampDepotRight,
                       EBuildPlacementFootprintPolicy::StructureOnly, Point2D(57.0f, 47.0f)),
          SuccessValue, "Supply depot placement should expose the opposite ramp depot slot for the wall.");
    Check(ContainsSlot(SupplyDepotSlotsValue, EBuildPlacementSlotType::NaturalApproachDepot,
                       EBuildPlacementFootprintPolicy::StructureOnly, Point2D(62.0f, 56.0f)),
          SuccessValue, "Supply depot placement should include a natural-approach depot slot.");

    GameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
        UNIT_TYPEID::TERRAN_SUPPLYDEPOT)] = 1U;
    const std::vector<FBuildPlacementSlot> SecondSupplyDepotSlotsValue =
        BuildPlacementServiceValue.GetStructurePlacementSlots(GameStateDescriptorValue,
                                                              ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                              BuildPlacementContextValue);
    Check(!SecondSupplyDepotSlotsValue.empty() &&
              SecondSupplyDepotSlotsValue.front().SlotId.SlotType == EBuildPlacementSlotType::MainRampDepotLeft,
          SuccessValue, "Observed depot counts should not reorder descriptor-backed wall slots.");
    Check(SecondSupplyDepotSlotsValue.size() >= 2U &&
              SecondSupplyDepotSlotsValue[1].SlotId.SlotType == EBuildPlacementSlotType::MainRampDepotRight,
          SuccessValue, "Supply depot placement should keep both ramp-wall slots ordered ahead of support slots.");
    GameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
        UNIT_TYPEID::TERRAN_SUPPLYDEPOT)] = 0U;

    const std::vector<FBuildPlacementSlot> BarracksSlotsValue =
        BuildPlacementServiceValue.GetStructurePlacementSlots(GameStateDescriptorValue,
                                                              ABILITY_ID::BUILD_BARRACKS,
                                                              BuildPlacementContextValue);
    Check(BarracksSlotsValue.size() == 7U, SuccessValue,
          "The first barracks should expose one wall barracks option plus addon-safe production slots.");
    Check(!BarracksSlotsValue.empty() &&
              BarracksSlotsValue.front().SlotId.SlotType == EBuildPlacementSlotType::MainRampBarracksWithAddon,
          SuccessValue, "The first barracks should prefer the ramp-facing barracks slot.");
    Check(!BarracksSlotsValue.empty() &&
              BarracksSlotsValue.front().FootprintPolicy ==
                  EBuildPlacementFootprintPolicy::RequiresAddonClearance,
          SuccessValue, "Barracks slots should require addon clearance.");
    Check(ContainsSlot(BarracksSlotsValue, EBuildPlacementSlotType::MainRampBarracksWithAddon,
                       EBuildPlacementFootprintPolicy::RequiresAddonClearance, Point2D(61.0f, 50.0f)),
          SuccessValue, "Barracks placement should expose the center wall slot.");
    Check(ContainsSlot(BarracksSlotsValue, EBuildPlacementSlotType::MainProductionWithAddon,
                       EBuildPlacementFootprintPolicy::RequiresAddonClearance, Point2D(70.0f, 58.0f)),
          SuccessValue, "Barracks placement should include addon-safe interior production slots.");

    GameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
        UNIT_TYPEID::TERRAN_BARRACKS)] = 1U;
    const std::vector<FBuildPlacementSlot> FollowUpBarracksSlotsValue =
        BuildPlacementServiceValue.GetStructurePlacementSlots(GameStateDescriptorValue,
                                                              ABILITY_ID::BUILD_BARRACKS,
                                                              BuildPlacementContextValue);
    Check(FollowUpBarracksSlotsValue.size() == 7U, SuccessValue,
          "Barracks placement should keep the descriptor-backed wall slot in the ordered placement list.");
    Check(!FollowUpBarracksSlotsValue.empty() &&
              FollowUpBarracksSlotsValue.front().SlotId.SlotType ==
                  EBuildPlacementSlotType::MainRampBarracksWithAddon,
          SuccessValue, "Observed barracks counts should not hide the descriptor-backed ramp slot.");

    const std::vector<FBuildPlacementSlot> FactorySlotsValue =
        BuildPlacementServiceValue.GetStructurePlacementSlots(GameStateDescriptorValue,
                                                              ABILITY_ID::BUILD_FACTORY,
                                                              BuildPlacementContextValue);
    Check(FactorySlotsValue.size() == 6U, SuccessValue,
          "Factory placement should use the addon-safe production grid.");
    Check(!FactorySlotsValue.empty() &&
              FactorySlotsValue.front().FootprintPolicy ==
                  EBuildPlacementFootprintPolicy::RequiresAddonClearance,
          SuccessValue, "Factory slots should require addon clearance.");
    Check(!ContainsSlot(FactorySlotsValue, EBuildPlacementSlotType::MainRampBarracksWithAddon,
                        EBuildPlacementFootprintPolicy::RequiresAddonClearance, Point2D(61.0f, 50.0f)),
          SuccessValue, "Factory placement should exclude the dedicated wall barracks slot.");

    const std::vector<FBuildPlacementSlot> FallbackSlotsValue =
        BuildPlacementServiceValue.GetStructurePlacementSlots(GameStateDescriptorValue,
                                                              ABILITY_ID::INVALID,
                                                              BuildPlacementContextValue);
    Check(FallbackSlotsValue.size() == 1U, SuccessValue,
          "Unsupported structures should still produce one deterministic support slot.");
    Check(!FallbackSlotsValue.empty() &&
              FallbackSlotsValue.front().SlotId.SlotType == EBuildPlacementSlotType::MainSupportStructure,
          SuccessValue, "Unsupported structures should map to the support-structure slot type.");
    Check(!FallbackSlotsValue.empty() &&
              ArePointsEqual(FallbackSlotsValue.front().BuildPoint, PrimaryAnchorValue),
          SuccessValue, "Unsupported structures should fall back to the primary anchor.");

    return SuccessValue;
}

}  // namespace sc2
