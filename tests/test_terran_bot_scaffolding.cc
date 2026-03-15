#include "test_terran_bot_scaffolding.h"

#include <iostream>

#include "common/armies/EArmyGoal.h"
#include "common/armies/EArmyPosture.h"
#include "common/armies/FArmyDomainState.h"
#include "common/build_orders/EOpeningPlanId.h"
#include "common/build_orders/EOpeningPlanLifecycleState.h"
#include "common/build_planning/FBuildPlanningState.h"
#include "common/descriptors/EGamePlan.h"
#include "common/descriptors/EMacroPhase.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/descriptors/FMacroStateDescriptor.h"
#include "common/planning/EIntentPlaybackState.h"
#include "common/planning/EPlanningProcessorState.h"
#include "common/spatial/FSpatialFieldSet.h"
#include "terran/terran.h"

namespace sc2
{
namespace
{

bool Check(const bool ConditionValue, bool& SuccessValue, const char* MessageValue)
{
    if (!ConditionValue)
    {
        SuccessValue = false;
        std::cerr << "    " << MessageValue << std::endl;
    }

    return ConditionValue;
}

}  // namespace

bool TestTerranBotScaffolding(int ArgC, char** ArgV)
{
    (void)ArgC;
    (void)ArgV;

    bool SuccessValue = true;

    const FMacroStateDescriptor MacroStateDescriptorValue;
    Check(MacroStateDescriptorValue.ActiveGamePlan == EGamePlan::Unknown, SuccessValue,
          "Macro state default game plan should be Unknown.");
    Check(MacroStateDescriptorValue.ActiveMacroPhase == EMacroPhase::Opening, SuccessValue,
          "Macro state default phase should be Opening.");
    Check(MacroStateDescriptorValue.CurrentGameLoop == 0U, SuccessValue,
          "Macro state should default the current game loop to zero.");
    Check(MacroStateDescriptorValue.DesiredBaseCount == 1U, SuccessValue,
          "Macro state should default to one desired base.");
    Check(MacroStateDescriptorValue.DesiredArmyCount == 1U, SuccessValue,
          "Macro state should default to one desired army.");
    Check(MacroStateDescriptorValue.WorkerCount == 0U, SuccessValue,
          "Macro state should default worker count to zero.");
    Check(MacroStateDescriptorValue.BarracksCount == 0U, SuccessValue,
          "Macro state should default barracks count to zero.");

    FArmyDomainState ArmyDomainStateValue;
    Check(ArmyDomainStateValue.MinimumArmyCount == 1U, SuccessValue,
          "Army domain should default to one minimum army.");
    Check(ArmyDomainStateValue.ActiveArmyCount == 1U, SuccessValue,
          "Army domain should always create the primary army anchor.");
    Check(ArmyDomainStateValue.ArmyGoals.size() == 1U, SuccessValue,
          "Army domain should own exactly one default army goal entry after reset.");
    Check(!ArmyDomainStateValue.ArmyGoals.empty() && ArmyDomainStateValue.ArmyGoals.front() == EArmyGoal::Unknown,
          SuccessValue, "Army domain should default the primary army goal to Unknown.");
    Check(ArmyDomainStateValue.ArmyPostures.size() == 1U, SuccessValue,
          "Army domain should own exactly one default army posture entry after reset.");
    Check(!ArmyDomainStateValue.ArmyPostures.empty() &&
              ArmyDomainStateValue.ArmyPostures.front() == EArmyPosture::Assemble,
          SuccessValue, "Army domain should default the primary army posture to Assemble.");
    Check(ArmyDomainStateValue.PrimaryArmyAttackSupplyThreshold == 40U, SuccessValue,
          "Army domain should default the attack threshold for the primary army.");

    ArmyDomainStateValue.MinimumArmyCount = 3U;
    ArmyDomainStateValue.ArmyGoals.clear();
    ArmyDomainStateValue.ArmyPostures.clear();
    ArmyDomainStateValue.ActiveArmyCount = 0U;
    ArmyDomainStateValue.EnsurePrimaryArmyExists();
    Check(ArmyDomainStateValue.ActiveArmyCount == 3U, SuccessValue,
          "Army domain should expand to the configured minimum army count.");
    Check(ArmyDomainStateValue.ArmyGoals.size() == 3U, SuccessValue,
          "Army goal storage should match the configured minimum army count.");
    Check(ArmyDomainStateValue.ArmyPostures.size() == 3U, SuccessValue,
          "Army posture storage should match the configured minimum army count.");

    FBuildPlanningState BuildPlanningStateValue;
    BuildPlanningStateValue.ActivePackageCount = 9U;
    BuildPlanningStateValue.CurrentGameLoop = 800U;
    BuildPlanningStateValue.AvailableMinerals = 300U;
    BuildPlanningStateValue.DesiredOrbitalCommandCount = 2U;
    BuildPlanningStateValue.DesiredRefineryCount = 2U;
    BuildPlanningStateValue.DesiredBarracksCount = 3U;
    BuildPlanningStateValue.DesiredFactoryTechLabCount = 1U;
    BuildPlanningStateValue.DesiredSiegeTankCount = 1U;
    BuildPlanningStateValue.ObservedTownHallCount = 2U;
    BuildPlanningStateValue.ObservedOrbitalCommandCount = 1U;
    BuildPlanningStateValue.ObservedUnitCounts[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_SCV)] = 18U;
    BuildPlanningStateValue.ObservedBuildingCounts[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_BARRACKS)] = 2U;
    BuildPlanningStateValue.ObservedBuildingsInConstruction[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_FACTORY)] = 1U;
    BuildPlanningStateValue.ReservedMinerals = 500U;
    BuildPlanningStateValue.CommittedSupply = 7U;
    BuildPlanningStateValue.Reset();
    Check(BuildPlanningStateValue.CurrentGameLoop == 0U, SuccessValue,
          "Build planning reset should clear the current game loop.");
    Check(BuildPlanningStateValue.ActivePackageCount == 0U, SuccessValue,
          "Build planning reset should clear active package count.");
    Check(BuildPlanningStateValue.AvailableMinerals == 0U, SuccessValue,
          "Build planning reset should clear available minerals.");
    Check(BuildPlanningStateValue.DesiredOrbitalCommandCount == 0U, SuccessValue,
          "Build planning reset should clear desired orbital command count.");
    Check(BuildPlanningStateValue.DesiredRefineryCount == 0U, SuccessValue,
          "Build planning reset should clear desired refinery count.");
    Check(BuildPlanningStateValue.DesiredBarracksCount == 0U, SuccessValue,
          "Build planning reset should clear desired barracks count.");
    Check(BuildPlanningStateValue.DesiredFactoryTechLabCount == 0U, SuccessValue,
          "Build planning reset should clear desired factory tech lab count.");
    Check(BuildPlanningStateValue.DesiredSiegeTankCount == 0U, SuccessValue,
          "Build planning reset should clear desired siege tank count.");
    Check(BuildPlanningStateValue.ObservedTownHallCount == 0U, SuccessValue,
          "Build planning reset should clear observed town hall count.");
    Check(BuildPlanningStateValue.ObservedOrbitalCommandCount == 0U, SuccessValue,
          "Build planning reset should clear observed orbital command count.");
    Check(BuildPlanningStateValue.ObservedUnitCounts[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_SCV)] == 0U,
          SuccessValue, "Build planning reset should clear observed unit counts.");
    Check(BuildPlanningStateValue.ObservedBuildingCounts[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_BARRACKS)] == 0U,
          SuccessValue, "Build planning reset should clear observed building counts.");
    Check(BuildPlanningStateValue.ObservedBuildingsInConstruction[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_FACTORY)] ==
              0U,
          SuccessValue, "Build planning reset should clear observed in-construction building counts.");
    Check(BuildPlanningStateValue.ReservedMinerals == 0U, SuccessValue,
          "Build planning reset should clear reserved minerals.");
    Check(BuildPlanningStateValue.CommittedSupply == 0U, SuccessValue,
          "Build planning reset should clear committed supply.");

    FSpatialFieldSet SpatialFieldSetValue;
    Check(!SpatialFieldSetValue.IsValid(), SuccessValue,
          "Spatial field set should be invalid before any resize.");
    SpatialFieldSetValue.Resize(4U, 3U);
    Check(SpatialFieldSetValue.IsValid(), SuccessValue,
          "Spatial field set should become valid after resize.");
    Check(SpatialFieldSetValue.EnemyGroundThreat.size() == 12U, SuccessValue,
          "Spatial field threat layer should match the resized field area.");
    Check(SpatialFieldSetValue.FriendlyGroundInfluence.size() == 12U, SuccessValue,
          "Spatial field friendly influence layer should match the resized field area.");
    Check(SpatialFieldSetValue.RetreatSafety.size() == 12U, SuccessValue,
          "Spatial field retreat safety layer should match the resized field area.");
    Check(SpatialFieldSetValue.EngageOpportunity.size() == 12U, SuccessValue,
          "Spatial field opportunity layer should match the resized field area.");
    SpatialFieldSetValue.Reset();
    Check(!SpatialFieldSetValue.IsValid(), SuccessValue,
          "Spatial field set reset should invalidate the field set.");

    FGameStateDescriptor GameStateDescriptorValue;
    GameStateDescriptorValue.CurrentStep = 42U;
    GameStateDescriptorValue.CurrentGameLoop = 672U;
    GameStateDescriptorValue.MacroState.DesiredBaseCount = 3U;
    GameStateDescriptorValue.MacroState.WorkerCount = 40U;
    GameStateDescriptorValue.ArmyState.MinimumArmyCount = 2U;
    GameStateDescriptorValue.ArmyState.ArmyGoals.clear();
    GameStateDescriptorValue.ArmyState.ArmyPostures.clear();
    GameStateDescriptorValue.ArmyState.ActiveArmyCount = 0U;
    GameStateDescriptorValue.BuildPlanning.AvailableVespene = 225U;
    GameStateDescriptorValue.BuildPlanning.DesiredMarineCount = 20U;
    GameStateDescriptorValue.BuildPlanning.ReservedVespene = 125U;
    GameStateDescriptorValue.OpeningPlanExecutionState.SetActivePlan(EOpeningPlanId::TerranTwoBaseMMMFrameOpening);
    GameStateDescriptorValue.OpeningPlanExecutionState.RecordSeededStep(1U, 10U);
    GameStateDescriptorValue.OpeningPlanExecutionState.MarkStepCompleted(1U);
    GameStateDescriptorValue.SpatialFields.Resize(2U, 2U);
    GameStateDescriptorValue.Reset();
    Check(GameStateDescriptorValue.CurrentStep == 0U, SuccessValue,
          "Game state descriptor reset should clear the current step.");
    Check(GameStateDescriptorValue.CurrentGameLoop == 0U, SuccessValue,
          "Game state descriptor reset should clear the current game loop.");
    Check(GameStateDescriptorValue.MacroState.DesiredBaseCount == 1U, SuccessValue,
          "Game state descriptor reset should restore macro defaults.");
    Check(GameStateDescriptorValue.MacroState.WorkerCount == 0U, SuccessValue,
          "Game state descriptor reset should clear rebuilt worker count.");
    Check(GameStateDescriptorValue.ArmyState.ActiveArmyCount == 1U, SuccessValue,
          "Game state descriptor reset should restore the primary army anchor.");
    Check(GameStateDescriptorValue.ArmyState.ArmyPostures.size() == 1U, SuccessValue,
          "Game state descriptor reset should restore the primary army posture storage.");
    Check(GameStateDescriptorValue.BuildPlanning.AvailableVespene == 0U, SuccessValue,
          "Game state descriptor reset should clear available vespene.");
    Check(GameStateDescriptorValue.BuildPlanning.DesiredMarineCount == 0U, SuccessValue,
          "Game state descriptor reset should clear desired marine count.");
    Check(GameStateDescriptorValue.BuildPlanning.ReservedVespene == 0U, SuccessValue,
          "Game state descriptor reset should clear build planning reservations.");
    Check(GameStateDescriptorValue.CommandAuthoritySchedulingState.NextOrderId == 1U, SuccessValue,
          "Game state descriptor reset should restore the scheduling order identifier seed.");
    Check(GameStateDescriptorValue.CommandAuthoritySchedulingState.ProcessorState == EPlanningProcessorState::Idle,
          SuccessValue, "Game state descriptor reset should restore the scheduling processor state.");
    Check(GameStateDescriptorValue.CommandAuthoritySchedulingState.PlaybackState == EIntentPlaybackState::Idle,
          SuccessValue, "Game state descriptor reset should restore the scheduling playback state.");
    Check(GameStateDescriptorValue.OpeningPlanExecutionState.ActivePlanId == EOpeningPlanId::Unknown, SuccessValue,
          "Game state descriptor reset should clear the active opening plan identifier.");
    Check(GameStateDescriptorValue.OpeningPlanExecutionState.LifecycleState ==
              EOpeningPlanLifecycleState::Uninitialized,
          SuccessValue, "Game state descriptor reset should restore the opening-plan lifecycle state.");
    Check(GameStateDescriptorValue.OpeningPlanExecutionState.SeededStepIds.empty(), SuccessValue,
          "Game state descriptor reset should clear seeded opening-plan steps.");
    Check(GameStateDescriptorValue.OpeningPlanExecutionState.CompletedStepIds.empty(), SuccessValue,
          "Game state descriptor reset should clear completed opening-plan steps.");
    Check(!GameStateDescriptorValue.SpatialFields.IsValid(), SuccessValue,
          "Game state descriptor reset should clear spatial fields.");

    Check(std::string(ToString(EGamePlan::TimingAttack)) == "TimingAttack", SuccessValue,
          "Game plan string conversion should expose TimingAttack.");
    Check(std::string(ToString(EMacroPhase::MidGame)) == "MidGame", SuccessValue,
          "Macro phase string conversion should expose MidGame.");
    Check(std::string(ToString(EArmyGoal::HoldBase)) == "HoldBase", SuccessValue,
          "Army goal string conversion should expose HoldBase.");
    Check(std::string(ToString(EArmyPosture::Engage)) == "Engage", SuccessValue,
          "Army posture string conversion should expose Engage.");

    TerranAgent TerranAgentValue;
    Unit ScvUnitValue;
    ScvUnitValue.unit_type = UNIT_TYPEID::TERRAN_SCV;
    UnitOrder BarracksBuildOrderValue;
    BarracksBuildOrderValue.ability_id = ABILITY_ID::BUILD_BARRACKS;
    ScvUnitValue.orders.push_back(BarracksBuildOrderValue);

    FCommandOrderRecord BarracksConstructionOrderValue = FCommandOrderRecord::CreatePointTarget(
        ECommandAuthorityLayer::UnitExecution, 4001U, ABILITY_ID::BUILD_BARRACKS, Point2D(30.0f, 30.0f), 150,
        EIntentDomain::StructureBuild, 100U);
    BarracksConstructionOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
    BarracksConstructionOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKS;

    Check(!TerranAgentValue.HasProducerConfirmedDispatchedOrder(BarracksConstructionOrderValue, &ScvUnitValue),
          SuccessValue,
          "Structure construction should not count as confirmed merely because the worker still carries the build order.");

    Unit BarracksUnitValue;
    BarracksUnitValue.unit_type = UNIT_TYPEID::TERRAN_BARRACKS;
    BarracksUnitValue.add_on_tag = 9001U;
    FCommandOrderRecord AddonConstructionOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::UnitExecution, 4002U, ABILITY_ID::BUILD_REACTOR_BARRACKS, 150,
        EIntentDomain::StructureBuild, 100U);
    AddonConstructionOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKS;
    AddonConstructionOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKSREACTOR;

    Check(TerranAgentValue.HasProducerConfirmedDispatchedOrder(AddonConstructionOrderValue, &BarracksUnitValue),
          SuccessValue, "Add-on construction should still confirm from the producer once the add-on is attached.");

    return SuccessValue;
}

}  // namespace sc2
