#include "test_terran_descriptor_pipeline.h"

#include <iostream>
#include <string>
#include <vector>

#include "common/armies/EArmyGoal.h"
#include "common/bot_status_models.h"
#include "common/descriptors/EGamePlan.h"
#include "common/descriptors/EMacroPhase.h"
#include "common/descriptors/FTerranForecastStateBuilder.h"
#include "common/descriptors/FTerranGameStateDescriptorBuilder.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/economy/EconomyForecastConstants.h"
#include "common/economy/FEconomyDomainState.h"
#include "FTestUnitFactory.h"
#include "common/goals/EGoalStatus.h"
#include "common/goals/EGoalType.h"
#include "common/goals/FGoalDescriptor.h"
#include "common/planning/FDefaultStrategicDirector.h"
#include "common/planning/FCommandOrderRecord.h"

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

bool HasGoalOfType(const std::vector<FGoalDescriptor>& GoalDescriptorsValue, const EGoalType GoalTypeValue,
                   const EGoalStatus GoalStatusValue)
{
    for (const FGoalDescriptor& GoalDescriptorValue : GoalDescriptorsValue)
    {
        if (GoalDescriptorValue.GoalType == GoalTypeValue && GoalDescriptorValue.GoalStatus == GoalStatusValue)
        {
            return true;
        }
    }

    return false;
}

bool HasGoalOfTypeForTarget(const std::vector<FGoalDescriptor>& GoalDescriptorsValue, const EGoalType GoalTypeValue,
                            const EGoalStatus GoalStatusValue, const UNIT_TYPEID TargetUnitTypeIdValue)
{
    for (const FGoalDescriptor& GoalDescriptorValue : GoalDescriptorsValue)
    {
        if (GoalDescriptorValue.GoalType == GoalTypeValue && GoalDescriptorValue.GoalStatus == GoalStatusValue &&
            GoalDescriptorValue.TargetUnitTypeId == TargetUnitTypeIdValue)
        {
            return true;
        }
    }

    return false;
}

void ConfigureOpeningState(FAgentState& AgentStateValue)
{
    AgentStateValue.Economy.Minerals = 150U;
    AgentStateValue.Economy.Vespene = 0U;
    AgentStateValue.Economy.Supply = 24U;
    AgentStateValue.Economy.SupplyCap = 31U;
    AgentStateValue.Economy.SupplyAvailable = 7U;

    AgentStateValue.Units.SetUnitCount(UNIT_TYPEID::TERRAN_SCV, 16U);
    AgentStateValue.Units.SetUnitCount(UNIT_TYPEID::TERRAN_MARINE, 8U);
    AgentStateValue.Units.Update();

    AgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_COMMANDCENTER, 1U);
    AgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 2U);
    AgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_BARRACKS, 1U);
}

void ConfigureTimingAttackState(FAgentState& AgentStateValue)
{
    AgentStateValue.Economy.Minerals = 320U;
    AgentStateValue.Economy.Vespene = 170U;
    AgentStateValue.Economy.Supply = 64U;
    AgentStateValue.Economy.SupplyCap = 86U;
    AgentStateValue.Economy.SupplyAvailable = 22U;

    AgentStateValue.Units.SetUnitCount(UNIT_TYPEID::TERRAN_SCV, 32U);
    AgentStateValue.Units.SetUnitCount(UNIT_TYPEID::TERRAN_MARINE, 24U);
    AgentStateValue.Units.SetUnitCount(UNIT_TYPEID::TERRAN_MARAUDER, 6U);
    AgentStateValue.Units.SetUnitCount(UNIT_TYPEID::TERRAN_MEDIVAC, 2U);
    AgentStateValue.Units.Update();

    AgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_COMMANDCENTER, 2U);
    AgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_BARRACKS, 3U);
    AgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_FACTORY, 1U);
    AgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_STARPORT, 1U);
}

