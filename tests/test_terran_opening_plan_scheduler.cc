#include "test_terran_opening_plan_scheduler.h"

#include <iostream>
#include <string>

#include "common/build_orders/FOpeningPlanRegistry.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/planning/ECommandAuthorityLayer.h"
#include "common/planning/EPlanningProcessorState.h"
#include "common/planning/FCommandAuthorityProcessor.h"

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

size_t GetBuildingIndex(const UNIT_TYPEID BuildingTypeIdValue)
{
    return GetTerranBuildingTypeIndex(BuildingTypeIdValue);
}

size_t GetUnitIndex(const UNIT_TYPEID UnitTypeIdValue)
{
    return GetTerranUnitTypeIndex(UnitTypeIdValue);
}

bool TryFindOrderIndexByPlanStepId(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                   const uint32_t PlanStepIdValue, const ECommandAuthorityLayer SourceLayerValue,
                                   size_t& OutOrderIndexValue)
{
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.PlanStepIds[OrderIndexValue] == PlanStepIdValue &&
            CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] == SourceLayerValue)
        {
            OutOrderIndexValue = OrderIndexValue;
            return true;
        }
    }

    return false;
}

void SetObservedWorkerCount(FGameStateDescriptor& GameStateDescriptorValue, const uint32_t WorkerCountValue)
{
    GameStateDescriptorValue.BuildPlanning.ObservedUnitCounts[GetUnitIndex(UNIT_TYPEID::TERRAN_SCV)] =
        static_cast<uint16_t>(WorkerCountValue);
}

}  // namespace

