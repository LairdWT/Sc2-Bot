#include "test_terran_opening_plan_scheduler.h"

#include <initializer_list>
#include <iostream>
#include <string>

#include "common/build_orders/FOpeningPlanRegistry.h"
#include "common/descriptors/EObservedWallSlotState.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/goals/EGoalDomain.h"
#include "common/goals/EGoalHorizon.h"
#include "common/goals/EGoalStatus.h"
#include "common/goals/EGoalType.h"
#include "common/goals/FGoalDescriptor.h"
#include "common/planning/ECommandAuthorityLayer.h"
#include "common/planning/ECommandCommitmentClass.h"
#include "common/planning/ECommandTaskExecutionGuarantee.h"
#include "common/planning/ECommandTaskOrigin.h"
#include "common/planning/ECommandTaskRetentionPolicy.h"
#include "common/planning/ECommandTaskType.h"
#include "common/planning/EOrderLifecycleState.h"
#include "common/planning/EPlanningProcessorState.h"
#include "common/planning/FCommandAuthorityProcessor.h"
#include "common/planning/FDefaultStrategicDirector.h"

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

FBuildPlacementSlotId CreatePlacementSlotIdValue(const EBuildPlacementSlotType BuildPlacementSlotTypeValue,
                                                 const uint8_t OrdinalValue)
{
    FBuildPlacementSlotId BuildPlacementSlotIdValue;
    BuildPlacementSlotIdValue.SlotType = BuildPlacementSlotTypeValue;
    BuildPlacementSlotIdValue.Ordinal = OrdinalValue;
    return BuildPlacementSlotIdValue;
}

void SetObservedPlacementSlotState(FGameStateDescriptor& GameStateDescriptorValue,
                                   const EBuildPlacementSlotType BuildPlacementSlotTypeValue,
                                   const uint8_t OrdinalValue,
                                   const EObservedWallSlotState ObservedWallSlotStateValue)
{
    GameStateDescriptorValue.ObservedPlacementSlotState.SetObservedPlacementSlotState(
        CreatePlacementSlotIdValue(BuildPlacementSlotTypeValue, OrdinalValue), ObservedWallSlotStateValue);
    GameStateDescriptorValue.ObservedRampWallState.SetObservedWallSlotState(BuildPlacementSlotTypeValue,
                                                                            ObservedWallSlotStateValue);
}

void CheckOpeningPlanStepMetadata(const FOpeningPlanStep& OpeningPlanStepValue, const uint32_t ExpectedStepIdValue,
                                  const uint64_t ExpectedMinGameLoopValue, const int ExpectedPriorityValue,
                                  const AbilityID ExpectedAbilityIdValue,
                                  const UNIT_TYPEID ExpectedProducerUnitTypeIdValue,
                                  const UNIT_TYPEID ExpectedResultUnitTypeIdValue,
                                  const uint32_t ExpectedTargetCountValue,
                                  const uint32_t ExpectedRequestedQueueCountValue,
                                  const uint32_t ExpectedParallelGroupIdValue,
                                  const std::initializer_list<uint32_t>& ExpectedRequiredStepIdsValue,
                                  bool& SuccessValue, const std::string& StepLabelValue)
{
    const FCommandTaskDescriptor& TaskDescriptorValue = OpeningPlanStepValue.TaskDescriptor;

    Check(TaskDescriptorValue.TaskId == ExpectedStepIdValue, SuccessValue,
          StepLabelValue + " should preserve the authored step identifier.");
    Check(TaskDescriptorValue.TriggerMinGameLoop == ExpectedMinGameLoopValue, SuccessValue,
          StepLabelValue + " should preserve the authored frame trigger.");
    Check(TaskDescriptorValue.BasePriorityValue == ExpectedPriorityValue, SuccessValue,
          StepLabelValue + " should preserve the authored priority.");
    Check(TaskDescriptorValue.ActionAbilityId == ExpectedAbilityIdValue, SuccessValue,
          StepLabelValue + " should preserve the authored ability.");
    Check(TaskDescriptorValue.ActionProducerUnitTypeId == ExpectedProducerUnitTypeIdValue, SuccessValue,
          StepLabelValue + " should preserve the authored producer type.");
    Check(TaskDescriptorValue.ActionResultUnitTypeId == ExpectedResultUnitTypeIdValue, SuccessValue,
          StepLabelValue + " should preserve the authored result type.");
    Check(TaskDescriptorValue.ActionTargetCount == ExpectedTargetCountValue, SuccessValue,
          StepLabelValue + " should preserve the authored target count.");
    Check(TaskDescriptorValue.ActionRequestedQueueCount == ExpectedRequestedQueueCountValue, SuccessValue,
          StepLabelValue + " should preserve the authored requested queue count.");
    Check(TaskDescriptorValue.ParallelGroupId == ExpectedParallelGroupIdValue, SuccessValue,
          StepLabelValue + " should preserve the authored parallel-group binding.");

    const std::vector<uint32_t> ExpectedRequiredStepIdValues(ExpectedRequiredStepIdsValue.begin(),
                                                             ExpectedRequiredStepIdsValue.end());
    Check(TaskDescriptorValue.TriggerRequiredCompletedTaskIds == ExpectedRequiredStepIdValues, SuccessValue,
          StepLabelValue + " should preserve the authored prerequisite steps.");
}

size_t GetBuildingIndex(const UNIT_TYPEID BuildingTypeIdValue)
{
    return GetTerranBuildingTypeIndex(BuildingTypeIdValue);
}

size_t GetUnitIndex(const UNIT_TYPEID UnitTypeIdValue)
{
    return GetTerranUnitTypeIndex(UnitTypeIdValue);
}

FGoalDescriptor CreateGoalDescriptor(const uint32_t GoalIdValue, const EGoalDomain GoalDomainValue,
                                     const EGoalHorizon GoalHorizonValue, const EGoalType GoalTypeValue,
                                     const EGoalStatus GoalStatusValue, const int BasePriorityValue,
                                     const uint32_t TargetCountValue = 0U,
                                     const UNIT_TYPEID TargetUnitTypeIdValue = UNIT_TYPEID::INVALID,
                                     const UpgradeID TargetUpgradeIdValue = UpgradeID(UPGRADE_ID::INVALID))
{
    FGoalDescriptor GoalDescriptorValue;
    GoalDescriptorValue.GoalId = GoalIdValue;
    GoalDescriptorValue.GoalDomain = GoalDomainValue;
    GoalDescriptorValue.GoalHorizon = GoalHorizonValue;
    GoalDescriptorValue.GoalType = GoalTypeValue;
    GoalDescriptorValue.GoalStatus = GoalStatusValue;
    GoalDescriptorValue.BasePriorityValue = BasePriorityValue;
    GoalDescriptorValue.TargetCount = TargetCountValue;
    GoalDescriptorValue.TargetUnitTypeId = TargetUnitTypeIdValue;
    GoalDescriptorValue.TargetUpgradeId = TargetUpgradeIdValue;
    return GoalDescriptorValue;
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

bool TryFindStrategicOrderByTaskType(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                     const ECommandTaskType CommandTaskTypeValue, size_t& OutOrderIndexValue)
{
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] == ECommandAuthorityLayer::StrategicDirector &&
            CommandAuthoritySchedulingStateValue.TaskTypes[OrderIndexValue] == CommandTaskTypeValue &&
            !IsTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            OutOrderIndexValue = OrderIndexValue;
            return true;
        }
    }

    return false;
}

bool TryFindStrategicOrderByGoalId(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                   const uint32_t GoalIdValue, size_t& OutOrderIndexValue)
{
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] !=
                ECommandAuthorityLayer::StrategicDirector ||
            CommandAuthoritySchedulingStateValue.SourceGoalIds[OrderIndexValue] != GoalIdValue ||
            IsTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        OutOrderIndexValue = OrderIndexValue;
        return true;
    }

    return false;
}

uint32_t CountActiveStrategicOpeningSupplyOrders(
    const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue)
{
    uint32_t ActiveStrategicOpeningSupplyOrderCountValue = 0U;
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] !=
                ECommandAuthorityLayer::StrategicDirector ||
            CommandAuthoritySchedulingStateValue.TaskOrigins[OrderIndexValue] != ECommandTaskOrigin::Opening ||
            CommandAuthoritySchedulingStateValue.AbilityIds[OrderIndexValue] != ABILITY_ID::BUILD_SUPPLYDEPOT ||
            IsTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]) ||
            CommandAuthoritySchedulingStateValue.ExecutionGuarantees[OrderIndexValue] ==
                ECommandTaskExecutionGuarantee::MustExecute)
        {
            continue;
        }

        ++ActiveStrategicOpeningSupplyOrderCountValue;
    }

    return ActiveStrategicOpeningSupplyOrderCountValue;
}