void ConfigureMacroState(FAgentState& AgentStateValue)
{
    AgentStateValue.Economy.Minerals = 780U;
    AgentStateValue.Economy.Vespene = 420U;
    AgentStateValue.Economy.Supply = 170U;
    AgentStateValue.Economy.SupplyCap = 200U;
    AgentStateValue.Economy.SupplyAvailable = 30U;

    AgentStateValue.Units.SetUnitCount(UNIT_TYPEID::TERRAN_SCV, 66U);
    AgentStateValue.Units.SetUnitCount(UNIT_TYPEID::TERRAN_MARINE, 60U);
    AgentStateValue.Units.SetUnitCount(UNIT_TYPEID::TERRAN_MARAUDER, 12U);
    AgentStateValue.Units.SetUnitCount(UNIT_TYPEID::TERRAN_MEDIVAC, 8U);
    AgentStateValue.Units.Update();

    AgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_COMMANDCENTER, 3U);
    AgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_BARRACKS, 5U);
    AgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_FACTORY, 2U);
    AgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_STARPORT, 2U);
}

}  // namespace

bool TestTerranDescriptorPipeline(int ArgC, char** ArgV)
{
    (void)ArgC;
    (void)ArgV;

    bool SuccessValue = true;

    FTerranGameStateDescriptorBuilder GameStateDescriptorBuilderValue;
    FDefaultStrategicDirector StrategicDirectorValue;

    {
        FAgentState AgentStateValue;
        ConfigureOpeningState(AgentStateValue);

        FGameStateDescriptor GameStateDescriptorValue;
        GameStateDescriptorBuilderValue.RebuildGameStateDescriptor(100U, 224U, AgentStateValue, GameStateDescriptorValue);

        Check(GameStateDescriptorValue.CurrentStep == 100U, SuccessValue,
              "Descriptor builder should write the current step.");
        Check(GameStateDescriptorValue.CurrentGameLoop == 224U, SuccessValue,
              "Descriptor builder should write the current game loop.");
        Check(GameStateDescriptorValue.MacroState.ActiveBaseCount == 1U, SuccessValue,
              "Descriptor builder should rebuild the active base count.");
        Check(GameStateDescriptorValue.MacroState.CurrentGameLoop == 224U, SuccessValue,
              "Descriptor builder should propagate the current game loop into macro state.");
        Check(GameStateDescriptorValue.MacroState.WorkerCount == 16U, SuccessValue,
              "Descriptor builder should rebuild the worker count.");
        Check(GameStateDescriptorValue.MacroState.ArmyUnitCount == 8U, SuccessValue,
              "Descriptor builder should rebuild the army unit count.");
        Check(GameStateDescriptorValue.MacroState.BarracksCount == 1U, SuccessValue,
              "Descriptor builder should rebuild the barracks count.");
        Check(GameStateDescriptorValue.MacroState.ActiveMacroPhase == EMacroPhase::Opening, SuccessValue,
              "Single-base low-tech state should remain in the Opening macro phase.");
        Check(GameStateDescriptorValue.BuildPlanning.AvailableMinerals == 150U, SuccessValue,
              "Descriptor builder should rebuild available minerals.");
        Check(GameStateDescriptorValue.BuildPlanning.CurrentGameLoop == 224U, SuccessValue,
              "Descriptor builder should propagate the current game loop into build planning state.");
        Check(GameStateDescriptorValue.BuildPlanning.AvailableSupply == 7U, SuccessValue,
              "Descriptor builder should rebuild available supply.");
        Check(GameStateDescriptorValue.ArmyState.ReserveUnitCount == 8U, SuccessValue,
              "Descriptor builder should rebuild reserve unit count from the current army.");

        StrategicDirectorValue.UpdateGameStateDescriptor(GameStateDescriptorValue);

        Check(GameStateDescriptorValue.MacroState.ActiveGamePlan == EGamePlan::TimingAttack, SuccessValue,
              "The default strategic director should open on the timing-attack plan.");
        Check(GameStateDescriptorValue.MacroState.DesiredBaseCount == 2U, SuccessValue,
              "The opening plan should target a second base.");
        Check(GameStateDescriptorValue.MacroState.DesiredArmyCount == 1U, SuccessValue,
              "The opening plan should still maintain one army.");
        Check(!GameStateDescriptorValue.ArmyState.ArmyGoals.empty() &&
                  GameStateDescriptorValue.ArmyState.ArmyGoals.front() == EArmyGoal::HoldBase,
              SuccessValue, "The primary army should hold base before the timing army is ready.");
    }

    {
        FAgentState AgentStateValue;
        ConfigureTimingAttackState(AgentStateValue);

        FGameStateDescriptor GameStateDescriptorValue;
        GameStateDescriptorBuilderValue.RebuildGameStateDescriptor(800U, 2688U, AgentStateValue, GameStateDescriptorValue);
        StrategicDirectorValue.UpdateGameStateDescriptor(GameStateDescriptorValue);

        Check(GameStateDescriptorValue.MacroState.ActiveMacroPhase == EMacroPhase::MidGame, SuccessValue,
              "Two-base bio with a starport should be classified as MidGame.");
        Check(GameStateDescriptorValue.MacroState.ActiveGamePlan == EGamePlan::TimingAttack, SuccessValue,
              "Two-base bio should remain on the timing-attack plan.");
        Check(GameStateDescriptorValue.MacroState.DesiredBaseCount == 3U, SuccessValue,
              "The timing-attack plan should target a third base after the two-base setup.");
        Check(HasGoalOfType(GameStateDescriptorValue.GoalSet.StrategicGoals, EGoalType::PressureEnemy,
                            EGoalStatus::Active),
              SuccessValue, "The strategic goal set should include an active PressureEnemy goal in timing states.");
        Check(!GameStateDescriptorValue.ArmyState.ArmyGoals.empty() &&
                  GameStateDescriptorValue.ArmyState.ArmyGoals.front() == EArmyGoal::HoldBase,
              SuccessValue, "The primary army goal should still reflect the active immediate HoldOwnedBase goal.");
    }

    {
        FAgentState AgentStateValue;
        ConfigureMacroState(AgentStateValue);

        FGameStateDescriptor GameStateDescriptorValue;
        GameStateDescriptorBuilderValue.RebuildGameStateDescriptor(1800U, 5376U, AgentStateValue, GameStateDescriptorValue);
        StrategicDirectorValue.UpdateGameStateDescriptor(GameStateDescriptorValue);

        Check(GameStateDescriptorValue.MacroState.ActiveGamePlan == EGamePlan::Macro, SuccessValue,
              "Three-base state should transition the default strategic director to Macro.");
        Check(GameStateDescriptorValue.MacroState.DesiredBaseCount == 4U, SuccessValue,
              "Established three-base macro should target a fourth base.");
        Check(GameStateDescriptorValue.MacroState.DesiredArmyCount == 2U, SuccessValue,
              "Large macro states should request a second army anchor.");
        Check(GameStateDescriptorValue.ArmyState.ActiveArmyCount >= 2U, SuccessValue,
              "The army domain should expand to the requested minimum army count.");
        Check(HasGoalOfType(GameStateDescriptorValue.GoalSet.StrategicGoals, EGoalType::ScoutExpansionLocations,
                            EGoalStatus::Active),
              SuccessValue, "Macro strategic goals should include active map sweep intent.");
        Check(!GameStateDescriptorValue.ArmyState.ArmyGoals.empty() &&
                  GameStateDescriptorValue.ArmyState.ArmyGoals.front() == EArmyGoal::HoldBase,
              SuccessValue, "Immediate HoldOwnedBase should remain the primary derived army goal in macro states.");
        Check(GameStateDescriptorValue.ArmyState.ArmyGoals.size() > 1U &&
                  GameStateDescriptorValue.ArmyState.ArmyGoals[1] == EArmyGoal::HoldBase,
              SuccessValue, "The second army anchor should default to HoldBase.");
    }

    {
        FTerranForecastStateBuilder ForecastStateBuilderValue;
        FEconomyDomainState EconomyDomainStateSeedValue;
        FAgentState SeedAgentStateValue;
        ConfigureOpeningState(SeedAgentStateValue);

        FGameStateDescriptor SeedDescriptorValue;
        GameStateDescriptorBuilderValue.RebuildGameStateDescriptor(850U, 2400U, SeedAgentStateValue,
                                                                  SeedDescriptorValue);
        ForecastStateBuilderValue.RebuildForecastState(SeedAgentStateValue, EconomyDomainStateSeedValue,
                                                       SeedDescriptorValue);

        FAgentState ForecastAgentStateValue;
        ForecastAgentStateValue.Economy.Minerals = 120U;
        ForecastAgentStateValue.Economy.Vespene = 100U;
        ForecastAgentStateValue.Economy.Supply = 30U;
        ForecastAgentStateValue.Economy.SupplyCap = 30U;
        ForecastAgentStateValue.Economy.SupplyAvailable = 0U;
        ForecastAgentStateValue.Units.SetUnitCount(UNIT_TYPEID::TERRAN_SCV, 32U);
        ForecastAgentStateValue.Units.SetUnitCount(UNIT_TYPEID::TERRAN_MARINE, 20U);
        ForecastAgentStateValue.Units.Update();
        ForecastAgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_COMMANDCENTER, 2U);
        ForecastAgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 2U);
        ForecastAgentStateValue.Buildings.SetCurrentlyInConstruction(UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 1U);
        ForecastAgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_BARRACKS, 2U);
        ForecastAgentStateValue.Buildings.SetCurrentlyInConstruction(UNIT_TYPEID::TERRAN_BARRACKS, 1U);
        ForecastAgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_FACTORY, 1U);
        ForecastAgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_STARPORT, 1U);
        std::vector<Unit> ForecastUnitStorageValue;
        ForecastUnitStorageValue.push_back(
            MakeSelfBuildingUnit(1001U, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 0.9f));
        std::vector<const Unit*> ForecastUnitPointersValue = {&ForecastUnitStorageValue[0]};
        ForecastAgentStateValue.UnitContainer.SetUnits(ForecastUnitPointersValue);

        FEconomyDomainState EconomyDomainStateValue = EconomyDomainStateSeedValue;
        FEconomyDomainState EconomyDomainStateCopyValue = EconomyDomainStateSeedValue;
        FGameStateDescriptor ForecastDescriptorValue;
        FGameStateDescriptor ForecastDescriptorCopyValue;
        GameStateDescriptorBuilderValue.RebuildGameStateDescriptor(851U, 2496U, ForecastAgentStateValue,
                                                                  ForecastDescriptorValue);
        GameStateDescriptorBuilderValue.RebuildGameStateDescriptor(851U, 2496U, ForecastAgentStateValue,
                                                                  ForecastDescriptorCopyValue);

        FCommandOrderRecord MarineOrderValue = FCommandOrderRecord::CreateNoTarget(
            ECommandAuthorityLayer::EconomyAndProduction, NullTag, ABILITY_ID::TRAIN_MARINE, 180,
            EIntentDomain::UnitProduction, 2496U);
        MarineOrderValue.TaskType = ECommandTaskType::UnitProduction;
        MarineOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKS;
        MarineOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_MARINE;
        MarineOrderValue.TargetCount = 21U;
        const uint32_t ForecastMarineOrderIdValue =
            ForecastDescriptorValue.CommandAuthoritySchedulingState.EnqueueOrder(MarineOrderValue);
        ForecastDescriptorValue.CommandAuthoritySchedulingState.SetOrderLifecycleState(ForecastMarineOrderIdValue,
                                                                                       EOrderLifecycleState::Ready);
        const uint32_t ForecastMarineOrderCopyIdValue =
            ForecastDescriptorCopyValue.CommandAuthoritySchedulingState.EnqueueOrder(MarineOrderValue);
        ForecastDescriptorCopyValue.CommandAuthoritySchedulingState.SetOrderLifecycleState(
            ForecastMarineOrderCopyIdValue, EOrderLifecycleState::Ready);

        FCommandOrderRecord SupplyDepotOrderValue = FCommandOrderRecord::CreateNoTarget(
            ECommandAuthorityLayer::EconomyAndProduction, NullTag, ABILITY_ID::BUILD_SUPPLYDEPOT, 170,
            EIntentDomain::StructureBuild, 2496U);
        SupplyDepotOrderValue.TaskType = ECommandTaskType::Supply;
        SupplyDepotOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
        SupplyDepotOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_SUPPLYDEPOT;
        SupplyDepotOrderValue.TargetCount = 4U;
        const uint32_t ForecastSupplyDepotOrderIdValue =
            ForecastDescriptorValue.CommandAuthoritySchedulingState.EnqueueOrder(SupplyDepotOrderValue);
        ForecastDescriptorValue.CommandAuthoritySchedulingState.SetOrderLifecycleState(
            ForecastSupplyDepotOrderIdValue, EOrderLifecycleState::Ready);
        const uint32_t ForecastSupplyDepotOrderCopyIdValue =
            ForecastDescriptorCopyValue.CommandAuthoritySchedulingState.EnqueueOrder(SupplyDepotOrderValue);
        ForecastDescriptorCopyValue.CommandAuthoritySchedulingState.SetOrderLifecycleState(
            ForecastSupplyDepotOrderCopyIdValue, EOrderLifecycleState::Ready);

        ForecastStateBuilderValue.RebuildForecastState(ForecastAgentStateValue, EconomyDomainStateValue,
                                                       ForecastDescriptorValue);
        ForecastStateBuilderValue.RebuildForecastState(ForecastAgentStateValue, EconomyDomainStateCopyValue,
                                                       ForecastDescriptorCopyValue);
        StrategicDirectorValue.UpdateGameStateDescriptor(ForecastDescriptorValue);

        Check(ForecastDescriptorValue.SchedulerOutlook.GetScheduledUnitCount(UNIT_TYPEID::TERRAN_MARINE) == 1U,
              SuccessValue, "Forecast rebuild should project scheduled marine production from active scheduler work.");
        Check(ForecastDescriptorValue.ProductionState.GetProjectedBuildingCount(UNIT_TYPEID::TERRAN_BARRACKS) == 3U,
              SuccessValue, "Forecast rebuild should project in-progress barracks into near-term capacity.");
        Check(ForecastDescriptorValue.EconomyState.ProjectedAvailableSupplyByHorizon[ShortForecastHorizonIndexValue] ==
                  7U,
              SuccessValue, "Short-horizon supply projection should only count observed relief that can finish inside the horizon.");
        Check(ForecastDescriptorValue.EconomyState.ProjectedAvailableSupplyByHorizon[LongForecastHorizonIndexValue] ==
                  15U,
              SuccessValue, "Long-horizon supply projection should include scheduled supply depots once their build time fits inside the horizon.");
        Check(HasGoalOfTypeForTarget(ForecastDescriptorValue.GoalSet.NearTermGoals,
                                     EGoalType::BuildProductionCapacity, EGoalStatus::Satisfied,
                                     UNIT_TYPEID::TERRAN_BARRACKS),
              SuccessValue, "Strategic direction should satisfy barracks capacity goals from projected near-term state.");
        Check(ForecastDescriptorValue.EconomyState.ProjectedAvailableSupplyByHorizon[ShortForecastHorizonIndexValue] ==
                  ForecastDescriptorCopyValue.EconomyState
                      .ProjectedAvailableSupplyByHorizon[ShortForecastHorizonIndexValue] &&
                  ForecastDescriptorValue.SchedulerOutlook.GetScheduledUnitCount(UNIT_TYPEID::TERRAN_MARINE) ==
                      ForecastDescriptorCopyValue.SchedulerOutlook.GetScheduledUnitCount(
                          UNIT_TYPEID::TERRAN_MARINE) &&
                  ForecastDescriptorValue.ProductionState.GetProjectedBuildingCount(UNIT_TYPEID::TERRAN_BARRACKS) ==
                      ForecastDescriptorCopyValue.ProductionState.GetProjectedBuildingCount(
                          UNIT_TYPEID::TERRAN_BARRACKS),
              SuccessValue, "Forecast rebuild should remain deterministic when replayed from the same observed and scheduler inputs.");
    }

    return SuccessValue;
}

}  // namespace sc2
