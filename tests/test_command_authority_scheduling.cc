#include "test_command_authority_scheduling.h"

#include <cmath>
#include <iostream>
#include <string>

#include "common/agent_framework.h"
#include "common/bot_status_models.h"
#include "common/build_orders/FOpeningPlanExecutionState.h"
#include "common/descriptors/FTerranForecastStateBuilder.h"
#include "common/descriptors/FTerranGameStateDescriptorBuilder.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/economy/EconomyForecastConstants.h"
#include "common/economy/FEconomyDomainState.h"
#include "common/planning/ECommandAuthorityLayer.h"
#include "common/planning/ECommandCommitmentClass.h"
#include "common/planning/ECommandOrderDeferralReason.h"
#include "common/planning/ECommandTaskExecutionGuarantee.h"
#include "common/planning/ECommandTaskOrigin.h"
#include "common/planning/ECommandTaskRetentionPolicy.h"
#include "common/planning/EIntentPlaybackState.h"
#include "common/planning/EOrderLifecycleState.h"
#include "common/planning/EPlanningProcessorState.h"
#include "common/planning/FBlockedTaskRecord.h"
#include "common/planning/FBlockedTaskRingBuffer.h"
#include "common/planning/FCommandAuthoritySchedulingState.h"
#include "common/planning/FCommandOrderRecord.h"
#include "common/planning/FCommandTaskDescriptor.h"
#include "common/planning/FIntentSchedulingService.h"
#include "common/planning/FTerranCommandTaskAdmissionService.h"

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

bool ApproxEqual(const float LeftValue, const float RightValue)
{
    constexpr float CoordinateToleranceValue = 0.001f;
    return std::fabs(LeftValue - RightValue) <= CoordinateToleranceValue;
}

FCommandOrderRecord CreateStrategicBuildBarracksOrder(
    const uint32_t SourceGoalIdValue,
    const ECommandTaskRetentionPolicy RetentionPolicyValue = ECommandTaskRetentionPolicy::BufferedRetry)
{
    FCommandOrderRecord CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::BUILD_BARRACKS, 160,
        EIntentDomain::StructureBuild, 100U);
    CommandOrderRecordValue.SourceGoalId = SourceGoalIdValue;
    CommandOrderRecordValue.TaskPackageKind = ECommandTaskPackageKind::ProductionScale;
    CommandOrderRecordValue.TaskNeedKind = ECommandTaskNeedKind::Structure;
    CommandOrderRecordValue.TaskType = ECommandTaskType::ProductionStructure;
    CommandOrderRecordValue.Origin = ECommandTaskOrigin::GoalMacro;
    CommandOrderRecordValue.CommitmentClass = ECommandCommitmentClass::FlexibleMacro;
    CommandOrderRecordValue.ExecutionGuarantee = ECommandTaskExecutionGuarantee::Preferred;
    CommandOrderRecordValue.RetentionPolicy = RetentionPolicyValue;
    CommandOrderRecordValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
    CommandOrderRecordValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKS;
    CommandOrderRecordValue.TargetCount = 1U;
    return CommandOrderRecordValue;
}

FCommandTaskDescriptor CreateMandatoryOpeningBarracksTaskDescriptor()
{
    FCommandTaskDescriptor CommandTaskDescriptorValue;
    CommandTaskDescriptorValue.TaskId = 2U;
    CommandTaskDescriptorValue.PackageKind = ECommandTaskPackageKind::Opening;
    CommandTaskDescriptorValue.NeedKind = ECommandTaskNeedKind::Structure;
    CommandTaskDescriptorValue.ActionKind = ECommandTaskActionKind::BuildStructure;
    CommandTaskDescriptorValue.CompletionKind = ECommandTaskCompletionKind::CountAtLeast;
    CommandTaskDescriptorValue.TaskType = ECommandTaskType::ProductionStructure;
    CommandTaskDescriptorValue.Origin = ECommandTaskOrigin::Opening;
    CommandTaskDescriptorValue.CommitmentClass = ECommandCommitmentClass::MandatoryOpening;
    CommandTaskDescriptorValue.ExecutionGuarantee = ECommandTaskExecutionGuarantee::MustExecute;
    CommandTaskDescriptorValue.RetentionPolicy = ECommandTaskRetentionPolicy::HotMustRun;
    CommandTaskDescriptorValue.ActionAbilityId = ABILITY_ID::BUILD_BARRACKS;
    CommandTaskDescriptorValue.ActionProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
    CommandTaskDescriptorValue.ActionResultUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKS;
    CommandTaskDescriptorValue.ActionTargetCount = 1U;
    CommandTaskDescriptorValue.ActionRequestedQueueCount = 1U;
    CommandTaskDescriptorValue.ActionPreferredPlacementSlotType = EBuildPlacementSlotType::MainRampBarracksWithAddon;
    CommandTaskDescriptorValue.ActionPreferredPlacementSlotId.SlotType =
        EBuildPlacementSlotType::MainRampBarracksWithAddon;
    CommandTaskDescriptorValue.ActionPreferredPlacementSlotId.Ordinal = 0U;
    CommandTaskDescriptorValue.CompletionObservedCountAtLeast = 1U;
    return CommandTaskDescriptorValue;
}

FBlockedTaskRecord CreateBlockedTaskRecord(const uint32_t TaskIdValue,
                                           const ECommandTaskRetentionPolicy RetentionPolicyValue)
{
    FBlockedTaskRecord BlockedTaskRecordValue;
    BlockedTaskRecordValue.TaskId = TaskIdValue;
    BlockedTaskRecordValue.SourceGoalId = 1000U + TaskIdValue;
    BlockedTaskRecordValue.PackageKind = ECommandTaskPackageKind::ProductionScale;
    BlockedTaskRecordValue.NeedKind = ECommandTaskNeedKind::Structure;
    BlockedTaskRecordValue.TaskType = ECommandTaskType::ProductionStructure;
    BlockedTaskRecordValue.RetentionPolicy = RetentionPolicyValue;
    BlockedTaskRecordValue.AbilityId = ABILITY_ID::BUILD_BARRACKS;
    BlockedTaskRecordValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKS;
    BlockedTaskRecordValue.UpgradeId = UpgradeID(UPGRADE_ID::INVALID);
    BlockedTaskRecordValue.BlockingReason = ECommandOrderDeferralReason::NoProducer;
    BlockedTaskRecordValue.WakeKind = EBlockedTaskWakeKind::ProducerAvailability;
    BlockedTaskRecordValue.NextEligibleGameLoop = 200U;
    BlockedTaskRecordValue.LastSeenStimulusRevision = 1U;
    BlockedTaskRecordValue.RequestedQueueCount = 1U;
    return BlockedTaskRecordValue;
}

void PopulateForecastStimulusState(FGameStateDescriptor& GameStateDescriptorValue,
                                   const uint32_t ProjectedAvailableMineralsValue,
                                   const uint32_t ProjectedAvailableVespeneValue,
                                   const uint32_t ProjectedAvailableSupplyValue,
                                   const uint32_t ProjectedWorkerCountValue,
                                   const uint32_t ProjectedTownHallCountValue,
                                   const uint32_t ProjectedBarracksCountValue,
                                   const uint32_t ProjectedFactoryCountValue,
                                   const uint32_t ProjectedStarportCountValue)
{
    GameStateDescriptorValue.EconomyState.BudgetedMinerals = ProjectedAvailableMineralsValue;
    GameStateDescriptorValue.EconomyState.BudgetedVespene = ProjectedAvailableVespeneValue;
    GameStateDescriptorValue.EconomyState.BudgetedSupplyAvailable = ProjectedAvailableSupplyValue;
    GameStateDescriptorValue.EconomyState.ProjectedAvailableMineralsByHorizon[ShortForecastHorizonIndexValue] =
        ProjectedAvailableMineralsValue;
    GameStateDescriptorValue.EconomyState.ProjectedAvailableVespeneByHorizon[ShortForecastHorizonIndexValue] =
        ProjectedAvailableVespeneValue;
    GameStateDescriptorValue.EconomyState.ProjectedAvailableSupplyByHorizon[ShortForecastHorizonIndexValue] =
        ProjectedAvailableSupplyValue;
    GameStateDescriptorValue.CommitmentLedger.ProjectedAvailableAfterMandatoryMineralsByHorizon[
        ShortForecastHorizonIndexValue] = ProjectedAvailableMineralsValue;
    GameStateDescriptorValue.CommitmentLedger.ProjectedAvailableAfterMandatoryVespeneByHorizon[
        ShortForecastHorizonIndexValue] = ProjectedAvailableVespeneValue;
    GameStateDescriptorValue.CommitmentLedger.ProjectedAvailableAfterMandatorySupplyByHorizon[
        ShortForecastHorizonIndexValue] = ProjectedAvailableSupplyValue;
    GameStateDescriptorValue.CommitmentLedger.ProjectedDiscretionaryMineralsByHorizon[
        ShortForecastHorizonIndexValue] = ProjectedAvailableMineralsValue;
    GameStateDescriptorValue.CommitmentLedger.ProjectedDiscretionaryVespeneByHorizon[
        ShortForecastHorizonIndexValue] = ProjectedAvailableVespeneValue;
    GameStateDescriptorValue.CommitmentLedger.ProjectedDiscretionarySupplyByHorizon[
        ShortForecastHorizonIndexValue] = ProjectedAvailableSupplyValue;

    GameStateDescriptorValue.ProductionState.ProjectedUnitCounts[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_SCV)] =
        ProjectedWorkerCountValue;
    GameStateDescriptorValue.ProductionState.ProjectedBuildingCounts[GetTerranBuildingTypeIndex(
        UNIT_TYPEID::TERRAN_COMMANDCENTER)] = ProjectedTownHallCountValue;
    GameStateDescriptorValue.ProductionState.ProjectedBuildingCounts[GetTerranBuildingTypeIndex(
        UNIT_TYPEID::TERRAN_BARRACKS)] = ProjectedBarracksCountValue;
    GameStateDescriptorValue.ProductionState.ProjectedBuildingCounts[GetTerranBuildingTypeIndex(
        UNIT_TYPEID::TERRAN_FACTORY)] = ProjectedFactoryCountValue;
    GameStateDescriptorValue.ProductionState.ProjectedBuildingCounts[GetTerranBuildingTypeIndex(
        UNIT_TYPEID::TERRAN_STARPORT)] = ProjectedStarportCountValue;
    GameStateDescriptorValue.ProductionState.CurrentTownHallCapacity = ProjectedTownHallCountValue;
    GameStateDescriptorValue.ProductionState.NearTermTownHallCapacity = ProjectedTownHallCountValue;
    GameStateDescriptorValue.ProductionState.CurrentBarracksCapacity = ProjectedBarracksCountValue;
    GameStateDescriptorValue.ProductionState.NearTermBarracksCapacity = ProjectedBarracksCountValue;
    GameStateDescriptorValue.ProductionState.CurrentFactoryCapacity = ProjectedFactoryCountValue;
    GameStateDescriptorValue.ProductionState.NearTermFactoryCapacity = ProjectedFactoryCountValue;
    GameStateDescriptorValue.ProductionState.CurrentStarportCapacity = ProjectedStarportCountValue;
    GameStateDescriptorValue.ProductionState.NearTermStarportCapacity = ProjectedStarportCountValue;
}

}  // namespace