void SetObservedWorkerCount(FGameStateDescriptor& GameStateDescriptorValue, const uint32_t WorkerCountValue)
{
    GameStateDescriptorValue.BuildPlanning.ObservedUnitCounts[GetUnitIndex(UNIT_TYPEID::TERRAN_SCV)] =
        static_cast<uint16_t>(WorkerCountValue);
}

void SetProjectedDiscretionaryBudget(FGameStateDescriptor& GameStateDescriptorValue, const uint32_t MineralsValue,
                                     const uint32_t VespeneValue, const uint32_t SupplyValue)
{
    GameStateDescriptorValue.BuildPlanning.AvailableMinerals = MineralsValue;
    GameStateDescriptorValue.BuildPlanning.AvailableVespene = VespeneValue;
    GameStateDescriptorValue.BuildPlanning.AvailableSupply = SupplyValue;
    GameStateDescriptorValue.EconomyState.BudgetedMinerals = MineralsValue;
    GameStateDescriptorValue.EconomyState.BudgetedVespene = VespeneValue;
    GameStateDescriptorValue.EconomyState.BudgetedSupplyAvailable = SupplyValue;
    GameStateDescriptorValue.EconomyState.ProjectedAvailableMineralsByHorizon[ShortForecastHorizonIndexValue] =
        MineralsValue;
    GameStateDescriptorValue.EconomyState.ProjectedAvailableVespeneByHorizon[ShortForecastHorizonIndexValue] =
        VespeneValue;
    GameStateDescriptorValue.EconomyState.ProjectedAvailableSupplyByHorizon[ShortForecastHorizonIndexValue] =
        SupplyValue;
    GameStateDescriptorValue.CommitmentLedger.ProjectedAvailableAfterMandatoryMineralsByHorizon[
        ShortForecastHorizonIndexValue] = MineralsValue;
    GameStateDescriptorValue.CommitmentLedger.ProjectedAvailableAfterMandatoryVespeneByHorizon[
        ShortForecastHorizonIndexValue] = VespeneValue;
    GameStateDescriptorValue.CommitmentLedger.ProjectedAvailableAfterMandatorySupplyByHorizon[
        ShortForecastHorizonIndexValue] = SupplyValue;
    GameStateDescriptorValue.CommitmentLedger.ProjectedDiscretionaryMineralsByHorizon[
        ShortForecastHorizonIndexValue] = MineralsValue;
    GameStateDescriptorValue.CommitmentLedger.ProjectedDiscretionaryVespeneByHorizon[
        ShortForecastHorizonIndexValue] = VespeneValue;
    GameStateDescriptorValue.CommitmentLedger.ProjectedDiscretionarySupplyByHorizon[
        ShortForecastHorizonIndexValue] = SupplyValue;
}

}  // namespace

