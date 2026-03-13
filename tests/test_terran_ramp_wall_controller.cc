#include "test_terran_ramp_wall_controller.h"

#include <iostream>
#include <string>
#include <vector>

#include "common/agent_framework.h"
#include "common/planning/FTerranRampWallController.h"
#include "common/services/EBuildPlacementFootprintPolicy.h"
#include "common/services/EBuildPlacementSlotType.h"

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

Unit MakeUnit(const Tag TagValue, const UNIT_TYPEID UnitTypeIdValue, const Unit::Alliance AllianceValue,
              const Point2D& PositionValue, const bool IsBuildingValue)
{
    Unit UnitValue;
    UnitValue.display_type = Unit::Visible;
    UnitValue.alliance = AllianceValue;
    UnitValue.tag = TagValue;
    UnitValue.unit_type = UnitTypeIdValue;
    UnitValue.owner = AllianceValue == Unit::Alliance::Self ? 1 : 2;
    UnitValue.pos = Point3D(PositionValue.x, PositionValue.y, 0.0f);
    UnitValue.facing = 0.0f;
    UnitValue.radius = 0.5f;
    UnitValue.build_progress = 1.0f;
    UnitValue.cloak = Unit::NotCloaked;
    UnitValue.detect_range = 0.0f;
    UnitValue.radar_range = 0.0f;
    UnitValue.is_selected = false;
    UnitValue.is_on_screen = true;
    UnitValue.is_blip = false;
    UnitValue.health = 45.0f;
    UnitValue.health_max = 45.0f;
    UnitValue.shield = 0.0f;
    UnitValue.shield_max = 0.0f;
    UnitValue.energy = 0.0f;
    UnitValue.energy_max = 0.0f;
    UnitValue.mineral_contents = 0;
    UnitValue.vespene_contents = 0;
    UnitValue.is_flying = false;
    UnitValue.is_burrowed = false;
    UnitValue.is_hallucination = false;
    UnitValue.weapon_cooldown = 0.0f;
    UnitValue.add_on_tag = NullTag;
    UnitValue.cargo_space_taken = 0;
    UnitValue.cargo_space_max = 0;
    UnitValue.assigned_harvesters = 0;
    UnitValue.ideal_harvesters = 0;
    UnitValue.engaged_target_tag = NullTag;
    UnitValue.is_powered = true;
    UnitValue.is_alive = true;
    UnitValue.last_seen_game_loop = 0;
    UnitValue.attack_upgrade_level = 0;
    UnitValue.armor_upgrade_level = 0;
    UnitValue.shield_upgrade_level = 0;
    UnitValue.is_building = IsBuildingValue;
    return UnitValue;
}

void AddOrder(Unit& UnitValue, const ABILITY_ID AbilityIdValue)
{
    UnitOrder UnitOrderValue;
    UnitOrderValue.ability_id = AbilityIdValue;
    UnitValue.orders.push_back(UnitOrderValue);
}

void AppendUnitPointers(const std::vector<Unit>& UnitStorageValue, Units& OutUnitsValue)
{
    OutUnitsValue.clear();
    OutUnitsValue.reserve(UnitStorageValue.size());
    for (const Unit& UnitValue : UnitStorageValue)
    {
        OutUnitsValue.push_back(&UnitValue);
    }
}