bool TestTerranOpeningPlanScheduler(int ArgC, char** ArgV)
{
    (void)ArgC;
    (void)ArgV;

    bool SuccessValue = true;

    const FOpeningPlanDescriptor& OpeningPlanDescriptorValue =
        FOpeningPlanRegistry::GetOpeningPlanDescriptor(EOpeningPlanId::TerranTwoBaseMMMFrameOpening);
    Check(OpeningPlanDescriptorValue.Steps.size() == 32U, SuccessValue,
          "Compiled opening-plan registry should expose the authored Terran opener steps.");

    FGameStateDescriptor GameStateDescriptorValue;
    GameStateDescriptorValue.BuildPlanning.ObservedTownHallCount = 1U;
    SetObservedWorkerCount(GameStateDescriptorValue, 12U);

    FCommandAuthorityProcessor CommandAuthorityProcessorValue;

    GameStateDescriptorValue.CurrentGameLoop = 357U;
    CommandAuthorityProcessorValue.ProcessSchedulerStep(GameStateDescriptorValue);

    Check(GameStateDescriptorValue.OpeningPlanExecutionState.ActivePlanId ==
              EOpeningPlanId::TerranTwoBaseMMMFrameOpening,
          SuccessValue, "Scheduler processor should initialize the default Terran opening plan.");
    Check(GameStateDescriptorValue.OpeningPlanExecutionState.LifecycleState == EOpeningPlanLifecycleState::Active,
          SuccessValue, "Scheduler processor should mark the opening plan active after initialization.");
    Check(GameStateDescriptorValue.CommandAuthoritySchedulingState.GetOrderCount() == 2U, SuccessValue,
          "Before the first explicit step, the scheduler should seed only the worker goal and its child order.");
    Check(GameStateDescriptorValue.CommandAuthoritySchedulingState.ProcessorState == EPlanningProcessorState::Processing,
          SuccessValue, "Seeded strategic and planning work should place the scheduler in the processing state.");

    size_t WorkerStrategicOrderIndexValue = 0U;
    Check(TryFindOrderIndexByPlanStepId(GameStateDescriptorValue.CommandAuthoritySchedulingState, 9000U,
                                        ECommandAuthorityLayer::StrategicDirector, WorkerStrategicOrderIndexValue),
          SuccessValue, "Scheduler processor should seed the worker-goal strategic order.");
    const FCommandOrderRecord WorkerStrategicOrderValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState.GetOrderRecord(WorkerStrategicOrderIndexValue);
    Check(WorkerStrategicOrderValue.TargetCount == OpeningPlanDescriptorValue.Goals.TargetWorkerCount, SuccessValue,
          "Worker-goal strategic order should target the opening-plan worker goal.");

    GameStateDescriptorValue.CurrentGameLoop = 358U;
    CommandAuthorityProcessorValue.ProcessSchedulerStep(GameStateDescriptorValue);

    Check(GameStateDescriptorValue.CommandAuthoritySchedulingState.GetOrderCount() == 4U, SuccessValue,
          "Once the first opener frame is reached, the scheduler should seed the depot step and its child order.");

    size_t DepotStrategicOrderIndexValue = 0U;
    Check(TryFindOrderIndexByPlanStepId(GameStateDescriptorValue.CommandAuthoritySchedulingState, 1U,
                                        ECommandAuthorityLayer::StrategicDirector, DepotStrategicOrderIndexValue),
          SuccessValue, "Scheduler processor should seed the first supply-depot strategic order.");
    const FCommandOrderRecord DepotStrategicOrderValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState.GetOrderRecord(DepotStrategicOrderIndexValue);
    Check(DepotStrategicOrderValue.AbilityId == ABILITY_ID::BUILD_SUPPLYDEPOT, SuccessValue,
          "First authored opening-plan step should build a supply depot.");
    Check(DepotStrategicOrderValue.ResultUnitTypeId == UNIT_TYPEID::TERRAN_SUPPLYDEPOT, SuccessValue,
          "First authored opening-plan step should target a supply depot result type.");

    size_t DepotChildOrderIndexValue = 0U;
    Check(GameStateDescriptorValue.CommandAuthoritySchedulingState.TryGetChildOrderIndex(
              DepotStrategicOrderValue.OrderId, ECommandAuthorityLayer::EconomyAndProduction, DepotChildOrderIndexValue),
          SuccessValue, "Each strategic opening-plan step should spawn exactly one economy child order.");
    const FCommandOrderRecord DepotChildOrderValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState.GetOrderRecord(DepotChildOrderIndexValue);
    Check(DepotChildOrderValue.PlanStepId == 1U, SuccessValue,
          "Economy child orders should preserve the authored opening-plan step identifier.");

    CommandAuthorityProcessorValue.ProcessSchedulerStep(GameStateDescriptorValue);
    Check(GameStateDescriptorValue.CommandAuthoritySchedulingState.GetOrderCount() == 4U, SuccessValue,
          "Repeated scheduler passes without new observations should not duplicate seeded orders.");

    GameStateDescriptorValue.CurrentGameLoop = 896U;
    CommandAuthorityProcessorValue.ProcessSchedulerStep(GameStateDescriptorValue);
    size_t BarracksStrategicOrderIndexValue = 0U;
    Check(!TryFindOrderIndexByPlanStepId(GameStateDescriptorValue.CommandAuthoritySchedulingState, 2U,
                                         ECommandAuthorityLayer::StrategicDirector, BarracksStrategicOrderIndexValue),
          SuccessValue, "Dependency-gated steps should stay deferred until their prerequisite step completes.");

    GameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetBuildingIndex(UNIT_TYPEID::TERRAN_SUPPLYDEPOT)] = 1U;
    CommandAuthorityProcessorValue.ProcessSchedulerStep(GameStateDescriptorValue);

    Check(GameStateDescriptorValue.OpeningPlanExecutionState.IsStepCompleted(1U), SuccessValue,
          "Observed state should complete the authored opening-plan step once the target is satisfied.");
    Check(TryFindOrderIndexByPlanStepId(GameStateDescriptorValue.CommandAuthoritySchedulingState, 2U,
                                        ECommandAuthorityLayer::StrategicDirector, BarracksStrategicOrderIndexValue),
          SuccessValue, "After the depot completes, the barracks step should unlock at its authored game loop.");

    const FCommandOrderRecord BarracksStrategicOrderValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState.GetOrderRecord(BarracksStrategicOrderIndexValue);
    Check(BarracksStrategicOrderValue.AbilityId == ABILITY_ID::BUILD_BARRACKS, SuccessValue,
          "The second authored step should seed a barracks strategic order.");

    size_t RefineryStrategicOrderIndexValue = 0U;
    Check(!TryFindOrderIndexByPlanStepId(GameStateDescriptorValue.CommandAuthoritySchedulingState, 3U,
                                         ECommandAuthorityLayer::StrategicDirector, RefineryStrategicOrderIndexValue),
          SuccessValue, "Later authored steps should not seed before their minimum game loop is reached.");

    GameStateDescriptorValue.CurrentGameLoop = 941U;
    CommandAuthorityProcessorValue.ProcessSchedulerStep(GameStateDescriptorValue);
    Check(TryFindOrderIndexByPlanStepId(GameStateDescriptorValue.CommandAuthoritySchedulingState, 3U,
                                        ECommandAuthorityLayer::StrategicDirector, RefineryStrategicOrderIndexValue),
          SuccessValue, "Scheduler processor should seed later authored steps once their frame gate is reached.");

    GameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetBuildingIndex(UNIT_TYPEID::TERRAN_BARRACKS)] = 1U;
    CommandAuthorityProcessorValue.ProcessSchedulerStep(GameStateDescriptorValue);
    Check(GameStateDescriptorValue.OpeningPlanExecutionState.IsStepCompleted(2U), SuccessValue,
          "Observed barracks completion should complete the second authored opening-plan step.");

    return SuccessValue;
}

}  // namespace sc2