bool TestTerranOpeningPlanScheduler(int ArgC, char** ArgV)
{
    (void)ArgC;
    (void)ArgV;

    bool SuccessValue = true;

    const FOpeningPlanDescriptor& OpeningPlanDescriptorValue =
        FOpeningPlanRegistry::GetOpeningPlanDescriptor(EOpeningPlanId::TerranTwoBaseMMMFrameOpening);
    Check(OpeningPlanDescriptorValue.Steps.size() == 102U, SuccessValue,
          "Compiled opening-plan registry should expose the authored Terran opener steps.");
    Check(OpeningPlanDescriptorValue.Summary ==
              "Two-base MMM pressure package expressed in explicit game-loop steps through the 8:17 follow-up add-on wave.",
          SuccessValue, "Compiled opening-plan registry should preserve the authored summary.");
    Check(OpeningPlanDescriptorValue.Goals.TargetBaseCount == 3U, SuccessValue,
          "Compiled opening-plan registry should preserve the authored target base count.");
    Check(OpeningPlanDescriptorValue.Goals.TargetOrbitalCommandCount == 2U, SuccessValue,
          "Compiled opening-plan registry should preserve the authored target orbital count.");
    Check(OpeningPlanDescriptorValue.Goals.TargetWorkerCount == 44U, SuccessValue,
          "Compiled opening-plan registry should preserve the authored target worker count.");
    Check(OpeningPlanDescriptorValue.Goals.TargetRefineryCount == 4U, SuccessValue,
          "Compiled opening-plan registry should preserve the authored refinery target.");
    Check(OpeningPlanDescriptorValue.Goals.TargetSupplyDepotCount == 14U, SuccessValue,
          "Compiled opening-plan registry should preserve the authored supply-depot target.");
    Check(OpeningPlanDescriptorValue.Goals.TargetBarracksCount == 5U, SuccessValue,
          "Compiled opening-plan registry should preserve the authored barracks target.");
    Check(OpeningPlanDescriptorValue.Goals.TargetFactoryCount == 1U, SuccessValue,
          "Compiled opening-plan registry should preserve the authored factory target.");
    Check(OpeningPlanDescriptorValue.Goals.TargetStarportCount == 1U, SuccessValue,
          "Compiled opening-plan registry should preserve the authored starport target.");
    Check(OpeningPlanDescriptorValue.Goals.TargetBarracksReactorCount == 2U, SuccessValue,
          "Compiled opening-plan registry should preserve the authored barracks-reactor target.");
    Check(OpeningPlanDescriptorValue.Goals.TargetBarracksTechLabCount == 1U, SuccessValue,
          "Compiled opening-plan registry should preserve the authored barracks-tech-lab target.");
    Check(OpeningPlanDescriptorValue.Goals.TargetFactoryTechLabCount == 2U, SuccessValue,
          "Compiled opening-plan registry should preserve the authored factory-tech-lab target.");
    Check(OpeningPlanDescriptorValue.Goals.TargetStarportReactorCount == 1U, SuccessValue,
          "Compiled opening-plan registry should preserve the authored starport-reactor target.");
    Check(OpeningPlanDescriptorValue.Goals.TargetMarineCount == 36U, SuccessValue,
          "Compiled opening-plan registry should preserve the authored marine target.");
    Check(OpeningPlanDescriptorValue.Goals.TargetMarauderCount == 15U, SuccessValue,
          "Compiled opening-plan registry should preserve the authored marauder target.");
    Check(OpeningPlanDescriptorValue.Goals.TargetHellionCount == 1U, SuccessValue,
          "Compiled opening-plan registry should preserve the authored hellion target.");
    Check(OpeningPlanDescriptorValue.Goals.TargetCycloneCount == 1U, SuccessValue,
          "Compiled opening-plan registry should preserve the authored cyclone target.");
    Check(OpeningPlanDescriptorValue.Goals.TargetMedivacCount == 6U, SuccessValue,
          "Compiled opening-plan registry should preserve the authored medivac target.");
    Check(OpeningPlanDescriptorValue.Goals.TargetLiberatorCount == 1U, SuccessValue,
          "Compiled opening-plan registry should preserve the authored liberator target.");
    Check(OpeningPlanDescriptorValue.Goals.TargetSiegeTankCount == 1U, SuccessValue,
          "Compiled opening-plan registry should preserve the authored siege-tank target.");

    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[0], 1U, 358U, 100, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                 UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 1U, 1U, 0U, {},
                                 SuccessValue,
                                 "Opening step 1");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[1], 2U, 896U, 95, ABILITY_ID::BUILD_BARRACKS,
                                 UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_BARRACKS, 1U, 1U, 15U, {1U},
                                 SuccessValue, "Opening step 2");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[2], 3U, 941U, 94, ABILITY_ID::BUILD_REFINERY,
                                 UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_REFINERY, 1U, 1U, 15U, {1U},
                                 SuccessValue, "Opening step 3");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[3], 4U, 1926U, 90, ABILITY_ID::TRAIN_MARINE,
                                 UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 1U, 1U, 0U, {2U},
                                 SuccessValue, "Opening step 4");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[4], 5U, 1949U, 89, ABILITY_ID::MORPH_ORBITALCOMMAND,
                                 UNIT_TYPEID::TERRAN_COMMANDCENTER, UNIT_TYPEID::TERRAN_ORBITALCOMMAND, 1U, 1U, 0U,
                                 {2U}, SuccessValue, "Opening step 5");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[5], 6U, 2195U, 88, ABILITY_ID::BUILD_COMMANDCENTER,
                                 UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_COMMANDCENTER, 2U, 1U, 20U, {3U},
                                 SuccessValue, "Opening step 6");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[6], 7U, 2330U, 87,
                                 ABILITY_ID::BUILD_REACTOR_BARRACKS, UNIT_TYPEID::TERRAN_BARRACKS,
                                 UNIT_TYPEID::TERRAN_BARRACKSREACTOR, 1U, 1U, 20U, {2U}, SuccessValue,
                                 "Opening step 7");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[7], 8U, 2464U, 86, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                 UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 2U, 1U, 0U, {},
                                 SuccessValue,
                                 "Opening step 8");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[8], 9U, 2867U, 84, ABILITY_ID::BUILD_FACTORY,
                                 UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_FACTORY, 1U, 1U, 22U, {3U},
                                 SuccessValue,
                                 "Opening step 9");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[9], 10U, 3024U, 83, ABILITY_ID::BUILD_REFINERY,
                                 UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_REFINERY, 2U, 1U, 22U, {6U},
                                 SuccessValue, "Opening step 10");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[10], 11U, 3136U, 82, ABILITY_ID::TRAIN_MARINE,
                                 UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 3U, 2U, 0U, {7U},
                                 SuccessValue, "Opening step 11");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[11], 12U, 3699U, 81, ABILITY_ID::TRAIN_MARINE,
                                 UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 5U, 2U, 0U, {11U},
                                 SuccessValue, "Opening step 12");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[12], 13U, 3808U, 80,
                                 ABILITY_ID::MORPH_ORBITALCOMMAND, UNIT_TYPEID::TERRAN_COMMANDCENTER,
                                 UNIT_TYPEID::TERRAN_ORBITALCOMMAND, 2U, 1U, 0U, {6U}, SuccessValue,
                                 "Opening step 13");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[13], 14U, 3853U, 79, ABILITY_ID::BUILD_STARPORT,
                                 UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_STARPORT, 1U, 1U, 29U, {9U},
                                 SuccessValue, "Opening step 14");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[14], 15U, 3898U, 78, ABILITY_ID::TRAIN_HELLION,
                                 UNIT_TYPEID::TERRAN_FACTORY, UNIT_TYPEID::TERRAN_HELLION, 1U, 1U, 29U, {9U},
                                 SuccessValue, "Opening step 15");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[15], 16U, 4010U, 77, ABILITY_ID::TRAIN_MARINE,
                                 UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 6U, 1U, 0U, {12U},
                                 SuccessValue, "Opening step 16");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[16], 17U, 4077U, 76, ABILITY_ID::TRAIN_MARINE,
                                 UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 7U, 1U, 0U, {16U},
                                 SuccessValue, "Opening step 17");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[17], 18U, 4278U, 75,
                                 ABILITY_ID::BUILD_SUPPLYDEPOT, UNIT_TYPEID::TERRAN_SCV,
                                 UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 3U, 1U, 0U, {}, SuccessValue, "Opening step 18");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[18], 19U, 4390U, 74,
                                 ABILITY_ID::BUILD_TECHLAB_FACTORY, UNIT_TYPEID::TERRAN_FACTORY,
                                 UNIT_TYPEID::TERRAN_FACTORYTECHLAB, 1U, 1U, 0U, {9U}, SuccessValue,
                                 "Opening step 19");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[19], 20U, 4502U, 73, ABILITY_ID::TRAIN_MARINE,
                                 UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 8U, 1U, 36U, {17U},
                                 SuccessValue, "Opening step 20");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[20], 21U, 4525U, 72, ABILITY_ID::TRAIN_MARINE,
                                 UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 9U, 1U, 36U, {20U},
                                 SuccessValue, "Opening step 21");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[21], 22U, 4659U, 71, ABILITY_ID::TRAIN_MEDIVAC,
                                 UNIT_TYPEID::TERRAN_STARPORT, UNIT_TYPEID::TERRAN_MEDIVAC, 1U, 1U, 0U, {14U},
                                 SuccessValue, "Opening step 22");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[22], 23U, 4794U, 70, ABILITY_ID::TRAIN_CYCLONE,
                                 UNIT_TYPEID::TERRAN_FACTORY, UNIT_TYPEID::TERRAN_CYCLONE, 1U, 1U, 0U, {19U},
                                 SuccessValue, "Opening step 23");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[23], 24U, 4950U, 69, ABILITY_ID::TRAIN_MARINE,
                                 UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 11U, 2U, 0U, {21U},
                                 SuccessValue, "Opening step 24");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[24], 25U, 5040U, 68,
                                 ABILITY_ID::BUILD_SUPPLYDEPOT, UNIT_TYPEID::TERRAN_SCV,
                                 UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 4U, 1U, 0U, {}, SuccessValue, "Opening step 25");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[25], 26U, 5264U, 67, ABILITY_ID::BUILD_BARRACKS,
                                 UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_BARRACKS, 2U, 1U, 0U, {6U},
                                 SuccessValue, "Opening step 26");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[26], 27U, 5398U, 66,
                                 ABILITY_ID::TRAIN_LIBERATOR, UNIT_TYPEID::TERRAN_STARPORT,
                                 UNIT_TYPEID::TERRAN_LIBERATOR, 1U, 1U, 51U, {14U}, SuccessValue,
                                 "Opening step 27");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[27], 28U, 5421U, 65, ABILITY_ID::TRAIN_MARINE,
                                 UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 12U, 1U, 51U, {24U},
                                 SuccessValue, "Opening step 28");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[28], 29U, 5578U, 64,
                                 ABILITY_ID::TRAIN_SIEGETANK, UNIT_TYPEID::TERRAN_FACTORY,
                                 UNIT_TYPEID::TERRAN_SIEGETANK, 1U, 1U, 0U, {19U}, SuccessValue,
                                 "Opening step 29");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[29], 30U, 5712U, 63,
                                 ABILITY_ID::BUILD_SUPPLYDEPOT, UNIT_TYPEID::TERRAN_SCV,
                                 UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 5U, 1U, 0U, {}, SuccessValue, "Opening step 30");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[30], 31U, 5779U, 62, ABILITY_ID::TRAIN_MARINE,
                                 UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 13U, 1U, 0U, {28U},
                                 SuccessValue, "Opening step 31");
    CheckOpeningPlanStepMetadata(OpeningPlanDescriptorValue.Steps[31], 32U, 5824U, 61, ABILITY_ID::TRAIN_MARINE,
                                 UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 14U, 1U, 0U, {31U},
                                 SuccessValue, "Opening step 32");
    Check(OpeningPlanDescriptorValue.Steps[0].TaskDescriptor.ActionPreferredPlacementSlotType ==
              EBuildPlacementSlotType::MainRampDepotLeft,
          SuccessValue, "The first depot step should bind to the left ramp depot slot.");
    Check(OpeningPlanDescriptorValue.Steps[0].TaskDescriptor.ActionPreferredPlacementSlotId.SlotType ==
                  EBuildPlacementSlotType::MainRampDepotLeft &&
              OpeningPlanDescriptorValue.Steps[0].TaskDescriptor.ActionPreferredPlacementSlotId.Ordinal == 0U,
          SuccessValue, "The first depot step should bind to the left ramp depot slot ordinal zero.");
    Check(OpeningPlanDescriptorValue.Steps[0].TaskDescriptor.Origin == ECommandTaskOrigin::Opening, SuccessValue,
          "Opening steps should preserve opening task origin metadata.");
    Check(OpeningPlanDescriptorValue.Steps[0].TaskDescriptor.CommitmentClass ==
              ECommandCommitmentClass::MandatoryOpening,
          SuccessValue, "Exact-slot wall depot steps should be mandatory opening commitments.");
    Check(OpeningPlanDescriptorValue.Steps[0].TaskDescriptor.ExecutionGuarantee ==
              ECommandTaskExecutionGuarantee::MustExecute,
          SuccessValue, "Exact-slot wall depot steps should be marked must-execute.");
    Check(OpeningPlanDescriptorValue.Steps[0].TaskDescriptor.RetentionPolicy ==
              ECommandTaskRetentionPolicy::HotMustRun,
          SuccessValue, "Exact-slot wall depot steps should stay in the hot must-run lane.");
    Check(OpeningPlanDescriptorValue.Steps[1].TaskDescriptor.ActionPreferredPlacementSlotType ==
              EBuildPlacementSlotType::MainRampBarracksWithAddon,
          SuccessValue, "The first barracks step should bind to the ramp barracks slot.");
    Check(OpeningPlanDescriptorValue.Steps[1].TaskDescriptor.ActionPreferredPlacementSlotId.SlotType ==
                  EBuildPlacementSlotType::MainRampBarracksWithAddon &&
              OpeningPlanDescriptorValue.Steps[1].TaskDescriptor.ActionPreferredPlacementSlotId.Ordinal == 0U,
          SuccessValue, "The first barracks step should bind to the ramp barracks slot ordinal zero.");
    Check(OpeningPlanDescriptorValue.Steps[1].TaskDescriptor.CommitmentClass ==
              ECommandCommitmentClass::MandatoryOpening,
          SuccessValue, "The wall barracks step should be a mandatory opening commitment.");
    Check(OpeningPlanDescriptorValue.Steps[7].TaskDescriptor.ActionPreferredPlacementSlotType ==
              EBuildPlacementSlotType::MainRampDepotRight,
          SuccessValue, "The second wall depot step should bind to the right ramp depot slot.");
    Check(OpeningPlanDescriptorValue.Steps[7].TaskDescriptor.ActionPreferredPlacementSlotId.SlotType ==
                  EBuildPlacementSlotType::MainRampDepotRight &&
              OpeningPlanDescriptorValue.Steps[7].TaskDescriptor.ActionPreferredPlacementSlotId.Ordinal == 0U,
          SuccessValue, "The second wall depot step should bind to the right ramp depot slot ordinal zero.");
    Check(OpeningPlanDescriptorValue.Steps[7].TaskDescriptor.CommitmentClass ==
              ECommandCommitmentClass::MandatoryOpening,
          SuccessValue, "The second wall depot step should be a mandatory opening commitment.");
    Check(OpeningPlanDescriptorValue.Steps[6].TaskDescriptor.ActionPreferredProducerPlacementSlotId.SlotType ==
                  EBuildPlacementSlotType::MainRampBarracksWithAddon &&
              OpeningPlanDescriptorValue.Steps[6].TaskDescriptor.ActionPreferredProducerPlacementSlotId.Ordinal == 0U,
          SuccessValue, "The first barracks reactor step should bind to the wall barracks producer slot.");
    Check(OpeningPlanDescriptorValue.Steps[6].TaskDescriptor.CommitmentClass ==
              ECommandCommitmentClass::MandatoryOpening,
          SuccessValue, "The first barracks reactor step should be a mandatory opening commitment.");
    Check(OpeningPlanDescriptorValue.Steps[6].TaskDescriptor.ExecutionGuarantee ==
              ECommandTaskExecutionGuarantee::MustExecute,
          SuccessValue, "The first barracks reactor step should be marked must-execute.");
    Check(OpeningPlanDescriptorValue.Steps[8].TaskDescriptor.ActionPreferredPlacementSlotType ==
              EBuildPlacementSlotType::MainFactoryWithAddon,
          SuccessValue, "The first factory step should bind to the first authored main factory slot.");
    Check(OpeningPlanDescriptorValue.Steps[8].TaskDescriptor.ActionPreferredPlacementSlotId.SlotType ==
                  EBuildPlacementSlotType::MainFactoryWithAddon &&
              OpeningPlanDescriptorValue.Steps[8].TaskDescriptor.ActionPreferredPlacementSlotId.Ordinal == 0U,
          SuccessValue, "The first factory step should bind to main factory slot ordinal zero.");
    Check(OpeningPlanDescriptorValue.Steps[8].TaskDescriptor.CommitmentClass ==
              ECommandCommitmentClass::MandatoryOpening,
          SuccessValue, "The first factory step should be a mandatory opening commitment.");
    Check(OpeningPlanDescriptorValue.Steps[13].TaskDescriptor.ActionPreferredPlacementSlotType ==
              EBuildPlacementSlotType::MainStarportWithAddon,
          SuccessValue, "The first starport step should bind to the first authored main starport slot.");
    Check(OpeningPlanDescriptorValue.Steps[13].TaskDescriptor.ActionPreferredPlacementSlotId.SlotType ==
                  EBuildPlacementSlotType::MainStarportWithAddon &&
              OpeningPlanDescriptorValue.Steps[13].TaskDescriptor.ActionPreferredPlacementSlotId.Ordinal == 0U,
          SuccessValue, "The first starport step should bind to main starport slot ordinal zero.");
    Check(OpeningPlanDescriptorValue.Steps[13].TaskDescriptor.CommitmentClass ==
              ECommandCommitmentClass::MandatoryOpening,
          SuccessValue, "The first starport step should be a mandatory opening commitment.");
    Check(OpeningPlanDescriptorValue.Steps[18].TaskDescriptor.ActionPreferredProducerPlacementSlotId.SlotType ==
                  EBuildPlacementSlotType::MainFactoryWithAddon &&
              OpeningPlanDescriptorValue.Steps[18].TaskDescriptor.ActionPreferredProducerPlacementSlotId.Ordinal == 0U,
          SuccessValue, "The first factory tech-lab step should bind to the first authored main factory slot.");
    Check(OpeningPlanDescriptorValue.Steps[40].TaskDescriptor.ActionPreferredProducerPlacementSlotId.SlotType ==
                  EBuildPlacementSlotType::MainStarportWithAddon &&
              OpeningPlanDescriptorValue.Steps[40].TaskDescriptor.ActionPreferredProducerPlacementSlotId.Ordinal == 0U,
          SuccessValue, "The first starport add-on step should bind to the first authored main starport slot.");
    Check(OpeningPlanDescriptorValue.Steps[25].TaskDescriptor.ActionPreferredPlacementSlotType ==
              EBuildPlacementSlotType::MainBarracksWithAddon,
          SuccessValue, "The second barracks step should bind to the first authored main barracks slot.");
    Check(OpeningPlanDescriptorValue.Steps[25].TaskDescriptor.ActionPreferredPlacementSlotId.SlotType ==
                  EBuildPlacementSlotType::MainBarracksWithAddon &&
              OpeningPlanDescriptorValue.Steps[25].TaskDescriptor.ActionPreferredPlacementSlotId.Ordinal == 0U,
          SuccessValue, "The second barracks step should bind to main barracks slot ordinal zero.");
    Check(OpeningPlanDescriptorValue.Steps[25].TaskDescriptor.CommitmentClass ==
              ECommandCommitmentClass::MandatoryOpening,
          SuccessValue, "The exact-slot follow-up barracks step should be a mandatory opening commitment.");
    Check(OpeningPlanDescriptorValue.Steps[0].TaskDescriptor.PackageKind == ECommandTaskPackageKind::Opening,
          SuccessValue, "Opening steps should preserve the standardized Opening package kind.");
    Check(OpeningPlanDescriptorValue.Steps[0].TaskDescriptor.NeedKind == ECommandTaskNeedKind::Structure,
          SuccessValue, "Structure tasks should preserve the standardized structure need kind.");
    Check(OpeningPlanDescriptorValue.Steps[3].TaskDescriptor.NeedKind == ECommandTaskNeedKind::Unit, SuccessValue,
          "Unit-training steps should preserve the standardized unit need kind.");
    Check(OpeningPlanDescriptorValue.Steps[37].TaskDescriptor.ActionKind == ECommandTaskActionKind::ResearchUpgrade,
          SuccessValue, "Research steps should preserve the standardized research action kind.");
    Check(OpeningPlanDescriptorValue.Steps[37].TaskDescriptor.CompletionKind ==
              ECommandTaskCompletionKind::CountAtLeast,
          SuccessValue, "Opening steps should preserve the standardized count-at-least completion kind.");

    FGameStateDescriptor GameStateDescriptorValue;
    GameStateDescriptorValue.BuildPlanning.ObservedTownHallCount = 1U;
    SetObservedWorkerCount(GameStateDescriptorValue, 12U);
    SetProjectedDiscretionaryBudget(GameStateDescriptorValue, 500U, 100U, 7U);
    GameStateDescriptorValue.MacroState.ActiveBaseCount = 1U;
    GameStateDescriptorValue.MacroState.WorkerCount = 12U;
    GameStateDescriptorValue.MacroState.ActiveMacroPhase = EMacroPhase::Opening;

    FCommandAuthorityProcessor CommandAuthorityProcessorValue;
    FDefaultStrategicDirector StrategicDirectorValue;

    GameStateDescriptorValue.CurrentGameLoop = 357U;
    StrategicDirectorValue.UpdateGameStateDescriptor(GameStateDescriptorValue);
    CommandAuthorityProcessorValue.ProcessSchedulerStep(GameStateDescriptorValue);

    Check(GameStateDescriptorValue.OpeningPlanExecutionState.ActivePlanId ==
              EOpeningPlanId::TerranTwoBaseMMMFrameOpening,
          SuccessValue, "Scheduler processor should initialize the default Terran opening plan.");
    Check(GameStateDescriptorValue.OpeningPlanExecutionState.LifecycleState == EOpeningPlanLifecycleState::Active,
          SuccessValue, "Scheduler processor should mark the opening plan active after initialization.");
    Check(GameStateDescriptorValue.CommandAuthoritySchedulingState.GetOrderCount() >= 2U, SuccessValue,
          "Before the first explicit step, the scheduler should seed worker production and supporting startup work.");
    Check(GameStateDescriptorValue.CommandAuthoritySchedulingState.ProcessorState != EPlanningProcessorState::Idle,
          SuccessValue, "Seeded strategic and planning work should keep the scheduler active.");

    size_t WorkerStrategicOrderIndexValue = 0U;
    Check(TryFindStrategicOrderByTaskType(GameStateDescriptorValue.CommandAuthoritySchedulingState,
                                          ECommandTaskType::WorkerProduction, WorkerStrategicOrderIndexValue),
          SuccessValue, "Scheduler processor should seed an active strategic worker-production order.");
    const FCommandOrderRecord WorkerStrategicOrderValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState.GetOrderRecord(WorkerStrategicOrderIndexValue);
    Check(WorkerStrategicOrderValue.TargetCount > 12U, SuccessValue,
          "Strategic worker-production orders should target growth beyond the current worker count.");

    GameStateDescriptorValue.CurrentGameLoop = 358U;
    CommandAuthorityProcessorValue.ProcessSchedulerStep(GameStateDescriptorValue);

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
    Check(DepotStrategicOrderValue.PreferredPlacementSlotType == EBuildPlacementSlotType::MainRampDepotLeft,
          SuccessValue, "The seeded depot strategic order should preserve its preferred ramp slot.");

    size_t DepotChildOrderIndexValue = 0U;
    Check(GameStateDescriptorValue.CommandAuthoritySchedulingState.TryGetChildOrderIndex(
              DepotStrategicOrderValue.OrderId, ECommandAuthorityLayer::EconomyAndProduction, DepotChildOrderIndexValue),
          SuccessValue, "Each strategic opening-plan step should spawn exactly one economy child order.");
    const FCommandOrderRecord DepotChildOrderValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState.GetOrderRecord(DepotChildOrderIndexValue);
    Check(DepotChildOrderValue.PlanStepId == 1U, SuccessValue,
          "Economy child orders should preserve the authored opening-plan step identifier.");
    Check(DepotChildOrderValue.PreferredPlacementSlotType == EBuildPlacementSlotType::MainRampDepotLeft,
          SuccessValue, "Economy child orders should preserve the authored preferred ramp slot.");
    const size_t OrderCountAfterDepotSeedValue = GameStateDescriptorValue.CommandAuthoritySchedulingState.GetOrderCount();

    GameStateDescriptorValue.CommandAuthoritySchedulingState.SetOrderLifecycleState(
        DepotChildOrderValue.OrderId, EOrderLifecycleState::Aborted);
    CommandAuthorityProcessorValue.ProcessSchedulerStep(GameStateDescriptorValue);
    Check(GameStateDescriptorValue.CommandAuthoritySchedulingState.GetOrderCount() >=
              (OrderCountAfterDepotSeedValue + 1U),
          SuccessValue, "Aborted economy children should not block the strategic parent from creating a replacement child.");

    size_t ReplacementDepotChildOrderIndexValue = 0U;
    Check(GameStateDescriptorValue.CommandAuthoritySchedulingState.TryGetActiveChildOrderIndex(
              DepotStrategicOrderValue.OrderId, ECommandAuthorityLayer::EconomyAndProduction,
              ReplacementDepotChildOrderIndexValue),
          SuccessValue, "Opening-plan parents should expose the replacement active economy child after an abort.");
    Check(GameStateDescriptorValue.CommandAuthoritySchedulingState.OrderIds[ReplacementDepotChildOrderIndexValue] !=
              DepotChildOrderValue.OrderId,
          SuccessValue, "Replacement active child lookup should return the new economy child order.");
    const FCommandOrderRecord ReplacementDepotChildOrderValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState.GetOrderRecord(ReplacementDepotChildOrderIndexValue);
    Check(ReplacementDepotChildOrderValue.PreferredPlacementSlotType == EBuildPlacementSlotType::MainRampDepotLeft,
          SuccessValue, "Replacement economy children should preserve the authored preferred ramp slot.");
    const size_t OrderCountAfterReplacementValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState.GetOrderCount();

    CommandAuthorityProcessorValue.ProcessSchedulerStep(GameStateDescriptorValue);
    Check(GameStateDescriptorValue.CommandAuthoritySchedulingState.GetOrderCount() == OrderCountAfterReplacementValue,
          SuccessValue,
          "Repeated scheduler passes without new observations should not duplicate seeded orders.");

    GameStateDescriptorValue.CurrentGameLoop = 896U;
    CommandAuthorityProcessorValue.ProcessSchedulerStep(GameStateDescriptorValue);
    size_t BarracksStrategicOrderIndexValue = 0U;
    Check(!TryFindOrderIndexByPlanStepId(GameStateDescriptorValue.CommandAuthoritySchedulingState, 2U,
                                         ECommandAuthorityLayer::StrategicDirector, BarracksStrategicOrderIndexValue),
          SuccessValue, "Dependency-gated steps should stay deferred until their prerequisite step completes.");

    GameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetBuildingIndex(UNIT_TYPEID::TERRAN_SUPPLYDEPOT)] = 1U;
    SetObservedPlacementSlotState(GameStateDescriptorValue, EBuildPlacementSlotType::MainRampDepotLeft, 0U,
                                  EObservedWallSlotState::Occupied);
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
    Check(BarracksStrategicOrderValue.PreferredPlacementSlotType ==
              EBuildPlacementSlotType::MainRampBarracksWithAddon,
          SuccessValue, "The seeded barracks strategic order should preserve the ramp barracks slot binding.");

    size_t RefineryStrategicOrderIndexValue = 0U;
    Check(!TryFindOrderIndexByPlanStepId(GameStateDescriptorValue.CommandAuthoritySchedulingState, 3U,
                                         ECommandAuthorityLayer::StrategicDirector, RefineryStrategicOrderIndexValue),
          SuccessValue, "Later authored steps should not seed before their minimum game loop is reached.");

    GameStateDescriptorValue.CurrentGameLoop = 2464U;
    CommandAuthorityProcessorValue.ProcessSchedulerStep(GameStateDescriptorValue);
    size_t WallDepotRightStrategicOrderIndexValue = 0U;
    Check(!TryFindOrderIndexByPlanStepId(GameStateDescriptorValue.CommandAuthoritySchedulingState, 8U,
                                         ECommandAuthorityLayer::StrategicDirector,
                                         WallDepotRightStrategicOrderIndexValue),
          SuccessValue,
          "The second wall depot step should stay blocked until the mandatory wall barracks step completes.");

    GameStateDescriptorValue.CurrentGameLoop = 5040U;
    CommandAuthorityProcessorValue.ProcessSchedulerStep(GameStateDescriptorValue);
    size_t FlexibleDepotStrategicOrderIndexValue = 0U;
    Check(!TryFindOrderIndexByPlanStepId(GameStateDescriptorValue.CommandAuthoritySchedulingState, 25U,
                                         ECommandAuthorityLayer::StrategicDirector,
                                         FlexibleDepotStrategicOrderIndexValue),
          SuccessValue,
          "Later generic opening depots should stay blocked while the mandatory wall chain is unresolved.");

    FGameStateDescriptor NoSupplyPressureOpeningGameStateDescriptorValue;
    NoSupplyPressureOpeningGameStateDescriptorValue.BuildPlanning.ObservedTownHallCount = 1U;
    NoSupplyPressureOpeningGameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetBuildingIndex(
        UNIT_TYPEID::TERRAN_SUPPLYDEPOT)] = 2U;
    NoSupplyPressureOpeningGameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetBuildingIndex(
        UNIT_TYPEID::TERRAN_BARRACKS)] = 1U;
    NoSupplyPressureOpeningGameStateDescriptorValue.MacroState.ActiveBaseCount = 1U;
    NoSupplyPressureOpeningGameStateDescriptorValue.MacroState.WorkerCount = 18U;
    NoSupplyPressureOpeningGameStateDescriptorValue.MacroState.BarracksCount = 1U;
    NoSupplyPressureOpeningGameStateDescriptorValue.MacroState.SupplyUsed = 18U;
    NoSupplyPressureOpeningGameStateDescriptorValue.MacroState.SupplyCap = 31U;
    SetObservedWorkerCount(NoSupplyPressureOpeningGameStateDescriptorValue, 18U);
    SetProjectedDiscretionaryBudget(NoSupplyPressureOpeningGameStateDescriptorValue, 500U, 100U, 13U);
    SetObservedPlacementSlotState(NoSupplyPressureOpeningGameStateDescriptorValue,
                                  EBuildPlacementSlotType::MainRampDepotLeft, 0U,
                                  EObservedWallSlotState::Occupied);
    SetObservedPlacementSlotState(NoSupplyPressureOpeningGameStateDescriptorValue,
                                  EBuildPlacementSlotType::MainRampBarracksWithAddon, 0U,
                                  EObservedWallSlotState::Occupied);
    SetObservedPlacementSlotState(NoSupplyPressureOpeningGameStateDescriptorValue,
                                  EBuildPlacementSlotType::MainRampDepotRight, 0U,
                                  EObservedWallSlotState::Occupied);
    NoSupplyPressureOpeningGameStateDescriptorValue.CurrentGameLoop = 5040U;
    CommandAuthorityProcessorValue.ProcessSchedulerStep(NoSupplyPressureOpeningGameStateDescriptorValue);

    size_t NoDemandFlexibleDepotStrategicOrderIndexValue = 0U;
    Check(!TryFindOrderIndexByPlanStepId(NoSupplyPressureOpeningGameStateDescriptorValue.CommandAuthoritySchedulingState,
                                         25U, ECommandAuthorityLayer::StrategicDirector,
                                         NoDemandFlexibleDepotStrategicOrderIndexValue),
          SuccessValue,
          "Flexible opening depot steps should stay deferred when current buffered supply demand is already satisfied.");

    FGameStateDescriptor FactoryFamilyOpeningGameStateDescriptorValue;
    FactoryFamilyOpeningGameStateDescriptorValue.BuildPlanning.ObservedTownHallCount = 2U;
    FactoryFamilyOpeningGameStateDescriptorValue.MacroState.ActiveBaseCount = 2U;
    FactoryFamilyOpeningGameStateDescriptorValue.MacroState.WorkerCount = 28U;
    SetObservedWorkerCount(FactoryFamilyOpeningGameStateDescriptorValue, 28U);
    SetProjectedDiscretionaryBudget(FactoryFamilyOpeningGameStateDescriptorValue, 600U, 200U, 16U);
    FactoryFamilyOpeningGameStateDescriptorValue.CurrentGameLoop = 4390U;
    FactoryFamilyOpeningGameStateDescriptorValue.OpeningPlanExecutionState.SetActivePlan(
        EOpeningPlanId::TerranTwoBaseMMMFrameOpening);
    FactoryFamilyOpeningGameStateDescriptorValue.OpeningPlanExecutionState.WallChainState =
        EOpeningWallChainState::Completed;
    for (uint32_t CompletedStepIdValue = 1U; CompletedStepIdValue <= 14U; ++CompletedStepIdValue)
    {
        FactoryFamilyOpeningGameStateDescriptorValue.OpeningPlanExecutionState.MarkStepCompleted(CompletedStepIdValue);
    }

    CommandAuthorityProcessorValue.ProcessSchedulerStep(FactoryFamilyOpeningGameStateDescriptorValue);

    size_t HellionStrategicOrderIndexValue = 0U;
    Check(TryFindOrderIndexByPlanStepId(FactoryFamilyOpeningGameStateDescriptorValue.CommandAuthoritySchedulingState,
                                        15U, ECommandAuthorityLayer::StrategicDirector,
                                        HellionStrategicOrderIndexValue),
          SuccessValue,
          "The first factory hellion step should seed once the earlier opening prerequisites are complete.");

    size_t FactoryTechLabStrategicOrderIndexValue = 0U;
    Check(!TryFindOrderIndexByPlanStepId(FactoryFamilyOpeningGameStateDescriptorValue.CommandAuthoritySchedulingState,
                                         19U, ECommandAuthorityLayer::StrategicDirector,
                                         FactoryTechLabStrategicOrderIndexValue),
          SuccessValue,
          "The factory tech-lab step should stay deferred while the earlier opening hellion step is still incomplete.");

    FactoryFamilyOpeningGameStateDescriptorValue.OpeningPlanExecutionState.MarkStepCompleted(15U);
    FactoryFamilyOpeningGameStateDescriptorValue.CurrentGameLoop = 4391U;
    CommandAuthorityProcessorValue.ProcessSchedulerStep(FactoryFamilyOpeningGameStateDescriptorValue);
    Check(TryFindOrderIndexByPlanStepId(FactoryFamilyOpeningGameStateDescriptorValue.CommandAuthoritySchedulingState,
                                        19U, ECommandAuthorityLayer::StrategicDirector,
                                        FactoryTechLabStrategicOrderIndexValue),
          SuccessValue,
          "The factory tech-lab step should seed once the earlier opening hellion step completes.");

    FGameStateDescriptor SerializedOpeningDepotGameStateDescriptorValue;
    SerializedOpeningDepotGameStateDescriptorValue.BuildPlanning.ObservedTownHallCount = 2U;
    SerializedOpeningDepotGameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetBuildingIndex(
        UNIT_TYPEID::TERRAN_SUPPLYDEPOT)] = 3U;
    SerializedOpeningDepotGameStateDescriptorValue.MacroState.ActiveBaseCount = 2U;
    SerializedOpeningDepotGameStateDescriptorValue.MacroState.WorkerCount = 32U;
    SerializedOpeningDepotGameStateDescriptorValue.MacroState.SupplyUsed = 40U;
    SerializedOpeningDepotGameStateDescriptorValue.MacroState.SupplyCap = 47U;
    SetObservedWorkerCount(SerializedOpeningDepotGameStateDescriptorValue, 32U);
    SetProjectedDiscretionaryBudget(SerializedOpeningDepotGameStateDescriptorValue, 700U, 150U, 2U);
    SerializedOpeningDepotGameStateDescriptorValue.CurrentGameLoop = 6070U;
    SerializedOpeningDepotGameStateDescriptorValue.OpeningPlanExecutionState.SetActivePlan(
        EOpeningPlanId::TerranTwoBaseMMMFrameOpening);
    SerializedOpeningDepotGameStateDescriptorValue.OpeningPlanExecutionState.WallChainState =
        EOpeningWallChainState::Completed;
    for (uint32_t CompletedStepIdValue = 1U; CompletedStepIdValue <= 24U; ++CompletedStepIdValue)
    {
        SerializedOpeningDepotGameStateDescriptorValue.OpeningPlanExecutionState.MarkStepCompleted(
            CompletedStepIdValue);
    }

    CommandAuthorityProcessorValue.ProcessSchedulerStep(SerializedOpeningDepotGameStateDescriptorValue);
    Check(CountActiveStrategicOpeningSupplyOrders(
              SerializedOpeningDepotGameStateDescriptorValue.CommandAuthoritySchedulingState) == 1U,
          SuccessValue,
          "Only one flexible opening depot step should stay active at a time when later depot steps are also eligible.");
    size_t FirstFlexibleDepotStrategicOrderIndexValue = 0U;
    Check(TryFindOrderIndexByPlanStepId(SerializedOpeningDepotGameStateDescriptorValue.CommandAuthoritySchedulingState,
                                        25U, ECommandAuthorityLayer::StrategicDirector,
                                        FirstFlexibleDepotStrategicOrderIndexValue),
          SuccessValue, "The first eligible flexible opening depot step should seed first.");
    size_t SecondFlexibleDepotStrategicOrderIndexValue = 0U;
    Check(!TryFindOrderIndexByPlanStepId(SerializedOpeningDepotGameStateDescriptorValue.CommandAuthoritySchedulingState,
                                         30U, ECommandAuthorityLayer::StrategicDirector,
                                         SecondFlexibleDepotStrategicOrderIndexValue),
          SuccessValue,
          "Later flexible opening depot steps should wait while an earlier flexible depot step is still outstanding.");
    size_t ThirdFlexibleDepotStrategicOrderIndexValue = 0U;
    Check(!TryFindOrderIndexByPlanStepId(SerializedOpeningDepotGameStateDescriptorValue.CommandAuthoritySchedulingState,
                                         33U, ECommandAuthorityLayer::StrategicDirector,
                                         ThirdFlexibleDepotStrategicOrderIndexValue),
          SuccessValue,
          "The scheduler should not seed a second later flexible depot while the first one remains active.");

    const uint32_t FirstFlexibleDepotOrderIdValue =
        SerializedOpeningDepotGameStateDescriptorValue.CommandAuthoritySchedulingState
            .OrderIds[FirstFlexibleDepotStrategicOrderIndexValue];
    SerializedOpeningDepotGameStateDescriptorValue.OpeningPlanExecutionState.MarkStepCompleted(25U);
    SerializedOpeningDepotGameStateDescriptorValue.CommandAuthoritySchedulingState.SetOrderLifecycleState(
        FirstFlexibleDepotOrderIdValue, EOrderLifecycleState::Completed);
    SerializedOpeningDepotGameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetBuildingIndex(
        UNIT_TYPEID::TERRAN_SUPPLYDEPOT)] = 4U;
    SerializedOpeningDepotGameStateDescriptorValue.CurrentGameLoop = 6071U;
    CommandAuthorityProcessorValue.ProcessSchedulerStep(SerializedOpeningDepotGameStateDescriptorValue);
    Check(TryFindOrderIndexByPlanStepId(SerializedOpeningDepotGameStateDescriptorValue.CommandAuthoritySchedulingState,
                                        30U, ECommandAuthorityLayer::StrategicDirector,
                                        SecondFlexibleDepotStrategicOrderIndexValue),
          SuccessValue,
          "Once the earlier flexible depot step completes, the next eligible flexible depot step should seed.");

    FGameStateDescriptor OpeningOnlyGameStateDescriptorValue;
    OpeningOnlyGameStateDescriptorValue.BuildPlanning.ObservedTownHallCount = 1U;
    OpeningOnlyGameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetBuildingIndex(
        UNIT_TYPEID::TERRAN_SUPPLYDEPOT)] = 1U;
    OpeningOnlyGameStateDescriptorValue.MacroState.ActiveBaseCount = 1U;
    OpeningOnlyGameStateDescriptorValue.MacroState.WorkerCount = 12U;
    SetObservedWorkerCount(OpeningOnlyGameStateDescriptorValue, 12U);
    SetProjectedDiscretionaryBudget(OpeningOnlyGameStateDescriptorValue, 500U, 100U, 7U);
    SetObservedPlacementSlotState(OpeningOnlyGameStateDescriptorValue, EBuildPlacementSlotType::MainRampDepotLeft, 0U,
                                  EObservedWallSlotState::Occupied);
    OpeningOnlyGameStateDescriptorValue.CurrentGameLoop = 941U;
    CommandAuthorityProcessorValue.ProcessSchedulerStep(OpeningOnlyGameStateDescriptorValue);
    Check(TryFindOrderIndexByPlanStepId(OpeningOnlyGameStateDescriptorValue.CommandAuthoritySchedulingState, 3U,
                                        ECommandAuthorityLayer::StrategicDirector, RefineryStrategicOrderIndexValue),
          SuccessValue, "Opening-only scheduling should seed later authored steps once their frame gate is reached.");

    GameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetBuildingIndex(UNIT_TYPEID::TERRAN_BARRACKS)] = 1U;
    SetObservedPlacementSlotState(GameStateDescriptorValue, EBuildPlacementSlotType::MainRampBarracksWithAddon, 0U,
                                  EObservedWallSlotState::Occupied);
    CommandAuthorityProcessorValue.ProcessSchedulerStep(GameStateDescriptorValue);
    Check(GameStateDescriptorValue.OpeningPlanExecutionState.IsStepCompleted(2U), SuccessValue,
          "Observed barracks completion should complete the second authored opening-plan step.");

    FGameStateDescriptor ReactorPendingWallDepotGameStateDescriptorValue;
    ReactorPendingWallDepotGameStateDescriptorValue.BuildPlanning.ObservedTownHallCount = 1U;
    ReactorPendingWallDepotGameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetBuildingIndex(
        UNIT_TYPEID::TERRAN_SUPPLYDEPOT)] = 1U;
    ReactorPendingWallDepotGameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetBuildingIndex(
        UNIT_TYPEID::TERRAN_BARRACKS)] = 1U;
    ReactorPendingWallDepotGameStateDescriptorValue.MacroState.ActiveBaseCount = 1U;
    ReactorPendingWallDepotGameStateDescriptorValue.MacroState.WorkerCount = 16U;
    ReactorPendingWallDepotGameStateDescriptorValue.MacroState.BarracksCount = 1U;
    SetObservedWorkerCount(ReactorPendingWallDepotGameStateDescriptorValue, 16U);
    SetProjectedDiscretionaryBudget(ReactorPendingWallDepotGameStateDescriptorValue, 500U, 100U, 7U);
    SetObservedPlacementSlotState(ReactorPendingWallDepotGameStateDescriptorValue,
                                  EBuildPlacementSlotType::MainRampDepotLeft, 0U,
                                  EObservedWallSlotState::Occupied);
    SetObservedPlacementSlotState(ReactorPendingWallDepotGameStateDescriptorValue,
                                  EBuildPlacementSlotType::MainRampBarracksWithAddon, 0U,
                                  EObservedWallSlotState::Occupied);
    ReactorPendingWallDepotGameStateDescriptorValue.CurrentGameLoop = 2464U;
    CommandAuthorityProcessorValue.ProcessSchedulerStep(ReactorPendingWallDepotGameStateDescriptorValue);

    size_t BarracksReactorStrategicOrderIndexValue = 0U;
    Check(!TryFindOrderIndexByPlanStepId(ReactorPendingWallDepotGameStateDescriptorValue.CommandAuthoritySchedulingState,
                                         7U, ECommandAuthorityLayer::StrategicDirector,
                                         BarracksReactorStrategicOrderIndexValue),
          SuccessValue,
          "The mandatory barracks reactor step should stay deferred until the earlier opening marine completes on the same barracks family.");

    size_t ReactorPendingWallDepotStrategicOrderIndexValue = 0U;
    Check(TryFindOrderIndexByPlanStepId(ReactorPendingWallDepotGameStateDescriptorValue.CommandAuthoritySchedulingState,
                                        8U, ECommandAuthorityLayer::StrategicDirector,
                                        ReactorPendingWallDepotStrategicOrderIndexValue),
          SuccessValue,
          "The second wall depot should still seed while the mandatory barracks reactor is pending.");

    ReactorPendingWallDepotGameStateDescriptorValue.BuildPlanning.ObservedUnitCounts[GetUnitIndex(
        UNIT_TYPEID::TERRAN_MARINE)] = 1U;
    ReactorPendingWallDepotGameStateDescriptorValue.CurrentGameLoop = 2465U;
    CommandAuthorityProcessorValue.ProcessSchedulerStep(ReactorPendingWallDepotGameStateDescriptorValue);
    Check(TryFindOrderIndexByPlanStepId(ReactorPendingWallDepotGameStateDescriptorValue.CommandAuthoritySchedulingState,
                                        7U, ECommandAuthorityLayer::StrategicDirector,
                                        BarracksReactorStrategicOrderIndexValue),
          SuccessValue,
          "The mandatory barracks reactor step should seed once the earlier opening marine step is observed complete.");

    FGameStateDescriptor WallSlotGameStateDescriptorValue;
    WallSlotGameStateDescriptorValue.BuildPlanning.ObservedTownHallCount = 1U;
    SetObservedWorkerCount(WallSlotGameStateDescriptorValue, 12U);
    SetProjectedDiscretionaryBudget(WallSlotGameStateDescriptorValue, 500U, 100U, 7U);
    WallSlotGameStateDescriptorValue.RampWallDescriptor.bIsValid = true;
    SetObservedPlacementSlotState(WallSlotGameStateDescriptorValue, EBuildPlacementSlotType::MainRampDepotLeft, 0U,
                                  EObservedWallSlotState::Empty);
    SetObservedPlacementSlotState(WallSlotGameStateDescriptorValue,
                                  EBuildPlacementSlotType::MainRampBarracksWithAddon, 0U,
                                  EObservedWallSlotState::Empty);
    SetObservedPlacementSlotState(WallSlotGameStateDescriptorValue, EBuildPlacementSlotType::MainRampDepotRight, 0U,
                                  EObservedWallSlotState::Empty);

    WallSlotGameStateDescriptorValue.CurrentGameLoop = 357U;
    CommandAuthorityProcessorValue.ProcessSchedulerStep(WallSlotGameStateDescriptorValue);
    WallSlotGameStateDescriptorValue.CurrentGameLoop = 358U;
    CommandAuthorityProcessorValue.ProcessSchedulerStep(WallSlotGameStateDescriptorValue);

    size_t WallDepotStrategicOrderIndexValue = 0U;
    Check(TryFindOrderIndexByPlanStepId(WallSlotGameStateDescriptorValue.CommandAuthoritySchedulingState, 1U,
                                        ECommandAuthorityLayer::StrategicDirector, WallDepotStrategicOrderIndexValue),
          SuccessValue, "Slot-bound wall verification should seed the first depot strategic order.");
    const FCommandOrderRecord WallDepotStrategicOrderValue =
        WallSlotGameStateDescriptorValue.CommandAuthoritySchedulingState.GetOrderRecord(WallDepotStrategicOrderIndexValue);
    size_t InitialWallDepotChildOrderIndexValue = 0U;
    Check(WallSlotGameStateDescriptorValue.CommandAuthoritySchedulingState.TryGetChildOrderIndex(
              WallDepotStrategicOrderValue.OrderId, ECommandAuthorityLayer::EconomyAndProduction,
              InitialWallDepotChildOrderIndexValue),
          SuccessValue, "Slot-bound wall verification should create the first depot economy child.");
    const FCommandOrderRecord InitialWallDepotChildOrderValue =
        WallSlotGameStateDescriptorValue.CommandAuthoritySchedulingState.GetOrderRecord(
            InitialWallDepotChildOrderIndexValue);

    WallSlotGameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetBuildingIndex(UNIT_TYPEID::TERRAN_SUPPLYDEPOT)] =
        1U;
    CommandAuthorityProcessorValue.ProcessSchedulerStep(WallSlotGameStateDescriptorValue);
    Check(!WallSlotGameStateDescriptorValue.OpeningPlanExecutionState.IsStepCompleted(1U), SuccessValue,
          "Slot-bound wall steps should not complete from aggregate depot counts when the exact wall slot is empty.");

    SetObservedPlacementSlotState(WallSlotGameStateDescriptorValue, EBuildPlacementSlotType::MainRampDepotLeft, 0U,
                                  EObservedWallSlotState::Occupied);
    CommandAuthorityProcessorValue.ProcessSchedulerStep(WallSlotGameStateDescriptorValue);
    Check(WallSlotGameStateDescriptorValue.OpeningPlanExecutionState.IsStepCompleted(1U), SuccessValue,
          "Slot-bound wall steps should complete once the exact wall slot is occupied.");

    SetObservedPlacementSlotState(WallSlotGameStateDescriptorValue, EBuildPlacementSlotType::MainRampDepotLeft, 0U,
                                  EObservedWallSlotState::Empty);
    CommandAuthorityProcessorValue.ProcessSchedulerStep(WallSlotGameStateDescriptorValue);
    Check(!WallSlotGameStateDescriptorValue.OpeningPlanExecutionState.IsStepCompleted(1U), SuccessValue,
          "Destroyed wall structures should regress slot-bound opening-plan step completion.");
    size_t RebuiltWallDepotChildOrderIndexValue = 0U;
    Check(WallSlotGameStateDescriptorValue.CommandAuthoritySchedulingState.TryGetActiveChildOrderIndex(
              WallDepotStrategicOrderValue.OrderId, ECommandAuthorityLayer::EconomyAndProduction,
              RebuiltWallDepotChildOrderIndexValue),
          SuccessValue, "Regressed slot-bound wall steps should respawn an active economy child.");
    Check(WallSlotGameStateDescriptorValue.CommandAuthoritySchedulingState.OrderIds[RebuiltWallDepotChildOrderIndexValue] !=
              InitialWallDepotChildOrderValue.OrderId,
          SuccessValue, "Regressed slot-bound wall steps should create a replacement economy child order.");

    FGameStateDescriptor GoalDrivenFamilyFreedomGameStateDescriptorValue;
    GoalDrivenFamilyFreedomGameStateDescriptorValue.BuildPlanning.ObservedTownHallCount = 1U;
    GoalDrivenFamilyFreedomGameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetBuildingIndex(
        UNIT_TYPEID::TERRAN_SUPPLYDEPOT)] = 1U;
    GoalDrivenFamilyFreedomGameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetBuildingIndex(
        UNIT_TYPEID::TERRAN_BARRACKS)] = 1U;
    GoalDrivenFamilyFreedomGameStateDescriptorValue.MacroState.ActiveBaseCount = 1U;
    GoalDrivenFamilyFreedomGameStateDescriptorValue.MacroState.WorkerCount = 20U;
    GoalDrivenFamilyFreedomGameStateDescriptorValue.MacroState.BarracksCount = 1U;
    SetObservedWorkerCount(GoalDrivenFamilyFreedomGameStateDescriptorValue, 20U);
    SetProjectedDiscretionaryBudget(GoalDrivenFamilyFreedomGameStateDescriptorValue, 1000U, 500U, 20U);
    GoalDrivenFamilyFreedomGameStateDescriptorValue.CommitmentLedger.ProjectedDiscretionaryMineralsByHorizon[
        ShortForecastHorizonIndexValue] = 1000U;
    GoalDrivenFamilyFreedomGameStateDescriptorValue.CommitmentLedger.ProjectedDiscretionaryVespeneByHorizon[
        ShortForecastHorizonIndexValue] = 500U;
    GoalDrivenFamilyFreedomGameStateDescriptorValue.CommitmentLedger.ProjectedDiscretionarySupplyByHorizon[
        ShortForecastHorizonIndexValue] = 20U;
    GoalDrivenFamilyFreedomGameStateDescriptorValue.CurrentGameLoop = 2200U;
    GoalDrivenFamilyFreedomGameStateDescriptorValue.GoalSet.NearTermGoals.push_back(CreateGoalDescriptor(
        901U, EGoalDomain::Technology, EGoalHorizon::NearTerm, EGoalType::UnlockTechnology, EGoalStatus::Active, 160,
        1U, UNIT_TYPEID::TERRAN_STARPORTREACTOR));
    GoalDrivenFamilyFreedomGameStateDescriptorValue.GoalSet.NearTermGoals.push_back(CreateGoalDescriptor(
        902U, EGoalDomain::Technology, EGoalHorizon::NearTerm, EGoalType::ResearchUpgrade, EGoalStatus::Active, 155,
        1U, UNIT_TYPEID::INVALID, UpgradeID(UPGRADE_ID::STIMPACK)));
    GoalDrivenFamilyFreedomGameStateDescriptorValue.GoalSet.StrategicGoals.push_back(CreateGoalDescriptor(
        903U, EGoalDomain::Army, EGoalHorizon::Strategic, EGoalType::ProduceArmy, EGoalStatus::Active, 150, 1U,
        UNIT_TYPEID::TERRAN_MEDIVAC));

    CommandAuthorityProcessorValue.ProcessSchedulerStep(GoalDrivenFamilyFreedomGameStateDescriptorValue);

    size_t StarportReactorStrategicOrderIndexValue = 0U;
    Check(TryFindStrategicOrderByGoalId(GoalDrivenFamilyFreedomGameStateDescriptorValue.CommandAuthoritySchedulingState,
                                        901U, StarportReactorStrategicOrderIndexValue),
          SuccessValue,
          "Goal-driven starport add-on work should be allowed when no seeded or presently admissible opening starport-family task owns the producer family.");

    size_t StimpackStrategicOrderIndexValue = 0U;
    Check(TryFindStrategicOrderByGoalId(GoalDrivenFamilyFreedomGameStateDescriptorValue.CommandAuthoritySchedulingState,
                                        902U, StimpackStrategicOrderIndexValue),
          SuccessValue,
          "Goal-driven upgrade work should be allowed when no seeded or presently admissible opening barracks-family task owns the producer family.");

    size_t MedivacStrategicOrderIndexValue = 0U;
    Check(TryFindStrategicOrderByGoalId(GoalDrivenFamilyFreedomGameStateDescriptorValue.CommandAuthoritySchedulingState,
                                        903U, MedivacStrategicOrderIndexValue),
          SuccessValue,
          "Goal-driven unit production should be allowed when no seeded or presently admissible opening producer-family task owns that family.");

    FGameStateDescriptor DelayedBarracksFamilyReleaseGameStateDescriptorValue;
    DelayedBarracksFamilyReleaseGameStateDescriptorValue.BuildPlanning.ObservedTownHallCount = 2U;
    DelayedBarracksFamilyReleaseGameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetBuildingIndex(
        UNIT_TYPEID::TERRAN_SUPPLYDEPOT)] = 3U;
    DelayedBarracksFamilyReleaseGameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetBuildingIndex(
        UNIT_TYPEID::TERRAN_BARRACKS)] = 2U;
    DelayedBarracksFamilyReleaseGameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetBuildingIndex(
        UNIT_TYPEID::TERRAN_STARPORT)] = 1U;
    DelayedBarracksFamilyReleaseGameStateDescriptorValue.MacroState.ActiveBaseCount = 2U;
    DelayedBarracksFamilyReleaseGameStateDescriptorValue.MacroState.WorkerCount = 32U;
    DelayedBarracksFamilyReleaseGameStateDescriptorValue.MacroState.BarracksCount = 2U;
    DelayedBarracksFamilyReleaseGameStateDescriptorValue.MacroState.SupplyUsed = 36U;
    DelayedBarracksFamilyReleaseGameStateDescriptorValue.MacroState.SupplyCap = 47U;
    SetObservedWorkerCount(DelayedBarracksFamilyReleaseGameStateDescriptorValue, 32U);
    SetProjectedDiscretionaryBudget(DelayedBarracksFamilyReleaseGameStateDescriptorValue, 1000U, 300U, 11U);
    DelayedBarracksFamilyReleaseGameStateDescriptorValue.CurrentGameLoop = 4505U;
    DelayedBarracksFamilyReleaseGameStateDescriptorValue.OpeningPlanExecutionState.SetActivePlan(
        EOpeningPlanId::TerranTwoBaseMMMFrameOpening);
    DelayedBarracksFamilyReleaseGameStateDescriptorValue.OpeningPlanExecutionState.WallChainState =
        EOpeningWallChainState::Completed;
    for (uint32_t CompletedStepIdValue = 1U; CompletedStepIdValue <= 8U; ++CompletedStepIdValue)
    {
        DelayedBarracksFamilyReleaseGameStateDescriptorValue.OpeningPlanExecutionState.MarkStepCompleted(
            CompletedStepIdValue);
    }
    for (uint32_t CompletedStepIdValue = 10U; CompletedStepIdValue <= 17U; ++CompletedStepIdValue)
    {
        DelayedBarracksFamilyReleaseGameStateDescriptorValue.OpeningPlanExecutionState.MarkStepCompleted(
            CompletedStepIdValue);
    }
    DelayedBarracksFamilyReleaseGameStateDescriptorValue.GoalSet.StrategicGoals.push_back(CreateGoalDescriptor(
        904U, EGoalDomain::Army, EGoalHorizon::Strategic, EGoalType::ProduceArmy, EGoalStatus::Active, 150, 20U,
        UNIT_TYPEID::TERRAN_MARINE));

    CommandAuthorityProcessorValue.ProcessSchedulerStep(DelayedBarracksFamilyReleaseGameStateDescriptorValue);

    size_t ReleasedMarineStrategicOrderIndexValue = 0U;
    Check(TryFindStrategicOrderByGoalId(DelayedBarracksFamilyReleaseGameStateDescriptorValue
                                            .CommandAuthoritySchedulingState,
                                        904U, ReleasedMarineStrategicOrderIndexValue),
          SuccessValue,
          "Goal-driven marine production should seed when the next opening barracks task is stalled by an unrelated mandatory structure step.");

    return SuccessValue;
}

}  // namespace sc2