bool TestCommandAuthorityScheduling(int ArgC, char** ArgV)
{
    (void)ArgC;
    (void)ArgV;

    bool SuccessValue = true;

    FCommandAuthoritySchedulingState CommandAuthoritySchedulingStateValue;
    Check(CommandAuthoritySchedulingStateValue.NextOrderId == 1U, SuccessValue,
          "Scheduling state should start order identifiers at one.");
    Check(CommandAuthoritySchedulingStateValue.ProcessorState == EPlanningProcessorState::Idle, SuccessValue,
          "Scheduling processor state should default to Idle.");
    Check(CommandAuthoritySchedulingStateValue.PlaybackState == EIntentPlaybackState::Idle, SuccessValue,
          "Scheduling playback state should default to Idle.");
    Check(CommandAuthoritySchedulingStateValue.MaxStrategicOrdersPerStep == 4U, SuccessValue,
          "Scheduling state should expose the default strategic work budget.");
    Check(CommandAuthoritySchedulingStateValue.MaxUnitIntentsPerStep == 32U, SuccessValue,
          "Scheduling state should expose the default ready-intent drain budget.");

    FIntentSchedulingService IntentSchedulingServiceValue;

    const uint32_t StrategicOrderIdValue = IntentSchedulingServiceValue.SubmitOrder(
        CommandAuthoritySchedulingStateValue,
        FCommandOrderRecord::CreateNoTarget(ECommandAuthorityLayer::StrategicDirector, NullTag,
                                            ABILITY_ID::INVALID, 300, EIntentDomain::Recovery, 10U));

    const uint32_t ArmyOrderIdValue = IntentSchedulingServiceValue.SubmitOrder(
        CommandAuthoritySchedulingStateValue,
        FCommandOrderRecord::CreateNoTarget(ECommandAuthorityLayer::Army, NullTag, ABILITY_ID::INVALID, 200,
                                            EIntentDomain::ArmyCombat, 11U, 60U, StrategicOrderIdValue, 0, -1));

    const uint32_t SquadOrderIdValue = IntentSchedulingServiceValue.SubmitOrder(
        CommandAuthoritySchedulingStateValue,
        FCommandOrderRecord::CreateNoTarget(ECommandAuthorityLayer::Squad, NullTag, ABILITY_ID::INVALID, 180,
                                            EIntentDomain::ArmyCombat, 12U, 60U, ArmyOrderIdValue, 0, 1));

    const uint32_t FirstUnitOrderIdValue = IntentSchedulingServiceValue.SubmitOrder(
        CommandAuthoritySchedulingStateValue,
        FCommandOrderRecord::CreatePointTarget(ECommandAuthorityLayer::UnitExecution, 701U,
                                               ABILITY_ID::ATTACK_ATTACK, Point2D(30.0f, 44.0f), 100,
                                               EIntentDomain::ArmyCombat, 13U, 80U, SquadOrderIdValue, 0, 1, true,
                                               false, false));

    const uint32_t SecondUnitOrderIdValue = IntentSchedulingServiceValue.SubmitOrder(
        CommandAuthoritySchedulingStateValue,
        FCommandOrderRecord::CreatePointTarget(ECommandAuthorityLayer::UnitExecution, 702U,
                                               ABILITY_ID::MOVE_MOVE, Point2D(34.0f, 48.0f), 90,
                                               EIntentDomain::ArmyCombat, 14U, 80U, SquadOrderIdValue, 0, 1, true,
                                               false, false));

    Check(CommandAuthoritySchedulingStateValue.GetOrderCount() == 5U, SuccessValue,
          "Scheduling state should store all submitted order records.");
    Check(CommandAuthoritySchedulingStateValue.StrategicOrderIndices.size() == 1U, SuccessValue,
          "Strategic orders should be tracked in their own queue.");
    Check(CommandAuthoritySchedulingStateValue.ArmyOrderIndices.size() == 1U, SuccessValue,
          "Army orders should be tracked in their own queue.");
    Check(CommandAuthoritySchedulingStateValue.SquadOrderIndices.size() == 1U, SuccessValue,
          "Squad orders should be tracked in their own queue.");
    Check(CommandAuthoritySchedulingStateValue.PlanningProcessIndices.size() == 2U, SuccessValue,
          "Queued unit-execution work should stay in the planning process queue until ready.");
    Check(CommandAuthoritySchedulingStateValue.ProcessorState == EPlanningProcessorState::Processing, SuccessValue,
          "Queued work should place the processor in the Processing state.");
    Check(CommandAuthoritySchedulingStateValue.PlaybackState == EIntentPlaybackState::Blocked, SuccessValue,
          "Playback should remain blocked while queued work has not reached the ready buffer.");

    size_t FirstUnitOrderIndexValue = 0U;
    Check(CommandAuthoritySchedulingStateValue.TryGetOrderIndex(FirstUnitOrderIdValue, FirstUnitOrderIndexValue),
          SuccessValue, "Scheduling state should map order identifiers back to their authoritative index.");
    const FCommandOrderRecord FirstUnitOrderRecordValue =
        CommandAuthoritySchedulingStateValue.GetOrderRecord(FirstUnitOrderIndexValue);
    Check(FirstUnitOrderRecordValue.ParentOrderId == SquadOrderIdValue, SuccessValue,
          "Unit orders should preserve their parent order identifier.");
    Check(FirstUnitOrderRecordValue.SourceLayer == ECommandAuthorityLayer::UnitExecution, SuccessValue,
          "Unit orders should reconstruct their source authority layer.");
    Check(FirstUnitOrderRecordValue.IntentDomain == EIntentDomain::ArmyCombat, SuccessValue,
          "Unit orders should reconstruct their intent domain.");
    Check(FirstUnitOrderRecordValue.RequiresPathingValidation, SuccessValue,
          "Unit orders should preserve their pathing validation requirement.");
    Check(FirstUnitOrderRecordValue.PlanStepId == 0U, SuccessValue,
          "Existing orders without an opening-plan source should default the plan step identifier to zero.");
    Check(FirstUnitOrderRecordValue.TargetCount == 0U, SuccessValue,
          "Existing orders without an authored target count should default that count to zero.");
    Check(FirstUnitOrderRecordValue.RequestedQueueCount == 1U, SuccessValue,
          "Existing orders without authored queue metadata should default requested queue count to one.");
    Check(FirstUnitOrderRecordValue.CommitmentClass == ECommandCommitmentClass::FlexibleMacro, SuccessValue,
          "Existing orders without commitment metadata should default to flexible macro commitment.");
    Check(FirstUnitOrderRecordValue.ExecutionGuarantee == ECommandTaskExecutionGuarantee::Preferred, SuccessValue,
          "Existing orders without guarantee metadata should default to preferred execution.");
    Check(FirstUnitOrderRecordValue.ProducerUnitTypeId == UNIT_TYPEID::INVALID, SuccessValue,
          "Existing orders without a producer type should default the producer type to invalid.");
    Check(FirstUnitOrderRecordValue.ResultUnitTypeId == UNIT_TYPEID::INVALID, SuccessValue,
          "Existing orders without a result type should default the result type to invalid.");
    Check(FirstUnitOrderRecordValue.PreferredPlacementSlotType == EBuildPlacementSlotType::Unknown, SuccessValue,
          "Existing orders without slot metadata should default the preferred placement slot type to unknown.");
    Check(!FirstUnitOrderRecordValue.PreferredPlacementSlotId.IsValid(), SuccessValue,
          "Existing orders without slot metadata should default the preferred placement slot id to invalid.");
    Check(!FirstUnitOrderRecordValue.ReservedPlacementSlotId.IsValid(), SuccessValue,
          "Existing orders without slot metadata should default the reserved placement slot to invalid.");
    Check(FirstUnitOrderRecordValue.LastDeferralReason == ECommandOrderDeferralReason::None, SuccessValue,
          "Existing orders should default the scheduler deferral reason to None.");
    Check(FirstUnitOrderRecordValue.DispatchAttemptCount == 0U, SuccessValue,
          "Existing orders should default dispatch attempts to zero.");

    FBuildPlacementSlotId ReservedPlacementSlotIdValue;
    ReservedPlacementSlotIdValue.SlotType = EBuildPlacementSlotType::MainRampDepotLeft;
    ReservedPlacementSlotIdValue.Ordinal = 0U;
    Check(CommandAuthoritySchedulingStateValue.SetOrderReservedPlacementSlot(FirstUnitOrderIdValue,
                                                                             ReservedPlacementSlotIdValue),
          SuccessValue, "Scheduling state should accept reserved placement-slot claims for active orders.");
    const FCommandOrderRecord ReservedFirstUnitOrderRecordValue =
        CommandAuthoritySchedulingStateValue.GetOrderRecord(FirstUnitOrderIndexValue);
    Check(ReservedFirstUnitOrderRecordValue.ReservedPlacementSlotId == ReservedPlacementSlotIdValue, SuccessValue,
          "Scheduling state should reconstruct reserved placement-slot claims.");

    CommandAuthoritySchedulingStateValue.SetOrderDeferralState(FirstUnitOrderIdValue,
                                                               ECommandOrderDeferralReason::ProducerBusy, 15U, 140U);
    const FCommandOrderRecord DeferredUnitOrderRecordValue =
        CommandAuthoritySchedulingStateValue.GetOrderRecord(FirstUnitOrderIndexValue);
    Check(DeferredUnitOrderRecordValue.LastDeferralReason == ECommandOrderDeferralReason::ProducerBusy, SuccessValue,
          "Scheduling state should reconstruct the last deferral reason for an order.");
    Check(DeferredUnitOrderRecordValue.LastDeferralStep == 15U, SuccessValue,
          "Scheduling state should reconstruct the deferral step for an order.");
    Check(DeferredUnitOrderRecordValue.LastDeferralGameLoop == 140U, SuccessValue,
          "Scheduling state should reconstruct the deferral game loop for an order.");
    CommandAuthoritySchedulingStateValue.ClearOrderDeferralState(FirstUnitOrderIdValue);

    IntentSchedulingServiceValue.UpdateOrderLifecycleState(CommandAuthoritySchedulingStateValue, FirstUnitOrderIdValue,
                                                           EOrderLifecycleState::Ready);
    IntentSchedulingServiceValue.UpdateOrderLifecycleState(CommandAuthoritySchedulingStateValue, SecondUnitOrderIdValue,
                                                           EOrderLifecycleState::Ready);

    Check(CommandAuthoritySchedulingStateValue.ReadyIntentIndices.size() == 2U, SuccessValue,
          "Ready unit-execution work should move into the ready intent buffer.");
    Check(CommandAuthoritySchedulingStateValue.ProcessorState == EPlanningProcessorState::ReadyToDrain, SuccessValue,
          "Ready work should move the processor into the ready-to-drain state.");
    Check(CommandAuthoritySchedulingStateValue.PlaybackState == EIntentPlaybackState::ReadyBufferPending, SuccessValue,
          "Ready work should expose pending playback.");

    CommandAuthoritySchedulingStateValue.MaxUnitIntentsPerStep = 1U;
    FIntentBuffer IntentBufferValue;
    const uint32_t FirstDrainCountValue =
        IntentSchedulingServiceValue.DrainReadyIntents(CommandAuthoritySchedulingStateValue, IntentBufferValue, 4U);

    Check(FirstDrainCountValue == 1U, SuccessValue,
          "Ready-intent draining should respect the per-step unit-intent budget.");
    Check(IntentBufferValue.Intents.size() == 1U, SuccessValue,
          "Draining one ready order should append exactly one unit intent.");
    Check(IntentBufferValue.Intents.front().ActorTag == 701U, SuccessValue,
          "Drained unit intents should preserve the source actor tag.");
    Check(IntentBufferValue.Intents.front().Ability == ABILITY_ID::ATTACK_ATTACK, SuccessValue,
          "Drained unit intents should preserve the source ability.");
    Check(IntentBufferValue.Intents.front().Domain == EIntentDomain::ArmyCombat, SuccessValue,
          "Drained unit intents should preserve the intent domain.");
    Check(IntentBufferValue.Intents.front().RequiresPathingValidation, SuccessValue,
          "Drained point intents should preserve pathing validation requirements.");
    Check(ApproxEqual(IntentBufferValue.Intents.front().TargetPoint.x, 30.0f) &&
              ApproxEqual(IntentBufferValue.Intents.front().TargetPoint.y, 44.0f),
          SuccessValue, "Drained point intents should preserve the point target.");
    Check(CommandAuthoritySchedulingStateValue.ReadyIntentIndices.size() == 1U, SuccessValue,
          "Only the drained ready order should leave the ready buffer.");

    CommandAuthoritySchedulingStateValue.MaxUnitIntentsPerStep = 2U;
    const uint32_t SecondDrainCountValue =
        IntentSchedulingServiceValue.DrainReadyIntents(CommandAuthoritySchedulingStateValue, IntentBufferValue, 4U);

    Check(SecondDrainCountValue == 1U, SuccessValue,
          "The remaining ready order should drain on the next pass.");
    Check(IntentBufferValue.Intents.size() == 2U, SuccessValue,
          "The second drain should append the remaining unit intent.");
    Check(CommandAuthoritySchedulingStateValue.ReadyIntentIndices.empty(), SuccessValue,
          "Ready buffer indices should empty after all ready work is dispatched.");
    Check(CommandAuthoritySchedulingStateValue.PlaybackState == EIntentPlaybackState::Dispatching, SuccessValue,
          "Dispatched work should keep playback state in Dispatching until completion.");

    CommandAuthoritySchedulingStateValue.SetOrderDispatchState(FirstUnitOrderIdValue, 16U, 160U, 0U, 0U);
    const FCommandOrderRecord DispatchedUnitOrderRecordValue =
        CommandAuthoritySchedulingStateValue.GetOrderRecord(FirstUnitOrderIndexValue);
    Check(DispatchedUnitOrderRecordValue.DispatchStep == 16U, SuccessValue,
          "Scheduling state should reconstruct the dispatch step snapshot.");
    Check(DispatchedUnitOrderRecordValue.DispatchGameLoop == 160U, SuccessValue,
          "Scheduling state should reconstruct the dispatch game-loop snapshot.");
    Check(DispatchedUnitOrderRecordValue.DispatchAttemptCount == 1U, SuccessValue,
          "Scheduling state should count dispatch attempts explicitly.");

    IntentSchedulingServiceValue.UpdateOrderLifecycleState(CommandAuthoritySchedulingStateValue, FirstUnitOrderIdValue,
                                                           EOrderLifecycleState::Completed);
    IntentSchedulingServiceValue.UpdateOrderLifecycleState(CommandAuthoritySchedulingStateValue, SecondUnitOrderIdValue,
                                                           EOrderLifecycleState::Completed);

    Check(CommandAuthoritySchedulingStateValue.CompletedOrderIndices.size() == 2U, SuccessValue,
          "Completed unit-execution orders should move into the completed-order buffer.");
    const FCommandOrderRecord CompletedFirstUnitOrderRecordValue =
        CommandAuthoritySchedulingStateValue.GetOrderRecord(FirstUnitOrderIndexValue);
    Check(!CompletedFirstUnitOrderRecordValue.ReservedPlacementSlotId.IsValid(), SuccessValue,
          "Terminal lifecycle transitions should release reserved placement-slot claims.");

    FCommandOrderRecord OpeningPlanOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::BUILD_FACTORY, 250,
        EIntentDomain::StructureBuild, 20U, 0U, 0U, -1, -1, false);
    OpeningPlanOrderValue.PlanStepId = 19U;
    OpeningPlanOrderValue.TargetCount = 1U;
    OpeningPlanOrderValue.RequestedQueueCount = 2U;
    OpeningPlanOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
    OpeningPlanOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_FACTORY;
    OpeningPlanOrderValue.UpgradeId = UpgradeID(UPGRADE_ID::INVALID);
    OpeningPlanOrderValue.PreferredPlacementSlotType = EBuildPlacementSlotType::MainProductionWithAddon;
    OpeningPlanOrderValue.PreferredPlacementSlotId.SlotType = EBuildPlacementSlotType::MainProductionWithAddon;
    OpeningPlanOrderValue.PreferredPlacementSlotId.Ordinal = 1U;
    const uint32_t OpeningPlanOrderIdValue =
        IntentSchedulingServiceValue.SubmitOrder(CommandAuthoritySchedulingStateValue, OpeningPlanOrderValue);

    size_t OpeningPlanOrderIndexValue = 0U;
    Check(CommandAuthoritySchedulingStateValue.TryGetOrderIndex(OpeningPlanOrderIdValue, OpeningPlanOrderIndexValue),
          SuccessValue, "Scheduling state should index authored opening-plan orders.");
    const FCommandOrderRecord OpeningPlanOrderRecordValue =
        CommandAuthoritySchedulingStateValue.GetOrderRecord(OpeningPlanOrderIndexValue);
    Check(OpeningPlanOrderRecordValue.PlanStepId == 19U, SuccessValue,
          "Scheduling state should reconstruct the authored opening-plan step identifier.");
    Check(OpeningPlanOrderRecordValue.TargetCount == 1U, SuccessValue,
          "Scheduling state should reconstruct the authored opening-plan target count.");
    Check(OpeningPlanOrderRecordValue.RequestedQueueCount == 2U, SuccessValue,
          "Scheduling state should reconstruct the authored requested queue count.");
    Check(OpeningPlanOrderRecordValue.ProducerUnitTypeId == UNIT_TYPEID::TERRAN_SCV, SuccessValue,
          "Scheduling state should reconstruct the authored producer unit type.");
    Check(OpeningPlanOrderRecordValue.ResultUnitTypeId == UNIT_TYPEID::TERRAN_FACTORY, SuccessValue,
          "Scheduling state should reconstruct the authored result unit type.");
    Check(OpeningPlanOrderRecordValue.PreferredPlacementSlotType ==
              EBuildPlacementSlotType::MainProductionWithAddon,
          SuccessValue, "Scheduling state should reconstruct the authored preferred slot type for opening-plan work.");
    Check(OpeningPlanOrderRecordValue.PreferredPlacementSlotId.SlotType ==
                  EBuildPlacementSlotType::MainProductionWithAddon &&
              OpeningPlanOrderRecordValue.PreferredPlacementSlotId.Ordinal == 1U,
          SuccessValue, "Scheduling state should reconstruct the authored preferred exact slot id for opening-plan work.");

    FCommandOrderRecord EconomyChildOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::EconomyAndProduction, NullTag, ABILITY_ID::BUILD_FACTORY, 250,
        EIntentDomain::StructureBuild, 21U, 0U, OpeningPlanOrderIdValue, -1, -1, false);
    EconomyChildOrderValue.PlanStepId = 19U;
    EconomyChildOrderValue.TargetCount = 1U;
    EconomyChildOrderValue.RequestedQueueCount = 2U;
    EconomyChildOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
    EconomyChildOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_FACTORY;
    EconomyChildOrderValue.PreferredPlacementSlotType = EBuildPlacementSlotType::MainProductionWithAddon;
    EconomyChildOrderValue.PreferredPlacementSlotId.SlotType = EBuildPlacementSlotType::MainProductionWithAddon;
    EconomyChildOrderValue.PreferredPlacementSlotId.Ordinal = 2U;
    EconomyChildOrderValue.ReservedPlacementSlotId.SlotType = EBuildPlacementSlotType::MainProductionWithAddon;
    EconomyChildOrderValue.ReservedPlacementSlotId.Ordinal = 2U;
    const uint32_t EconomyChildOrderIdValue =
        IntentSchedulingServiceValue.SubmitOrder(CommandAuthoritySchedulingStateValue, EconomyChildOrderValue);

    size_t ActiveEconomyChildOrderIndexValue = 0U;
    Check(CommandAuthoritySchedulingStateValue.TryGetActiveChildOrderIndex(
              OpeningPlanOrderIdValue, ECommandAuthorityLayer::EconomyAndProduction, ActiveEconomyChildOrderIndexValue),
          SuccessValue, "Scheduling state should find active child orders for an opening-plan parent.");
    Check(CommandAuthoritySchedulingStateValue.OrderIds[ActiveEconomyChildOrderIndexValue] == EconomyChildOrderIdValue,
          SuccessValue, "Active child lookup should return the non-terminal child order.");
    const FCommandOrderRecord ActiveEconomyChildOrderValue =
        CommandAuthoritySchedulingStateValue.GetOrderRecord(ActiveEconomyChildOrderIndexValue);
    Check(ActiveEconomyChildOrderValue.PreferredPlacementSlotType ==
              EBuildPlacementSlotType::MainProductionWithAddon,
          SuccessValue, "Scheduling state should reconstruct authored preferred placement-slot metadata.");
    Check(ActiveEconomyChildOrderValue.PreferredPlacementSlotId.SlotType ==
                  EBuildPlacementSlotType::MainProductionWithAddon &&
              ActiveEconomyChildOrderValue.PreferredPlacementSlotId.Ordinal == 2U,
          SuccessValue, "Scheduling state should reconstruct authored exact preferred placement-slot ids.");
    Check(ActiveEconomyChildOrderValue.RequestedQueueCount == 2U, SuccessValue,
          "Scheduling state should reconstruct authored requested queue-count metadata.");
    Check(ActiveEconomyChildOrderValue.ReservedPlacementSlotId.SlotType ==
                  EBuildPlacementSlotType::MainProductionWithAddon &&
              ActiveEconomyChildOrderValue.ReservedPlacementSlotId.Ordinal == 2U,
          SuccessValue, "Scheduling state should reconstruct authored reserved placement-slot metadata.");

    IntentSchedulingServiceValue.UpdateOrderLifecycleState(CommandAuthoritySchedulingStateValue, EconomyChildOrderIdValue,
                                                           EOrderLifecycleState::Aborted);
    Check(!CommandAuthoritySchedulingStateValue.TryGetActiveChildOrderIndex(
              OpeningPlanOrderIdValue, ECommandAuthorityLayer::EconomyAndProduction, ActiveEconomyChildOrderIndexValue),
          SuccessValue, "Terminal child orders should not block active child lookup.");
    size_t EconomyChildOrderIndexValue = 0U;
    Check(CommandAuthoritySchedulingStateValue.TryGetOrderIndex(EconomyChildOrderIdValue, EconomyChildOrderIndexValue),
          SuccessValue, "Scheduling state should keep aborted child orders addressable by identifier.");
    const FCommandOrderRecord AbortedEconomyChildOrderValue =
        CommandAuthoritySchedulingStateValue.GetOrderRecord(EconomyChildOrderIndexValue);
    Check(!AbortedEconomyChildOrderValue.ReservedPlacementSlotId.IsValid(), SuccessValue,
          "Aborted child orders should release their reserved placement-slot claims.");

    {
        FCommandAuthoritySchedulingState BucketedSchedulingStateValue;

        FCommandOrderRecord CriticalStrategicOrderValue = FCommandOrderRecord::CreateNoTarget(
            ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::INVALID, 500,
            EIntentDomain::Recovery, 30U);
        CriticalStrategicOrderValue.PriorityTier = ECommandPriorityTier::Critical;
        CriticalStrategicOrderValue.EffectivePriorityValue = 2500;
        const uint32_t CriticalStrategicOrderIdValue =
            IntentSchedulingServiceValue.SubmitOrder(BucketedSchedulingStateValue, CriticalStrategicOrderValue);

        FCommandOrderRecord LowStrategicOrderValue = FCommandOrderRecord::CreateNoTarget(
            ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::INVALID, 100,
            EIntentDomain::Recovery, 31U);
        LowStrategicOrderValue.PriorityTier = ECommandPriorityTier::Low;
        LowStrategicOrderValue.EffectivePriorityValue = 100;
        const uint32_t LowStrategicOrderIdValue =
            IntentSchedulingServiceValue.SubmitOrder(BucketedSchedulingStateValue, LowStrategicOrderValue);

        FCommandOrderRecord HighArmyOrderValue = FCommandOrderRecord::CreateNoTarget(
            ECommandAuthorityLayer::Army, NullTag, ABILITY_ID::INVALID, 300, EIntentDomain::ArmyCombat, 32U);
        HighArmyOrderValue.PriorityTier = ECommandPriorityTier::High;
        HighArmyOrderValue.EffectivePriorityValue = 1800;
        const uint32_t HighArmyOrderIdValue =
            IntentSchedulingServiceValue.SubmitOrder(BucketedSchedulingStateValue, HighArmyOrderValue);

        FCommandOrderRecord ReadyCriticalArmyIntentOrderValue = FCommandOrderRecord::CreatePointTarget(
            ECommandAuthorityLayer::UnitExecution, 801U, ABILITY_ID::ATTACK_ATTACK, Point2D(60.0f, 20.0f), 200,
            EIntentDomain::ArmyCombat, 33U, 0U, HighArmyOrderIdValue, 0, 0, true, false, false);
        ReadyCriticalArmyIntentOrderValue.PriorityTier = ECommandPriorityTier::Critical;
        ReadyCriticalArmyIntentOrderValue.EffectivePriorityValue = 2400;
        const uint32_t ReadyCriticalArmyIntentOrderIdValue =
            IntentSchedulingServiceValue.SubmitOrder(BucketedSchedulingStateValue, ReadyCriticalArmyIntentOrderValue);

        FCommandOrderRecord ReadyLowStructureIntentOrderValue = FCommandOrderRecord::CreatePointTarget(
            ECommandAuthorityLayer::UnitExecution, 802U, ABILITY_ID::SMART, Point2D(18.0f, 18.0f), 50,
            EIntentDomain::StructureBuild, 34U, 0U, LowStrategicOrderIdValue, -1, -1, false, false, false);
        ReadyLowStructureIntentOrderValue.PriorityTier = ECommandPriorityTier::Low;
        ReadyLowStructureIntentOrderValue.EffectivePriorityValue = 120;
        const uint32_t ReadyLowStructureIntentOrderIdValue =
            IntentSchedulingServiceValue.SubmitOrder(BucketedSchedulingStateValue, ReadyLowStructureIntentOrderValue);

        size_t CriticalStrategicOrderIndexValue = 0U;
        size_t LowStrategicOrderIndexValue = 0U;
        size_t HighArmyOrderIndexValue = 0U;
        size_t ReadyCriticalArmyIntentOrderIndexValue = 0U;
        size_t ReadyLowStructureIntentOrderIndexValue = 0U;
        Check(BucketedSchedulingStateValue.TryGetOrderIndex(CriticalStrategicOrderIdValue,
                                                            CriticalStrategicOrderIndexValue),
              SuccessValue, "Bucketed scheduling test should index the critical strategic order.");
        Check(BucketedSchedulingStateValue.TryGetOrderIndex(LowStrategicOrderIdValue, LowStrategicOrderIndexValue),
              SuccessValue, "Bucketed scheduling test should index the low strategic order.");
        Check(BucketedSchedulingStateValue.TryGetOrderIndex(HighArmyOrderIdValue, HighArmyOrderIndexValue),
              SuccessValue, "Bucketed scheduling test should index the high army order.");
        Check(BucketedSchedulingStateValue.TryGetOrderIndex(ReadyCriticalArmyIntentOrderIdValue,
                                                            ReadyCriticalArmyIntentOrderIndexValue),
              SuccessValue, "Bucketed scheduling test should index the critical ready-intent order.");
        Check(BucketedSchedulingStateValue.TryGetOrderIndex(ReadyLowStructureIntentOrderIdValue,
                                                            ReadyLowStructureIntentOrderIndexValue),
              SuccessValue, "Bucketed scheduling test should index the low ready-intent order.");

        IntentSchedulingServiceValue.UpdateOrderLifecycleState(
            BucketedSchedulingStateValue, ReadyCriticalArmyIntentOrderIdValue, EOrderLifecycleState::Ready);
        IntentSchedulingServiceValue.UpdateOrderLifecycleState(
            BucketedSchedulingStateValue, ReadyLowStructureIntentOrderIdValue, EOrderLifecycleState::Ready);

        Check(BucketedSchedulingStateValue.StrategicQueues[GetCommandPriorityTierIndex(
                  ECommandPriorityTier::Critical)].size() == 1U,
              SuccessValue, "Critical strategic orders should route into the critical strategic queue.");
        Check(BucketedSchedulingStateValue.StrategicQueues[GetCommandPriorityTierIndex(ECommandPriorityTier::Low)].size() ==
                  1U,
              SuccessValue, "Low strategic orders should route into the low strategic queue.");
        Check(BucketedSchedulingStateValue.ArmyQueues[GetCommandPriorityTierIndex(ECommandPriorityTier::High)].size() ==
                  1U,
              SuccessValue, "High-priority army orders should route into the high army queue.");
        Check(BucketedSchedulingStateValue.ReadyIntentQueues[GetCommandPriorityTierIndex(ECommandPriorityTier::Critical)]
                                                           [GetIntentDomainIndex(EIntentDomain::ArmyCombat)]
                                                               .size() == 1U,
              SuccessValue, "Critical ready intents should route into the critical ArmyCombat ready-intent queue.");
        Check(BucketedSchedulingStateValue.ReadyIntentQueues[GetCommandPriorityTierIndex(ECommandPriorityTier::Low)]
                                                           [GetIntentDomainIndex(EIntentDomain::StructureBuild)]
                                                               .size() == 1U,
              SuccessValue, "Low ready intents should route into the low StructureBuild ready-intent queue.");
        Check(BucketedSchedulingStateValue.StrategicOrderIndices.front() == CriticalStrategicOrderIndexValue,
              SuccessValue, "Derived strategic order views should list critical-tier work before lower tiers.");
        Check(BucketedSchedulingStateValue.StrategicOrderIndices.back() == LowStrategicOrderIndexValue, SuccessValue,
              "Derived strategic order views should keep lower-tier work after higher tiers.");
        Check(BucketedSchedulingStateValue.ArmyOrderIndices.front() == HighArmyOrderIndexValue, SuccessValue,
              "Derived army order views should preserve the high-tier army order.");
        Check(BucketedSchedulingStateValue.ReadyIntentIndices.front() == ReadyCriticalArmyIntentOrderIndexValue,
              SuccessValue, "Ready-intent draining order should start from the highest-priority ready queue.");
        Check(BucketedSchedulingStateValue.ReadyIntentIndices.back() == ReadyLowStructureIntentOrderIndexValue,
              SuccessValue, "Ready-intent draining order should leave lower-tier ready work until later.");

        FIntentBuffer BucketedIntentBufferValue;
        BucketedSchedulingStateValue.MaxUnitIntentsPerStep = 8U;
        const uint32_t BucketedDrainCountValue =
            IntentSchedulingServiceValue.DrainReadyIntents(BucketedSchedulingStateValue, BucketedIntentBufferValue, 35U);
        Check(BucketedDrainCountValue == 2U, SuccessValue,
              "Bucketed ready-intent draining should dispatch all available work when budget allows.");
        Check(BucketedIntentBufferValue.Intents.size() == 2U, SuccessValue,
              "Bucketed ready-intent draining should append both ready intents.");
        Check(BucketedIntentBufferValue.Intents.front().ActorTag == 801U, SuccessValue,
              "Critical ready intents should drain before lower-priority ready intents.");
        Check(BucketedIntentBufferValue.Intents.back().ActorTag == 802U, SuccessValue,
              "Lower-priority ready intents should drain after higher-priority ready intents.");
    }

    {
        FCommandAuthoritySchedulingState BatchedSchedulingStateValue;
        BatchedSchedulingStateValue.BeginMutationBatch();

        const uint32_t BatchedStrategicOrderIdValue = IntentSchedulingServiceValue.SubmitOrder(
            BatchedSchedulingStateValue,
            FCommandOrderRecord::CreateNoTarget(ECommandAuthorityLayer::StrategicDirector, NullTag,
                                                ABILITY_ID::INVALID, 220, EIntentDomain::Recovery, 40U));

        const uint32_t BatchedUnitOrderIdValue = IntentSchedulingServiceValue.SubmitOrder(
            BatchedSchedulingStateValue,
            FCommandOrderRecord::CreatePointTarget(ECommandAuthorityLayer::UnitExecution, 901U,
                                                   ABILITY_ID::MOVE_MOVE, Point2D(22.0f, 22.0f), 80,
                                                   EIntentDomain::ArmyCombat, 41U, 0U,
                                                   BatchedStrategicOrderIdValue, 0, 0, true, false, false));

        Check(BatchedSchedulingStateValue.GetOrderCount() == 2U, SuccessValue,
              "Batched scheduling should append orders immediately to the authoritative store.");
        Check(BatchedSchedulingStateValue.StrategicOrderIndices.empty(), SuccessValue,
              "Derived queue views should stay unchanged until a mutation batch ends.");
        Check(BatchedSchedulingStateValue.PlanningProcessIndices.empty(), SuccessValue,
              "Planning-process views should stay unchanged until a mutation batch ends.");
        Check(BatchedSchedulingStateValue.bDerivedQueuesDirty, SuccessValue,
              "Batched scheduling should mark derived queues dirty while mutations are pending.");

        BatchedSchedulingStateValue.EndMutationBatch();

        Check(BatchedSchedulingStateValue.StrategicOrderIndices.size() == 1U, SuccessValue,
              "Ending a mutation batch should rebuild the strategic queue views once.");
        Check(BatchedSchedulingStateValue.PlanningProcessIndices.size() == 1U, SuccessValue,
              "Ending a mutation batch should rebuild the planning-process views once.");
        Check(!BatchedSchedulingStateValue.bDerivedQueuesDirty, SuccessValue,
              "Ending a mutation batch should clear the derived-queue dirty flag after rebuild.");

        size_t BatchedUnitOrderIndexValue = 0U;
        Check(BatchedSchedulingStateValue.TryGetOrderIndex(BatchedUnitOrderIdValue, BatchedUnitOrderIndexValue),
              SuccessValue, "Batched scheduling should index the unit-execution order after rebuild.");
        IntentSchedulingServiceValue.UpdateOrderLifecycleState(BatchedSchedulingStateValue, BatchedUnitOrderIdValue,
                                                               EOrderLifecycleState::Completed);

        Check(BatchedSchedulingStateValue.CompletedOrderIndices.size() == 1U, SuccessValue,
              "Completed unit-execution work should enter the completed-order view before compaction.");
        Check(BatchedSchedulingStateValue.CompactTerminalOrders(), SuccessValue,
              "Terminal unit-execution work should compact out of the hot scheduling store.");
        Check(BatchedSchedulingStateValue.GetOrderCount() == 1U, SuccessValue,
              "Compaction should remove only the terminal unit-execution order from the hot store.");
        Check(!BatchedSchedulingStateValue.TryGetOrderIndex(BatchedUnitOrderIdValue, BatchedUnitOrderIndexValue),
              SuccessValue, "Compaction should remove the compacted unit-execution order mapping.");
        Check(BatchedSchedulingStateValue.TryGetOrderIndex(BatchedStrategicOrderIdValue, BatchedUnitOrderIndexValue),
              SuccessValue, "Compaction should preserve non-unit-execution scheduler work.");
    }

    {
        FCommandAuthoritySchedulingState TerminalCompactionSchedulingStateValue;
        const uint32_t GoalStrategicOrderIdValue = IntentSchedulingServiceValue.SubmitOrder(
            TerminalCompactionSchedulingStateValue, CreateStrategicBuildBarracksOrder(2200U));
        TerminalCompactionSchedulingStateValue.SetOrderLifecycleState(GoalStrategicOrderIdValue,
                                                                     EOrderLifecycleState::Completed);

        Check(TerminalCompactionSchedulingStateValue.CompactTerminalOrders(), SuccessValue,
              "Completed non-opening strategic work should compact out of the hot scheduling store.");
        Check(TerminalCompactionSchedulingStateValue.GetOrderCount() == 0U, SuccessValue,
              "Compaction should remove completed goal-driven strategic work.");
    }

    {
        FCommandAuthoritySchedulingState OpeningCompactionSchedulingStateValue;
        FOpeningPlanExecutionState OpeningPlanExecutionStateValue;
        OpeningPlanExecutionStateValue.SetActivePlan(EOpeningPlanId::TerranTwoBaseMMMFrameOpening);

        FCommandOrderRecord OpeningStrategicOrderValue = CreateStrategicBuildBarracksOrder(2300U);
        OpeningStrategicOrderValue.PlanStepId = 9U;
        const uint32_t OpeningStrategicOrderIdValue =
            IntentSchedulingServiceValue.SubmitOrder(OpeningCompactionSchedulingStateValue, OpeningStrategicOrderValue);
        OpeningPlanExecutionStateValue.RecordSeededStep(9U, OpeningStrategicOrderIdValue);
        OpeningCompactionSchedulingStateValue.SetOrderLifecycleState(OpeningStrategicOrderIdValue,
                                                                    EOrderLifecycleState::Aborted);

        Check(!OpeningCompactionSchedulingStateValue.CompactTerminalOrders(&OpeningPlanExecutionStateValue),
              SuccessValue, "Aborted seeded opening work should remain available for reopening.");
        Check(OpeningCompactionSchedulingStateValue.GetOrderCount() == 1U, SuccessValue,
              "Compaction should preserve seeded opening work that is still authoritative.");

        OpeningPlanExecutionStateValue.UnseedStep(9U);
        Check(OpeningCompactionSchedulingStateValue.CompactTerminalOrders(&OpeningPlanExecutionStateValue),
              SuccessValue, "Unseeded opening work should compact once it no longer owns the step.");
        Check(OpeningCompactionSchedulingStateValue.GetOrderCount() == 0U, SuccessValue,
              "Compaction should remove unseeded aborted opening work.");
    }

    {
        FBlockedTaskRingBuffer BlockedTaskRingBufferValue;
        BlockedTaskRingBufferValue.Reset(2U);

        bool bCoalescedValue = false;
        bool bDroppedValue = false;
        bool bRejectedMustRunValue = false;

        const FBlockedTaskRecord FirstBlockedTaskRecordValue =
            CreateBlockedTaskRecord(1U, ECommandTaskRetentionPolicy::DiscardableDuplicate);
        Check(BlockedTaskRingBufferValue.TryPushOrCoalesce(FirstBlockedTaskRecordValue, bCoalescedValue,
                                                           bDroppedValue, bRejectedMustRunValue),
              SuccessValue, "Blocked task buffer should accept the first buffered retry record.");
        Check(BlockedTaskRingBufferValue.GetCount() == 1U, SuccessValue,
              "Blocked task buffer should count the first record.");
        Check(!bCoalescedValue && !bDroppedValue && !bRejectedMustRunValue, SuccessValue,
              "First blocked task insert should be a clean admission.");

        FBlockedTaskRecord DuplicateBlockedTaskRecordValue = FirstBlockedTaskRecordValue;
        DuplicateBlockedTaskRecordValue.RetryCount = 3U;
        Check(BlockedTaskRingBufferValue.TryPushOrCoalesce(DuplicateBlockedTaskRecordValue, bCoalescedValue,
                                                           bDroppedValue, bRejectedMustRunValue),
              SuccessValue, "Equivalent blocked task signatures should coalesce instead of growing the buffer.");
        Check(bCoalescedValue, SuccessValue,
              "Equivalent blocked task signatures should report coalescing.");
        Check(BlockedTaskRingBufferValue.GetCount() == 1U, SuccessValue,
              "Coalescing should keep the blocked-task count stable.");

        const FBlockedTaskRecord SecondBlockedTaskRecordValue =
            CreateBlockedTaskRecord(2U, ECommandTaskRetentionPolicy::BufferedRetry);
        Check(BlockedTaskRingBufferValue.TryPushOrCoalesce(SecondBlockedTaskRecordValue, bCoalescedValue,
                                                           bDroppedValue, bRejectedMustRunValue),
              SuccessValue, "Blocked task buffer should accept a second distinct record before capacity.");
        Check(BlockedTaskRingBufferValue.GetCount() == 2U, SuccessValue,
              "Blocked task buffer should reach its configured capacity.");

        const FBlockedTaskRecord ThirdBlockedTaskRecordValue =
            CreateBlockedTaskRecord(3U, ECommandTaskRetentionPolicy::BufferedRetry);
        Check(BlockedTaskRingBufferValue.TryPushOrCoalesce(ThirdBlockedTaskRecordValue, bCoalescedValue,
                                                           bDroppedValue, bRejectedMustRunValue),
              SuccessValue,
              "Blocked task buffer should evict the oldest discardable duplicate when a distinct retry arrives at capacity.");
        Check(!BlockedTaskRingBufferValue.HasEquivalentRecord(FirstBlockedTaskRecordValue), SuccessValue,
              "Blocked task buffer should evict the discardable duplicate candidate first.");
        Check(BlockedTaskRingBufferValue.HasEquivalentRecord(ThirdBlockedTaskRecordValue), SuccessValue,
              "Blocked task buffer should retain the newly inserted retry after eviction.");

        FBlockedTaskRingBuffer MustRunRejectedRingBufferValue;
        MustRunRejectedRingBufferValue.Reset(1U);
        const FBlockedTaskRecord PinnedBlockedTaskRecordValue =
            CreateBlockedTaskRecord(10U, ECommandTaskRetentionPolicy::BufferedRetry);
        Check(MustRunRejectedRingBufferValue.TryPushOrCoalesce(PinnedBlockedTaskRecordValue, bCoalescedValue,
                                                               bDroppedValue, bRejectedMustRunValue),
              SuccessValue, "Must-run rejection test should fill the buffer with a non-evictable record.");

        const FBlockedTaskRecord MustRunBlockedTaskRecordValue =
            CreateBlockedTaskRecord(11U, ECommandTaskRetentionPolicy::HotMustRun);
        Check(!MustRunRejectedRingBufferValue.TryPushOrCoalesce(MustRunBlockedTaskRecordValue, bCoalescedValue,
                                                                bDroppedValue, bRejectedMustRunValue),
              SuccessValue, "Must-run retries should report rejection when no blocked-buffer capacity remains.");
        Check(bRejectedMustRunValue, SuccessValue,
              "Must-run retries should surface explicit rejection telemetry when blocked storage is exhausted.");
    }

    {
        FTerranCommandTaskAdmissionService CommandTaskAdmissionServiceValue;
        FGameStateDescriptor AdmissionGameStateDescriptorValue;
        AdmissionGameStateDescriptorValue.CurrentStep = 100U;
        AdmissionGameStateDescriptorValue.CurrentGameLoop = 100U;
        AdmissionGameStateDescriptorValue.MacroState.WorkerCount = 12U;
        AdmissionGameStateDescriptorValue.MacroState.ActiveBaseCount = 1U;
        AdmissionGameStateDescriptorValue.MacroState.BarracksCount = 1U;
        AdmissionGameStateDescriptorValue.BuildPlanning.ObservedTownHallCount = 1U;
        AdmissionGameStateDescriptorValue.BuildPlanning.AvailableMinerals = 500U;
        AdmissionGameStateDescriptorValue.BuildPlanning.AvailableVespene = 0U;
        AdmissionGameStateDescriptorValue.BuildPlanning.AvailableSupply = 4U;
        PopulateForecastStimulusState(AdmissionGameStateDescriptorValue, 500U, 0U, 4U, 12U, 1U, 1U, 0U, 0U);
        CommandTaskAdmissionServiceValue.RefreshStimulusState(AdmissionGameStateDescriptorValue);

        uint32_t AdmittedStrategicOrderCountValue = 0U;
        for (uint32_t GoalIndexValue = 0U;
             GoalIndexValue <
             (AdmissionGameStateDescriptorValue.CommandAuthoritySchedulingState.MaxActiveStrategicOrders + 6U);
             ++GoalIndexValue)
        {
            uint32_t AdmittedOrderIdValue = 0U;
            if (CommandTaskAdmissionServiceValue.TryAdmitGoalDrivenOrder(
                    AdmissionGameStateDescriptorValue,
                    CreateStrategicBuildBarracksOrder(100U + GoalIndexValue),
                    AdmittedOrderIdValue))
            {
                ++AdmittedStrategicOrderCountValue;
            }
        }

        Check(AdmittedStrategicOrderCountValue ==
                  AdmissionGameStateDescriptorValue.CommandAuthoritySchedulingState.MaxActiveStrategicOrders,
              SuccessValue, "Strategic hot admission should stop at the configured active-order cap.");
        Check(AdmissionGameStateDescriptorValue.CommandAuthoritySchedulingState.GetActiveOrderCountForLayer(
                  ECommandAuthorityLayer::StrategicDirector) ==
                  AdmissionGameStateDescriptorValue.CommandAuthoritySchedulingState.MaxActiveStrategicOrders,
              SuccessValue, "Strategic active-order counting should match the bounded admission cap.");

        uint32_t DuplicateAdmissionOrderIdValue = 0U;
        Check(!CommandTaskAdmissionServiceValue.TryAdmitGoalDrivenOrder(
                  AdmissionGameStateDescriptorValue, CreateStrategicBuildBarracksOrder(100U),
                  DuplicateAdmissionOrderIdValue),
              SuccessValue, "Equivalent hot strategic work should not admit duplicate active orders.");

        uint32_t MustRunOrderIdValue = 0U;
        Check(CommandTaskAdmissionServiceValue.TryAdmitGoalDrivenOrder(
                  AdmissionGameStateDescriptorValue,
                  CreateStrategicBuildBarracksOrder(10000U, ECommandTaskRetentionPolicy::HotMustRun),
                  MustRunOrderIdValue),
              SuccessValue, "Hot must-run strategic work should admit even when the normal strategic cap is full.");

        FGameStateDescriptor ZeroDiscretionaryBudgetGameStateDescriptorValue;
        ZeroDiscretionaryBudgetGameStateDescriptorValue.CurrentStep = 110U;
        ZeroDiscretionaryBudgetGameStateDescriptorValue.CurrentGameLoop = 110U;
        ZeroDiscretionaryBudgetGameStateDescriptorValue.MacroState.WorkerCount = 12U;
        ZeroDiscretionaryBudgetGameStateDescriptorValue.MacroState.ActiveBaseCount = 1U;
        ZeroDiscretionaryBudgetGameStateDescriptorValue.BuildPlanning.ObservedTownHallCount = 1U;
        ZeroDiscretionaryBudgetGameStateDescriptorValue.BuildPlanning.AvailableMinerals = 150U;
        ZeroDiscretionaryBudgetGameStateDescriptorValue.BuildPlanning.AvailableVespene = 0U;
        ZeroDiscretionaryBudgetGameStateDescriptorValue.BuildPlanning.AvailableSupply = 4U;
        PopulateForecastStimulusState(ZeroDiscretionaryBudgetGameStateDescriptorValue, 0U, 0U, 4U, 12U, 1U, 1U, 0U,
                                      0U);
        CommandTaskAdmissionServiceValue.RefreshStimulusState(ZeroDiscretionaryBudgetGameStateDescriptorValue);

        uint32_t RejectedFlexibleOrderIdValue = 0U;
        Check(!CommandTaskAdmissionServiceValue.TryAdmitGoalDrivenOrder(
                  ZeroDiscretionaryBudgetGameStateDescriptorValue, CreateStrategicBuildBarracksOrder(5000U),
                  RejectedFlexibleOrderIdValue),
              SuccessValue,
              "Flexible macro production should not admit when projected discretionary budget is exhausted.");

        uint32_t MandatoryOpeningOrderIdValue = 0U;
        Check(CommandTaskAdmissionServiceValue.TryAdmitOpeningTask(
                  ZeroDiscretionaryBudgetGameStateDescriptorValue, CreateMandatoryOpeningBarracksTaskDescriptor(),
                  MandatoryOpeningOrderIdValue),
              SuccessValue,
              "Mandatory exact-slot opening work should still admit when discretionary budget is exhausted.");

        FGameStateDescriptor ParkingGameStateDescriptorValue;
        ParkingGameStateDescriptorValue.CurrentStep = 200U;
        ParkingGameStateDescriptorValue.CurrentGameLoop = 200U;
        ParkingGameStateDescriptorValue.MacroState.WorkerCount = 12U;
        ParkingGameStateDescriptorValue.MacroState.ActiveBaseCount = 1U;
        ParkingGameStateDescriptorValue.MacroState.BarracksCount = 1U;
        ParkingGameStateDescriptorValue.BuildPlanning.ObservedTownHallCount = 1U;
        ParkingGameStateDescriptorValue.BuildPlanning.AvailableMinerals = 500U;
        ParkingGameStateDescriptorValue.BuildPlanning.AvailableVespene = 0U;
        ParkingGameStateDescriptorValue.BuildPlanning.AvailableSupply = 4U;
        PopulateForecastStimulusState(ParkingGameStateDescriptorValue, 500U, 0U, 4U, 12U, 1U, 1U, 0U, 0U);
        CommandTaskAdmissionServiceValue.RefreshStimulusState(ParkingGameStateDescriptorValue);

        uint32_t ParkedOrderIdValue = 0U;
        Check(CommandTaskAdmissionServiceValue.TryAdmitGoalDrivenOrder(
                  ParkingGameStateDescriptorValue, CreateStrategicBuildBarracksOrder(2000U), ParkedOrderIdValue),
              SuccessValue, "Deferred strategic work should admit before it can be parked.");
        ParkingGameStateDescriptorValue.CommandAuthoritySchedulingState.SetOrderDeferralState(
            ParkedOrderIdValue, ECommandOrderDeferralReason::NoProducer,
            ParkingGameStateDescriptorValue.CurrentStep, ParkingGameStateDescriptorValue.CurrentGameLoop);
        CommandTaskAdmissionServiceValue.ParkDeferredOrders(ParkingGameStateDescriptorValue);

        Check(ParkingGameStateDescriptorValue.CommandAuthoritySchedulingState.BlockedStrategicTasks.GetCount() == 1U,
              SuccessValue, "NoProducer deferrals should park strategic work in the blocked strategic buffer.");
        size_t ParkedOrderIndexValue = 0U;
        Check(ParkingGameStateDescriptorValue.CommandAuthoritySchedulingState.TryGetOrderIndex(
                  ParkedOrderIdValue, ParkedOrderIndexValue),
              SuccessValue, "Parked strategic orders should remain addressable in the authoritative order store.");
        if (ParkingGameStateDescriptorValue.CommandAuthoritySchedulingState.TryGetOrderIndex(
                ParkedOrderIdValue, ParkedOrderIndexValue))
        {
            const FCommandOrderRecord ParkedOrderRecordValue =
                ParkingGameStateDescriptorValue.CommandAuthoritySchedulingState.GetOrderRecord(ParkedOrderIndexValue);
            Check(ParkedOrderRecordValue.LifecycleState == EOrderLifecycleState::Expired, SuccessValue,
                  "Parked strategic work should retire from the hot order store immediately.");
        }

        ParkingGameStateDescriptorValue.CurrentGameLoop = 201U;
        CommandTaskAdmissionServiceValue.RefreshStimulusState(ParkingGameStateDescriptorValue);
        CommandTaskAdmissionServiceValue.ReactivateBlockedTasks(ParkingGameStateDescriptorValue);
        Check(ParkingGameStateDescriptorValue.CommandAuthoritySchedulingState.BlockedStrategicTasks.GetCount() == 1U,
              SuccessValue,
              "Blocked producer-availability retries should remain parked when neither cooldown nor producer stimulus changes.");

        ParkingGameStateDescriptorValue.MacroState.WorkerCount = 13U;
        PopulateForecastStimulusState(ParkingGameStateDescriptorValue, 500U, 0U, 4U, 13U, 1U, 1U, 0U, 0U);
        CommandTaskAdmissionServiceValue.RefreshStimulusState(ParkingGameStateDescriptorValue);
        CommandTaskAdmissionServiceValue.ReactivateBlockedTasks(ParkingGameStateDescriptorValue);
        Check(ParkingGameStateDescriptorValue.CommandAuthoritySchedulingState.BlockedStrategicTasks.GetCount() == 0U,
              SuccessValue, "Producer stimulus changes should reactivate blocked NoProducer retries.");
        Check(ParkingGameStateDescriptorValue.CommandAuthoritySchedulingState.TotalReactivatedBlockedTaskCount == 1U,
              SuccessValue, "Blocked retry reactivation should increment the explicit reactivation counter.");
        Check(ParkingGameStateDescriptorValue.CommandAuthoritySchedulingState.RecentReactivatedBlockedTaskCount == 1U,
              SuccessValue, "Blocked retry reactivation should increment the recent reactivation counter.");
        ParkingGameStateDescriptorValue.CommandAuthoritySchedulingState.AdvanceRecentBlockedTaskCounterWindow(400U);
        Check(ParkingGameStateDescriptorValue.CommandAuthoritySchedulingState.RecentReactivatedBlockedTaskCount == 0U,
              SuccessValue, "Blocked retry recent counters should reset once the reporting window advances.");

        FGameStateDescriptor ProducerBusyGameStateDescriptorValue;
        ProducerBusyGameStateDescriptorValue.CurrentStep = 300U;
        ProducerBusyGameStateDescriptorValue.CurrentGameLoop = 300U;
        ProducerBusyGameStateDescriptorValue.MacroState.WorkerCount = 12U;
        ProducerBusyGameStateDescriptorValue.MacroState.ActiveBaseCount = 1U;
        ProducerBusyGameStateDescriptorValue.MacroState.BarracksCount = 1U;
        ProducerBusyGameStateDescriptorValue.BuildPlanning.ObservedTownHallCount = 1U;
        PopulateForecastStimulusState(ProducerBusyGameStateDescriptorValue, 500U, 0U, 4U, 12U, 1U, 1U, 0U, 0U);
        CommandTaskAdmissionServiceValue.RefreshStimulusState(ProducerBusyGameStateDescriptorValue);

        uint32_t ProducerBusyOrderIdValue = 0U;
        Check(CommandTaskAdmissionServiceValue.TryAdmitGoalDrivenOrder(
                  ProducerBusyGameStateDescriptorValue, CreateStrategicBuildBarracksOrder(3000U),
                  ProducerBusyOrderIdValue),
              SuccessValue, "ProducerBusy parking test should admit the strategic work item.");

        for (uint64_t RetryStepValue = 300U; RetryStepValue < 302U; ++RetryStepValue)
        {
            ProducerBusyGameStateDescriptorValue.CommandAuthoritySchedulingState.SetOrderDeferralState(
                ProducerBusyOrderIdValue, ECommandOrderDeferralReason::ProducerBusy, RetryStepValue, RetryStepValue);
            CommandTaskAdmissionServiceValue.ParkDeferredOrders(ProducerBusyGameStateDescriptorValue);
            Check(ProducerBusyGameStateDescriptorValue.CommandAuthoritySchedulingState.BlockedStrategicTasks.GetCount() ==
                      0U,
                  SuccessValue,
                  "ProducerBusy deferrals should stay hot until the short consecutive busy threshold is reached.");
        }

        ProducerBusyGameStateDescriptorValue.CommandAuthoritySchedulingState.SetOrderDeferralState(
            ProducerBusyOrderIdValue, ECommandOrderDeferralReason::ProducerBusy, 302U, 302U);
        CommandTaskAdmissionServiceValue.ParkDeferredOrders(ProducerBusyGameStateDescriptorValue);
        Check(ProducerBusyGameStateDescriptorValue.CommandAuthoritySchedulingState.BlockedStrategicTasks.GetCount() ==
                  1U,
              SuccessValue, "ProducerBusy deferrals should park after three consecutive busy results.");

        FGameStateDescriptor ResourceWakeGameStateDescriptorValue;
        ResourceWakeGameStateDescriptorValue.CurrentStep = 350U;
        ResourceWakeGameStateDescriptorValue.CurrentGameLoop = 350U;
        ResourceWakeGameStateDescriptorValue.MacroState.WorkerCount = 12U;
        ResourceWakeGameStateDescriptorValue.MacroState.ActiveBaseCount = 1U;
        ResourceWakeGameStateDescriptorValue.MacroState.BarracksCount = 1U;
        ResourceWakeGameStateDescriptorValue.BuildPlanning.ObservedTownHallCount = 1U;
        PopulateForecastStimulusState(ResourceWakeGameStateDescriptorValue, 500U, 0U, 4U, 12U, 1U, 1U, 0U, 0U);
        CommandTaskAdmissionServiceValue.RefreshStimulusState(ResourceWakeGameStateDescriptorValue);

        uint32_t ResourceWakeOrderIdValue = 0U;
        Check(CommandTaskAdmissionServiceValue.TryAdmitGoalDrivenOrder(
                  ResourceWakeGameStateDescriptorValue, CreateStrategicBuildBarracksOrder(3500U),
                  ResourceWakeOrderIdValue),
              SuccessValue, "Resource wake test should admit the strategic work item before it is parked.");
        ResourceWakeGameStateDescriptorValue.CommandAuthoritySchedulingState.SetOrderDeferralState(
            ResourceWakeOrderIdValue, ECommandOrderDeferralReason::InsufficientResources,
            ResourceWakeGameStateDescriptorValue.CurrentStep, ResourceWakeGameStateDescriptorValue.CurrentGameLoop);
        CommandTaskAdmissionServiceValue.ParkDeferredOrders(ResourceWakeGameStateDescriptorValue);
        Check(ResourceWakeGameStateDescriptorValue.CommandAuthoritySchedulingState.BlockedStrategicTasks.GetCount() == 1U,
              SuccessValue, "Insufficient-resources deferrals should park strategic work in the blocked buffer.");

        ResourceWakeGameStateDescriptorValue.CurrentGameLoop = 351U;
        PopulateForecastStimulusState(ResourceWakeGameStateDescriptorValue, 250U, 0U, 4U, 12U, 1U, 1U, 0U, 0U);
        CommandTaskAdmissionServiceValue.RefreshStimulusState(ResourceWakeGameStateDescriptorValue);
        CommandTaskAdmissionServiceValue.ReactivateBlockedTasks(ResourceWakeGameStateDescriptorValue);
        Check(ResourceWakeGameStateDescriptorValue.CommandAuthoritySchedulingState.BlockedStrategicTasks.GetCount() == 0U,
              SuccessValue, "Forecasted mineral availability changes should reactivate blocked resource retries.");
        Check(ResourceWakeGameStateDescriptorValue.CommandAuthoritySchedulingState.TotalReactivatedBlockedTaskCount == 1U,
              SuccessValue, "Resource-driven blocked retry reactivation should increment the explicit reactivation counter.");
    }

    {
        FTerranGameStateDescriptorBuilder GameStateDescriptorBuilderValue;
        FTerranForecastStateBuilder ForecastStateBuilderValue;
        FEconomyDomainState EconomyDomainStateValue;

        FAgentState SeedAgentStateValue;
        SeedAgentStateValue.Economy.Minerals = 100U;
        SeedAgentStateValue.Economy.Vespene = 0U;
        SeedAgentStateValue.Economy.Supply = 14U;
        SeedAgentStateValue.Economy.SupplyCap = 23U;
        SeedAgentStateValue.Economy.SupplyAvailable = 9U;
        SeedAgentStateValue.Units.SetUnitCount(UNIT_TYPEID::TERRAN_SCV, 14U);
        SeedAgentStateValue.Units.Update();
        SeedAgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_COMMANDCENTER, 1U);
        SeedAgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 1U);

        FGameStateDescriptor SeedDescriptorValue;
        GameStateDescriptorBuilderValue.RebuildGameStateDescriptor(10U, 100U, SeedAgentStateValue, SeedDescriptorValue);
        ForecastStateBuilderValue.RebuildForecastState(SeedAgentStateValue, EconomyDomainStateValue, SeedDescriptorValue);

        FAgentState ForecastAgentStateValue;
        ForecastAgentStateValue.Economy.Minerals = 90U;
        ForecastAgentStateValue.Economy.Vespene = 0U;
        ForecastAgentStateValue.Economy.Supply = 15U;
        ForecastAgentStateValue.Economy.SupplyCap = 23U;
        ForecastAgentStateValue.Economy.SupplyAvailable = 8U;
        ForecastAgentStateValue.Units.SetUnitCount(UNIT_TYPEID::TERRAN_SCV, 14U);
        ForecastAgentStateValue.Units.Update();
        ForecastAgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_COMMANDCENTER, 1U);
        ForecastAgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 1U);
        ForecastAgentStateValue.Buildings.SetCurrentlyInConstruction(UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 1U);

        FGameStateDescriptor ForecastGameStateDescriptorValue;
        GameStateDescriptorBuilderValue.RebuildGameStateDescriptor(11U, 196U, ForecastAgentStateValue,
                                                                  ForecastGameStateDescriptorValue);

        FCommandOrderRecord MaterializedSupplyDepotOrderValue = FCommandOrderRecord::CreateNoTarget(
            ECommandAuthorityLayer::UnitExecution, NullTag, ABILITY_ID::BUILD_SUPPLYDEPOT, 160,
            EIntentDomain::StructureBuild, 196U);
        MaterializedSupplyDepotOrderValue.TaskType = ECommandTaskType::Supply;
        MaterializedSupplyDepotOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
        MaterializedSupplyDepotOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_SUPPLYDEPOT;
        MaterializedSupplyDepotOrderValue.TargetCount = 2U;
        const uint32_t MaterializedSupplyDepotOrderIdValue =
            ForecastGameStateDescriptorValue.CommandAuthoritySchedulingState.EnqueueOrder(
                MaterializedSupplyDepotOrderValue);
        ForecastGameStateDescriptorValue.CommandAuthoritySchedulingState.SetOrderDispatchState(
            MaterializedSupplyDepotOrderIdValue, 11U, 196U, 1U, 0U);
        ForecastGameStateDescriptorValue.CommandAuthoritySchedulingState.SetOrderLifecycleState(
            MaterializedSupplyDepotOrderIdValue, EOrderLifecycleState::Dispatched);

        FCommandOrderRecord MarineOrderValue = FCommandOrderRecord::CreateNoTarget(
            ECommandAuthorityLayer::EconomyAndProduction, NullTag, ABILITY_ID::TRAIN_MARINE, 150,
            EIntentDomain::UnitProduction, 196U);
        MarineOrderValue.TaskType = ECommandTaskType::UnitProduction;
        MarineOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKS;
        MarineOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_MARINE;
        MarineOrderValue.TargetCount = 1U;
        const uint32_t MarineOrderIdValue =
            ForecastGameStateDescriptorValue.CommandAuthoritySchedulingState.EnqueueOrder(MarineOrderValue);
        ForecastGameStateDescriptorValue.CommandAuthoritySchedulingState.SetOrderLifecycleState(
            MarineOrderIdValue, EOrderLifecycleState::Ready);

        FCommandOrderRecord ReadyScoutOrderValue = FCommandOrderRecord::CreatePointTarget(
            ECommandAuthorityLayer::UnitExecution, 900U, ABILITY_ID::MOVE_MOVE, Point2D(12.0f, 18.0f), 90,
            EIntentDomain::ArmyCombat, 196U);
        const uint32_t ReadyScoutOrderIdValue =
            ForecastGameStateDescriptorValue.CommandAuthoritySchedulingState.EnqueueOrder(ReadyScoutOrderValue);
        ForecastGameStateDescriptorValue.CommandAuthoritySchedulingState.SetOrderLifecycleState(
            ReadyScoutOrderIdValue, EOrderLifecycleState::Ready);

        ForecastStateBuilderValue.RebuildForecastState(ForecastAgentStateValue, EconomyDomainStateValue,
                                                       ForecastGameStateDescriptorValue);

        Check(ForecastGameStateDescriptorValue.SchedulerOutlook.InFlightOrderCount == 1U, SuccessValue,
              "Forecast projection should count only dispatched unit-execution rows as in-flight scheduler work.");
        Check(ForecastGameStateDescriptorValue.SchedulerOutlook.ActiveLeafOrderCount == 1U, SuccessValue,
              "Forecast projection should skip leaf orders that have already materialized into observation.");
        Check(ForecastGameStateDescriptorValue.SchedulerOutlook.GetScheduledBuildingCount(
                  UNIT_TYPEID::TERRAN_SUPPLYDEPOT) == 0U,
              SuccessValue, "Materialized supply depots should not remain in scheduled building projections.");
        Check(ForecastGameStateDescriptorValue.SchedulerOutlook.GetScheduledUnitCount(UNIT_TYPEID::TERRAN_MARINE) == 1U,
              SuccessValue, "Unmaterialized marine production should remain visible in scheduled unit projections.");
        Check(ForecastGameStateDescriptorValue.ProductionState.GetProjectedBuildingCount(
                  UNIT_TYPEID::TERRAN_SUPPLYDEPOT) == 2U,
              SuccessValue, "Projected supply depot count should combine observed and in-progress state without double counting scheduled rows.");
        Check(ForecastGameStateDescriptorValue.ProductionState.GetProjectedUnitCount(UNIT_TYPEID::TERRAN_MARINE) == 1U,
              SuccessValue, "Projected marine count should include unmaterialized scheduled unit production.");
    }

    FGameStateDescriptor GameStateDescriptorValue;
    GameStateDescriptorValue.CommandAuthoritySchedulingState = CommandAuthoritySchedulingStateValue;
    GameStateDescriptorValue.Reset();
    Check(GameStateDescriptorValue.CommandAuthoritySchedulingState.GetOrderCount() == 0U, SuccessValue,
          "Game state reset should clear scheduling records.");
    Check(GameStateDescriptorValue.CommandAuthoritySchedulingState.NextOrderId == 1U, SuccessValue,
          "Game state reset should restore the scheduling order identifier seed.");

    Check(std::string(ToString(ECommandAuthorityLayer::Squad)) == "Squad", SuccessValue,
          "Authority layer string conversion should expose Squad.");
    Check(std::string(ToString(EOrderLifecycleState::Ready)) == "Ready", SuccessValue,
          "Lifecycle state string conversion should expose Ready.");
    Check(std::string(ToString(EPlanningProcessorState::ReadyToDrain)) == "ReadyToDrain", SuccessValue,
          "Processor state string conversion should expose ReadyToDrain.");
    Check(std::string(ToString(EIntentPlaybackState::Dispatching)) == "Dispatching", SuccessValue,
          "Playback state string conversion should expose Dispatching.");
    Check(std::string(ToString(ECommandOrderDeferralReason::ProducerBusy)) == "ProducerBusy", SuccessValue,
          "Deferral reason string conversion should expose ProducerBusy.");

    return SuccessValue;
}

}  // namespace sc2