FRampWallDescriptor CreateRampWallDescriptor()
{
    FRampWallDescriptor RampWallDescriptorValue;
    RampWallDescriptorValue.bIsValid = true;
    RampWallDescriptorValue.WallCenterPoint = Point2D(10.0f, 10.0f);
    RampWallDescriptorValue.InsideStagingPoint = Point2D(8.0f, 10.0f);
    RampWallDescriptorValue.OutsideStagingPoint = Point2D(12.0f, 10.0f);

    RampWallDescriptorValue.LeftDepotSlot.SlotId.SlotType = EBuildPlacementSlotType::MainRampDepotLeft;
    RampWallDescriptorValue.LeftDepotSlot.SlotId.Ordinal = 0U;
    RampWallDescriptorValue.LeftDepotSlot.FootprintPolicy = EBuildPlacementFootprintPolicy::StructureOnly;
    RampWallDescriptorValue.LeftDepotSlot.BuildPoint = Point2D(10.0f, 11.5f);

    RampWallDescriptorValue.BarracksSlot.SlotId.SlotType = EBuildPlacementSlotType::MainRampBarracksWithAddon;
    RampWallDescriptorValue.BarracksSlot.SlotId.Ordinal = 0U;
    RampWallDescriptorValue.BarracksSlot.FootprintPolicy =
        EBuildPlacementFootprintPolicy::RequiresAddonClearance;
    RampWallDescriptorValue.BarracksSlot.BuildPoint = Point2D(10.0f, 10.0f);

    RampWallDescriptorValue.RightDepotSlot.SlotId.SlotType = EBuildPlacementSlotType::MainRampDepotRight;
    RampWallDescriptorValue.RightDepotSlot.SlotId.Ordinal = 0U;
    RampWallDescriptorValue.RightDepotSlot.FootprintPolicy = EBuildPlacementFootprintPolicy::StructureOnly;
    RampWallDescriptorValue.RightDepotSlot.BuildPoint = Point2D(10.0f, 8.5f);
    return RampWallDescriptorValue;
}

bool DoesIntentBufferContainOnlyAbility(const FIntentBuffer& IntentBufferValue, const ABILITY_ID AbilityIdValue)
{
    for (const FUnitIntent& IntentValue : IntentBufferValue.Intents)
    {
        if (IntentValue.Ability != AbilityIdValue || IntentValue.Domain != EIntentDomain::StructureControl)
        {
            return false;
        }
    }

    return true;
}

}  // namespace

