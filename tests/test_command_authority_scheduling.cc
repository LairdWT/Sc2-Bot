#include "test_command_authority_scheduling.h"

#include <cmath>
#include <iostream>
#include <string>

#include "common/agent_framework.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/planning/ECommandAuthorityLayer.h"
#include "common/planning/ECommandOrderDeferralReason.h"
#include "common/planning/EIntentPlaybackState.h"
#include "common/planning/EOrderLifecycleState.h"
#include "common/planning/EPlanningProcessorState.h"
#include "common/planning/FCommandAuthoritySchedulingState.h"
#include "common/planning/FCommandOrderRecord.h"
#include "common/planning/FIntentSchedulingService.h"

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