bool TestTerranRampWallController(int ArgC, char** ArgV)
{
    (void)ArgC;
    (void)ArgV;

    bool SuccessValue = true;

    const FRampWallDescriptor RampWallDescriptorValue = CreateRampWallDescriptor();
    FTerranRampWallController TerranRampWallControllerValue;

    std::vector<Unit> RaisedDepotStorageValue;
    RaisedDepotStorageValue.push_back(MakeUnit(101U, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, Unit::Alliance::Self,
                                               RampWallDescriptorValue.LeftDepotSlot.BuildPoint, true));
    RaisedDepotStorageValue.push_back(MakeUnit(102U, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, Unit::Alliance::Self,
                                               RampWallDescriptorValue.RightDepotSlot.BuildPoint, true));
    Units RaisedDepotUnitsValue;
    AppendUnitPointers(RaisedDepotStorageValue, RaisedDepotUnitsValue);
    const Units EmptyEnemyUnitsValue;

    const EWallGateState OpenWallGateStateValue = TerranRampWallControllerValue.EvaluateDesiredWallGateState(
        RaisedDepotUnitsValue, EmptyEnemyUnitsValue, RampWallDescriptorValue);
    Check(OpenWallGateStateValue == EWallGateState::Open, SuccessValue,
          "Completed wall depots without a threat should keep the wall open.");

    FIntentBuffer LowerIntentBufferValue;
    TerranRampWallControllerValue.ProduceWallGateIntents(RaisedDepotUnitsValue, RampWallDescriptorValue,
                                                         OpenWallGateStateValue, LowerIntentBufferValue);
    Check(LowerIntentBufferValue.Intents.size() == 2U, SuccessValue,
          "Open wall state should lower both completed wall depots.");
    Check(DoesIntentBufferContainOnlyAbility(LowerIntentBufferValue, ABILITY_ID::MORPH_SUPPLYDEPOT_LOWER),
          SuccessValue, "Open wall state should only emit lower-depot intents in the structure-control domain.");

    AddOrder(RaisedDepotStorageValue[0], ABILITY_ID::MORPH_SUPPLYDEPOT_LOWER);
    AddOrder(RaisedDepotStorageValue[1], ABILITY_ID::MORPH_SUPPLYDEPOT_LOWER);
    FIntentBuffer DuplicateLowerIntentBufferValue;
    TerranRampWallControllerValue.ProduceWallGateIntents(RaisedDepotUnitsValue, RampWallDescriptorValue,
                                                         OpenWallGateStateValue, DuplicateLowerIntentBufferValue);
    Check(DuplicateLowerIntentBufferValue.Intents.empty(), SuccessValue,
          "Repeated open-wall passes should not spam lower orders while matching morph orders are already present.");

    std::vector<Unit> LoweredDepotStorageValue;
    LoweredDepotStorageValue.push_back(MakeUnit(201U, UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED, Unit::Alliance::Self,
                                                RampWallDescriptorValue.LeftDepotSlot.BuildPoint, true));
    LoweredDepotStorageValue.push_back(MakeUnit(202U, UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED, Unit::Alliance::Self,
                                                RampWallDescriptorValue.RightDepotSlot.BuildPoint, true));
    Units LoweredDepotUnitsValue;
    AppendUnitPointers(LoweredDepotStorageValue, LoweredDepotUnitsValue);

    std::vector<Unit> OutsideThreatStorageValue;
    OutsideThreatStorageValue.push_back(MakeUnit(301U, UNIT_TYPEID::TERRAN_MARINE, Unit::Alliance::Enemy,
                                                 RampWallDescriptorValue.OutsideStagingPoint, false));
    Units OutsideThreatUnitsValue;
    AppendUnitPointers(OutsideThreatStorageValue, OutsideThreatUnitsValue);

    const EWallGateState ClosedWallGateStateValue = TerranRampWallControllerValue.EvaluateDesiredWallGateState(
        LoweredDepotUnitsValue, OutsideThreatUnitsValue, RampWallDescriptorValue);
    Check(ClosedWallGateStateValue == EWallGateState::Closed, SuccessValue,
          "Enemy ground pressure near the outside staging point should close the wall.");

    FIntentBuffer RaiseIntentBufferValue;
    TerranRampWallControllerValue.ProduceWallGateIntents(LoweredDepotUnitsValue, RampWallDescriptorValue,
                                                         ClosedWallGateStateValue, RaiseIntentBufferValue);
    Check(RaiseIntentBufferValue.Intents.size() == 2U, SuccessValue,
          "Closed wall state should raise both wall depots.");
    Check(DoesIntentBufferContainOnlyAbility(RaiseIntentBufferValue, ABILITY_ID::MORPH_SUPPLYDEPOT_RAISE),
          SuccessValue, "Closed wall state should only emit raise-depot intents in the structure-control domain.");

    std::vector<Unit> InsideThreatStorageValue;
    InsideThreatStorageValue.push_back(MakeUnit(302U, UNIT_TYPEID::TERRAN_MARINE, Unit::Alliance::Enemy,
                                                Point2D(7.0f, 10.0f), false));
    Units InsideThreatUnitsValue;
    AppendUnitPointers(InsideThreatStorageValue, InsideThreatUnitsValue);
    Check(TerranRampWallControllerValue.EvaluateDesiredWallGateState(LoweredDepotUnitsValue, InsideThreatUnitsValue,
                                                                     RampWallDescriptorValue) ==
              EWallGateState::Closed,
          SuccessValue, "Enemy ground units already on the main-base side of the wall should close the wall.");

    std::vector<Unit> IncompleteWallStorageValue;
    IncompleteWallStorageValue.push_back(MakeUnit(401U, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, Unit::Alliance::Self,
                                                  RampWallDescriptorValue.LeftDepotSlot.BuildPoint, true));
    Units IncompleteWallUnitsValue;
    AppendUnitPointers(IncompleteWallStorageValue, IncompleteWallUnitsValue);
    Check(TerranRampWallControllerValue.EvaluateDesiredWallGateState(IncompleteWallUnitsValue, EmptyEnemyUnitsValue,
                                                                     RampWallDescriptorValue) ==
              EWallGateState::Unavailable,
          SuccessValue, "Missing wall depots should make the wall controller unavailable.");
    FIntentBuffer UnavailableIntentBufferValue;
    TerranRampWallControllerValue.ProduceWallGateIntents(IncompleteWallUnitsValue, RampWallDescriptorValue,
                                                         EWallGateState::Unavailable, UnavailableIntentBufferValue);
    Check(UnavailableIntentBufferValue.Intents.empty(), SuccessValue,
          "Unavailable wall state should not emit invalid depot morph orders.");

    return SuccessValue;
}

}  // namespace sc2
