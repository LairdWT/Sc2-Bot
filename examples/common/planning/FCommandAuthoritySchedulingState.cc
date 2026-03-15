#include "common/planning/FCommandAuthoritySchedulingState.h"

#include <algorithm>

namespace sc2
{
namespace
{

template <typename TValueType>
std::vector<TValueType> BuildCompactedVector(const std::vector<TValueType>& SourceValues,
                                             const std::vector<size_t>& RetainedOrderIndicesValue)
{
    std::vector<TValueType> CompactedValues;
    CompactedValues.reserve(RetainedOrderIndicesValue.size());
    for (const size_t RetainedOrderIndexValue : RetainedOrderIndicesValue)
    {
        CompactedValues.push_back(SourceValues[RetainedOrderIndexValue]);
    }

    return CompactedValues;
}

bool ShouldCompactTerminalOrder(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                const size_t OrderIndexValue)
{
    return CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] ==
               ECommandAuthorityLayer::UnitExecution &&
           IsTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]);
}

}  // namespace


FCommandAuthoritySchedulingState::FCommandAuthoritySchedulingState()
{
    Reset();
}

void FCommandAuthoritySchedulingState::Reset()
{
    NextOrderId = 1U;
    ProcessorState = EPlanningProcessorState::Idle;
    PlaybackState = EIntentPlaybackState::Idle;
    MaxStrategicOrdersPerStep = 4U;
    MaxArmyOrdersPerStep = 8U;
    MaxSquadOrdersPerStep = 16U;
    MaxUnitIntentsPerStep = 32U;
    MaxActiveStrategicOrders = 24U;
    MaxActivePlanningOrders = 32U;
    MaxActiveUnitExecutionOrders = 128U;
    MaxBlockedStrategicTasks = 128U;
    MaxBlockedPlanningTasks = 192U;
    MutationBatchDepth = 0U;
    RejectedUnitExecutionAdmissionCount = 0U;
    SupersededUnitExecutionOrderCount = 0U;
    BufferedBlockedTaskCount = 0U;
    CoalescedBlockedTaskCount = 0U;
    DroppedBlockedTaskCount = 0U;
    ReactivatedBlockedTaskCount = 0U;
    RejectedMustRunBlockedTaskCount = 0U;
    bDerivedQueuesDirty = false;
    bPrioritiesDirty = false;
    SchedulerStimulusState.Reset();

    OrderIds.clear();
    ParentOrderIds.clear();
    SourceGoalIds.clear();
    SourceLayers.clear();
    LifecycleStates.clear();
    TaskPackageKinds.clear();
    TaskNeedKinds.clear();
    TaskTypes.clear();
    RetentionPolicies.clear();
    BlockedTaskWakeKinds.clear();
    BasePriorityValues.clear();
    EffectivePriorityValues.clear();
    PriorityTiers.clear();
    IntentDomains.clear();
    CreationSteps.clear();
    DeadlineSteps.clear();
    OwningArmyIndices.clear();
    OwningSquadIndices.clear();
    ActorTags.clear();
    AbilityIds.clear();
    TargetKinds.clear();
    TargetPoints.clear();
    TargetUnitTags.clear();
    QueuedValues.clear();
    RequiresPlacementValidationValues.clear();
    RequiresPathingValidationValues.clear();
    PlanStepIds.clear();
    TargetCounts.clear();
    RequestedQueueCounts.clear();
    ProducerUnitTypeIds.clear();
    ResultUnitTypeIds.clear();
    UpgradeIds.clear();
    PreferredPlacementSlotTypes.clear();
    PreferredPlacementSlotIdTypes.clear();
    PreferredPlacementSlotIdOrdinals.clear();
    ReservedPlacementSlotTypes.clear();
    ReservedPlacementSlotOrdinals.clear();
    LastDeferralReasons.clear();
    LastDeferralSteps.clear();
    LastDeferralGameLoops.clear();
    ConsecutiveDeferralCounts.clear();
    DispatchSteps.clear();
    DispatchGameLoops.clear();
    ObservedCountsAtDispatch.clear();
    ObservedInConstructionCountsAtDispatch.clear();
    DispatchAttemptCounts.clear();

    OrderIdToIndex.clear();
    StrategicOrderIndices.clear();
    PlanningProcessIndices.clear();
    ArmyOrderIndices.clear();
    SquadOrderIndices.clear();
    ReadyIntentIndices.clear();
    DispatchedOrderIndices.clear();
    CompletedOrderIndices.clear();
    for (std::vector<size_t>& StrategicQueueValue : StrategicQueues)
    {
        StrategicQueueValue.clear();
    }
    for (std::vector<size_t>& PlanningQueueValue : PlanningQueues)
    {
        PlanningQueueValue.clear();
    }
    for (std::vector<size_t>& ArmyQueueValue : ArmyQueues)
    {
        ArmyQueueValue.clear();
    }
    for (std::vector<size_t>& SquadQueueValue : SquadQueues)
    {
        SquadQueueValue.clear();
    }
    for (std::array<std::vector<size_t>, IntentDomainCountValue>& TierQueueGroupValue : ReadyIntentQueues)
    {
        for (std::vector<size_t>& ReadyQueueValue : TierQueueGroupValue)
        {
            ReadyQueueValue.clear();
        }
    }

    BlockedStrategicTasks.Reset(MaxBlockedStrategicTasks);
    BlockedPlanningTasks.Reset(MaxBlockedPlanningTasks);
}

void FCommandAuthoritySchedulingState::BeginMutationBatch()
{
    ++MutationBatchDepth;
}

void FCommandAuthoritySchedulingState::EndMutationBatch()
{
    if (MutationBatchDepth == 0U)
    {
        return;
    }

    --MutationBatchDepth;
    if (MutationBatchDepth == 0U && bDerivedQueuesDirty)
    {
        RebuildDerivedQueues();
    }
}

void FCommandAuthoritySchedulingState::Reserve(const size_t OrderCapacityValue)
{
    OrderIds.reserve(OrderCapacityValue);
    ParentOrderIds.reserve(OrderCapacityValue);
    SourceGoalIds.reserve(OrderCapacityValue);
    SourceLayers.reserve(OrderCapacityValue);
    LifecycleStates.reserve(OrderCapacityValue);
    TaskPackageKinds.reserve(OrderCapacityValue);
    TaskNeedKinds.reserve(OrderCapacityValue);
    TaskTypes.reserve(OrderCapacityValue);
    RetentionPolicies.reserve(OrderCapacityValue);
    BlockedTaskWakeKinds.reserve(OrderCapacityValue);
    BasePriorityValues.reserve(OrderCapacityValue);
    EffectivePriorityValues.reserve(OrderCapacityValue);
    PriorityTiers.reserve(OrderCapacityValue);
    IntentDomains.reserve(OrderCapacityValue);
    CreationSteps.reserve(OrderCapacityValue);
    DeadlineSteps.reserve(OrderCapacityValue);
    OwningArmyIndices.reserve(OrderCapacityValue);
    OwningSquadIndices.reserve(OrderCapacityValue);
    ActorTags.reserve(OrderCapacityValue);
    AbilityIds.reserve(OrderCapacityValue);
    TargetKinds.reserve(OrderCapacityValue);
    TargetPoints.reserve(OrderCapacityValue);
    TargetUnitTags.reserve(OrderCapacityValue);
    QueuedValues.reserve(OrderCapacityValue);
    RequiresPlacementValidationValues.reserve(OrderCapacityValue);
    RequiresPathingValidationValues.reserve(OrderCapacityValue);
    PlanStepIds.reserve(OrderCapacityValue);
    TargetCounts.reserve(OrderCapacityValue);
    RequestedQueueCounts.reserve(OrderCapacityValue);
    ProducerUnitTypeIds.reserve(OrderCapacityValue);
    ResultUnitTypeIds.reserve(OrderCapacityValue);
    UpgradeIds.reserve(OrderCapacityValue);
    PreferredPlacementSlotTypes.reserve(OrderCapacityValue);
    PreferredPlacementSlotIdTypes.reserve(OrderCapacityValue);
    PreferredPlacementSlotIdOrdinals.reserve(OrderCapacityValue);
    ReservedPlacementSlotTypes.reserve(OrderCapacityValue);
    ReservedPlacementSlotOrdinals.reserve(OrderCapacityValue);
    LastDeferralReasons.reserve(OrderCapacityValue);
    LastDeferralSteps.reserve(OrderCapacityValue);
    LastDeferralGameLoops.reserve(OrderCapacityValue);
    ConsecutiveDeferralCounts.reserve(OrderCapacityValue);
    DispatchSteps.reserve(OrderCapacityValue);
    DispatchGameLoops.reserve(OrderCapacityValue);
    ObservedCountsAtDispatch.reserve(OrderCapacityValue);
    ObservedInConstructionCountsAtDispatch.reserve(OrderCapacityValue);
    DispatchAttemptCounts.reserve(OrderCapacityValue);
}

uint32_t FCommandAuthoritySchedulingState::EnqueueOrder(const FCommandOrderRecord& CommandOrderRecordValue)
{
    FCommandOrderRecord StoredOrderValue = CommandOrderRecordValue;
    if (StoredOrderValue.OrderId == 0U)
    {
        StoredOrderValue.OrderId = NextOrderId;
        ++NextOrderId;
    }
    else
    {
        NextOrderId = std::max(NextOrderId, StoredOrderValue.OrderId + 1U);
    }

    const size_t OrderIndexValue = OrderIds.size();
    OrderIds.push_back(StoredOrderValue.OrderId);
    ParentOrderIds.push_back(StoredOrderValue.ParentOrderId);
    SourceGoalIds.push_back(StoredOrderValue.SourceGoalId);
    SourceLayers.push_back(StoredOrderValue.SourceLayer);
    LifecycleStates.push_back(StoredOrderValue.LifecycleState);
    TaskPackageKinds.push_back(StoredOrderValue.TaskPackageKind);
    TaskNeedKinds.push_back(StoredOrderValue.TaskNeedKind);
    TaskTypes.push_back(StoredOrderValue.TaskType);
    RetentionPolicies.push_back(StoredOrderValue.RetentionPolicy);
    BlockedTaskWakeKinds.push_back(StoredOrderValue.BlockedTaskWakeKind);
    BasePriorityValues.push_back(StoredOrderValue.BasePriorityValue);
    EffectivePriorityValues.push_back(StoredOrderValue.EffectivePriorityValue);
    PriorityTiers.push_back(StoredOrderValue.PriorityTier);
    IntentDomains.push_back(StoredOrderValue.IntentDomain);
    CreationSteps.push_back(StoredOrderValue.CreationStep);
    DeadlineSteps.push_back(StoredOrderValue.DeadlineStep);
    OwningArmyIndices.push_back(StoredOrderValue.OwningArmyIndex);
    OwningSquadIndices.push_back(StoredOrderValue.OwningSquadIndex);
    ActorTags.push_back(StoredOrderValue.ActorTag);
    AbilityIds.push_back(StoredOrderValue.AbilityId);
    TargetKinds.push_back(StoredOrderValue.TargetKind);
    TargetPoints.push_back(StoredOrderValue.TargetPoint);
    TargetUnitTags.push_back(StoredOrderValue.TargetUnitTag);
    QueuedValues.push_back(StoredOrderValue.Queued);
    RequiresPlacementValidationValues.push_back(StoredOrderValue.RequiresPlacementValidation);
    RequiresPathingValidationValues.push_back(StoredOrderValue.RequiresPathingValidation);
    PlanStepIds.push_back(StoredOrderValue.PlanStepId);
    TargetCounts.push_back(StoredOrderValue.TargetCount);
    RequestedQueueCounts.push_back(StoredOrderValue.RequestedQueueCount);
    ProducerUnitTypeIds.push_back(StoredOrderValue.ProducerUnitTypeId);
    ResultUnitTypeIds.push_back(StoredOrderValue.ResultUnitTypeId);
    UpgradeIds.push_back(StoredOrderValue.UpgradeId);
    PreferredPlacementSlotTypes.push_back(StoredOrderValue.PreferredPlacementSlotType);
    PreferredPlacementSlotIdTypes.push_back(StoredOrderValue.PreferredPlacementSlotId.SlotType);
    PreferredPlacementSlotIdOrdinals.push_back(StoredOrderValue.PreferredPlacementSlotId.Ordinal);
    ReservedPlacementSlotTypes.push_back(StoredOrderValue.ReservedPlacementSlotId.SlotType);
    ReservedPlacementSlotOrdinals.push_back(StoredOrderValue.ReservedPlacementSlotId.Ordinal);
    LastDeferralReasons.push_back(StoredOrderValue.LastDeferralReason);
    LastDeferralSteps.push_back(StoredOrderValue.LastDeferralStep);
    LastDeferralGameLoops.push_back(StoredOrderValue.LastDeferralGameLoop);
    ConsecutiveDeferralCounts.push_back(StoredOrderValue.ConsecutiveDeferralCount);
    DispatchSteps.push_back(StoredOrderValue.DispatchStep);
    DispatchGameLoops.push_back(StoredOrderValue.DispatchGameLoop);
    ObservedCountsAtDispatch.push_back(StoredOrderValue.ObservedCountAtDispatch);
    ObservedInConstructionCountsAtDispatch.push_back(StoredOrderValue.ObservedInConstructionCountAtDispatch);
    DispatchAttemptCounts.push_back(StoredOrderValue.DispatchAttemptCount);
    OrderIdToIndex[StoredOrderValue.OrderId] = OrderIndexValue;

    MarkDerivedQueuesDirty();
    return StoredOrderValue.OrderId;
}

bool FCommandAuthoritySchedulingState::IsOrderIndexValid(const size_t OrderIndexValue) const
{
    return OrderIndexValue < OrderIds.size();
}

bool FCommandAuthoritySchedulingState::TryGetOrderIndex(const uint32_t OrderIdValue, size_t& OutOrderIndexValue) const
{
    const std::unordered_map<uint32_t, size_t>::const_iterator FoundOrderIndexValue = OrderIdToIndex.find(OrderIdValue);
    if (FoundOrderIndexValue == OrderIdToIndex.end())
    {
        return false;
    }

    OutOrderIndexValue = FoundOrderIndexValue->second;
    return true;
}

bool FCommandAuthoritySchedulingState::TryGetChildOrderIndex(const uint32_t ParentOrderIdValue,
                                                             const ECommandAuthorityLayer SourceLayerValue,
                                                             size_t& OutOrderIndexValue) const
{
    for (size_t OrderIndexValue = 0U; OrderIndexValue < OrderIds.size(); ++OrderIndexValue)
    {
        if (ParentOrderIds[OrderIndexValue] == ParentOrderIdValue && SourceLayers[OrderIndexValue] == SourceLayerValue)
        {
            OutOrderIndexValue = OrderIndexValue;
            return true;
        }
    }

    return false;
}

bool FCommandAuthoritySchedulingState::TryGetActiveChildOrderIndex(const uint32_t ParentOrderIdValue,
                                                                   const ECommandAuthorityLayer SourceLayerValue,
                                                                   size_t& OutOrderIndexValue) const
{
    for (size_t OrderIndexValue = 0U; OrderIndexValue < OrderIds.size(); ++OrderIndexValue)
    {
        if (ParentOrderIds[OrderIndexValue] != ParentOrderIdValue ||
            SourceLayers[OrderIndexValue] != SourceLayerValue ||
            IsTerminalLifecycleState(LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        OutOrderIndexValue = OrderIndexValue;
        return true;
    }

    return false;
}

size_t FCommandAuthoritySchedulingState::GetOrderCount() const
{
    return OrderIds.size();
}

FCommandOrderRecord FCommandAuthoritySchedulingState::GetOrderRecord(const size_t OrderIndexValue) const
{
    FCommandOrderRecord CommandOrderRecordValue;
    if (!IsOrderIndexValid(OrderIndexValue))
    {
        return CommandOrderRecordValue;
    }

    CommandOrderRecordValue.OrderId = OrderIds[OrderIndexValue];
    CommandOrderRecordValue.ParentOrderId = ParentOrderIds[OrderIndexValue];
    CommandOrderRecordValue.SourceGoalId = SourceGoalIds[OrderIndexValue];
    CommandOrderRecordValue.SourceLayer = SourceLayers[OrderIndexValue];
    CommandOrderRecordValue.LifecycleState = LifecycleStates[OrderIndexValue];
    CommandOrderRecordValue.TaskPackageKind = TaskPackageKinds[OrderIndexValue];
    CommandOrderRecordValue.TaskNeedKind = TaskNeedKinds[OrderIndexValue];
    CommandOrderRecordValue.TaskType = TaskTypes[OrderIndexValue];
    CommandOrderRecordValue.RetentionPolicy = RetentionPolicies[OrderIndexValue];
    CommandOrderRecordValue.BlockedTaskWakeKind = BlockedTaskWakeKinds[OrderIndexValue];
    CommandOrderRecordValue.BasePriorityValue = BasePriorityValues[OrderIndexValue];
    CommandOrderRecordValue.EffectivePriorityValue = EffectivePriorityValues[OrderIndexValue];
    CommandOrderRecordValue.PriorityTier = PriorityTiers[OrderIndexValue];
    CommandOrderRecordValue.IntentDomain = IntentDomains[OrderIndexValue];
    CommandOrderRecordValue.CreationStep = CreationSteps[OrderIndexValue];
    CommandOrderRecordValue.DeadlineStep = DeadlineSteps[OrderIndexValue];
    CommandOrderRecordValue.OwningArmyIndex = OwningArmyIndices[OrderIndexValue];
    CommandOrderRecordValue.OwningSquadIndex = OwningSquadIndices[OrderIndexValue];
    CommandOrderRecordValue.ActorTag = ActorTags[OrderIndexValue];
    CommandOrderRecordValue.AbilityId = AbilityIds[OrderIndexValue];
    CommandOrderRecordValue.TargetKind = TargetKinds[OrderIndexValue];
    CommandOrderRecordValue.TargetPoint = TargetPoints[OrderIndexValue];
    CommandOrderRecordValue.TargetUnitTag = TargetUnitTags[OrderIndexValue];
    CommandOrderRecordValue.Queued = QueuedValues[OrderIndexValue];
    CommandOrderRecordValue.RequiresPlacementValidation = RequiresPlacementValidationValues[OrderIndexValue];
    CommandOrderRecordValue.RequiresPathingValidation = RequiresPathingValidationValues[OrderIndexValue];
    CommandOrderRecordValue.PlanStepId = PlanStepIds[OrderIndexValue];
    CommandOrderRecordValue.TargetCount = TargetCounts[OrderIndexValue];
    CommandOrderRecordValue.RequestedQueueCount = RequestedQueueCounts[OrderIndexValue];
    CommandOrderRecordValue.ProducerUnitTypeId = ProducerUnitTypeIds[OrderIndexValue];
    CommandOrderRecordValue.ResultUnitTypeId = ResultUnitTypeIds[OrderIndexValue];
    CommandOrderRecordValue.UpgradeId = UpgradeIds[OrderIndexValue];
    CommandOrderRecordValue.PreferredPlacementSlotType = PreferredPlacementSlotTypes[OrderIndexValue];
    CommandOrderRecordValue.PreferredPlacementSlotId.SlotType = PreferredPlacementSlotIdTypes[OrderIndexValue];
    CommandOrderRecordValue.PreferredPlacementSlotId.Ordinal = PreferredPlacementSlotIdOrdinals[OrderIndexValue];
    CommandOrderRecordValue.ReservedPlacementSlotId.SlotType = ReservedPlacementSlotTypes[OrderIndexValue];
    CommandOrderRecordValue.ReservedPlacementSlotId.Ordinal = ReservedPlacementSlotOrdinals[OrderIndexValue];
    CommandOrderRecordValue.LastDeferralReason = LastDeferralReasons[OrderIndexValue];
    CommandOrderRecordValue.LastDeferralStep = LastDeferralSteps[OrderIndexValue];
    CommandOrderRecordValue.LastDeferralGameLoop = LastDeferralGameLoops[OrderIndexValue];
    CommandOrderRecordValue.ConsecutiveDeferralCount = ConsecutiveDeferralCounts[OrderIndexValue];
    CommandOrderRecordValue.DispatchStep = DispatchSteps[OrderIndexValue];
    CommandOrderRecordValue.DispatchGameLoop = DispatchGameLoops[OrderIndexValue];
    CommandOrderRecordValue.ObservedCountAtDispatch = ObservedCountsAtDispatch[OrderIndexValue];
    CommandOrderRecordValue.ObservedInConstructionCountAtDispatch =
        ObservedInConstructionCountsAtDispatch[OrderIndexValue];
    CommandOrderRecordValue.DispatchAttemptCount = DispatchAttemptCounts[OrderIndexValue];
    return CommandOrderRecordValue;
}

bool FCommandAuthoritySchedulingState::SetOrderLifecycleState(const uint32_t OrderIdValue,
                                                              const EOrderLifecycleState LifecycleStateValue)
{
    size_t OrderIndexValue = 0U;
    if (!TryGetOrderIndex(OrderIdValue, OrderIndexValue))
    {
        return false;
    }

    if (LifecycleStates[OrderIndexValue] == LifecycleStateValue)
    {
        return true;
    }

    LifecycleStates[OrderIndexValue] = LifecycleStateValue;
    bPrioritiesDirty = true;
    if (IsTerminalLifecycleState(LifecycleStateValue))
    {
        ReservedPlacementSlotTypes[OrderIndexValue] = EBuildPlacementSlotType::Unknown;
        ReservedPlacementSlotOrdinals[OrderIndexValue] = 0U;
    }
    MarkDerivedQueuesDirty();
    return true;
}

bool FCommandAuthoritySchedulingState::SetOrderDeferralState(const uint32_t OrderIdValue,
                                                             const ECommandOrderDeferralReason DeferralReasonValue,
                                                             const uint64_t CurrentStepValue,
                                                             const uint64_t CurrentGameLoopValue)
{
    size_t OrderIndexValue = 0U;
    if (!TryGetOrderIndex(OrderIdValue, OrderIndexValue))
    {
        return false;
    }

    if (LastDeferralReasons[OrderIndexValue] == DeferralReasonValue)
    {
        ++ConsecutiveDeferralCounts[OrderIndexValue];
    }
    else
    {
        ConsecutiveDeferralCounts[OrderIndexValue] = 1U;
    }
    LastDeferralReasons[OrderIndexValue] = DeferralReasonValue;
    LastDeferralSteps[OrderIndexValue] = CurrentStepValue;
    LastDeferralGameLoops[OrderIndexValue] = CurrentGameLoopValue;
    bPrioritiesDirty = true;
    return true;
}

bool FCommandAuthoritySchedulingState::ClearOrderDeferralState(const uint32_t OrderIdValue)
{
    size_t OrderIndexValue = 0U;
    if (!TryGetOrderIndex(OrderIdValue, OrderIndexValue))
    {
        return false;
    }

    LastDeferralReasons[OrderIndexValue] = ECommandOrderDeferralReason::None;
    LastDeferralSteps[OrderIndexValue] = 0U;
    LastDeferralGameLoops[OrderIndexValue] = 0U;
    ConsecutiveDeferralCounts[OrderIndexValue] = 0U;
    bPrioritiesDirty = true;
    return true;
}

bool FCommandAuthoritySchedulingState::SetOrderDispatchState(const uint32_t OrderIdValue,
                                                             const uint64_t DispatchStepValue,
                                                             const uint64_t DispatchGameLoopValue,
                                                             const uint32_t ObservedCountValue,
                                                             const uint32_t ObservedInConstructionCountValue)
{
    size_t OrderIndexValue = 0U;
    if (!TryGetOrderIndex(OrderIdValue, OrderIndexValue))
    {
        return false;
    }

    DispatchSteps[OrderIndexValue] = DispatchStepValue;
    DispatchGameLoops[OrderIndexValue] = DispatchGameLoopValue;
    ObservedCountsAtDispatch[OrderIndexValue] = ObservedCountValue;
    ObservedInConstructionCountsAtDispatch[OrderIndexValue] = ObservedInConstructionCountValue;
    ++DispatchAttemptCounts[OrderIndexValue];
    LastDeferralReasons[OrderIndexValue] = ECommandOrderDeferralReason::None;
    LastDeferralSteps[OrderIndexValue] = 0U;
    LastDeferralGameLoops[OrderIndexValue] = 0U;
    ConsecutiveDeferralCounts[OrderIndexValue] = 0U;
    return true;
}

bool FCommandAuthoritySchedulingState::SetOrderReservedPlacementSlot(
    const uint32_t OrderIdValue, const FBuildPlacementSlotId& BuildPlacementSlotIdValue)
{
    size_t OrderIndexValue = 0U;
    if (!TryGetOrderIndex(OrderIdValue, OrderIndexValue))
    {
        return false;
    }

    ReservedPlacementSlotTypes[OrderIndexValue] = BuildPlacementSlotIdValue.SlotType;
    ReservedPlacementSlotOrdinals[OrderIndexValue] = BuildPlacementSlotIdValue.Ordinal;
    return true;
}

bool FCommandAuthoritySchedulingState::ClearOrderReservedPlacementSlot(const uint32_t OrderIdValue)
{
    size_t OrderIndexValue = 0U;
    if (!TryGetOrderIndex(OrderIdValue, OrderIndexValue))
    {
        return false;
    }

    ReservedPlacementSlotTypes[OrderIndexValue] = EBuildPlacementSlotType::Unknown;
    ReservedPlacementSlotOrdinals[OrderIndexValue] = 0U;
    return true;
}

bool FCommandAuthoritySchedulingState::CompactTerminalOrders()
{
    if (OrderIds.empty())
    {
        return false;
    }

    std::vector<size_t> RetainedOrderIndicesValue;
    RetainedOrderIndicesValue.reserve(OrderIds.size());

    bool bCompactedAnyOrderValue = false;
    for (size_t OrderIndexValue = 0U; OrderIndexValue < OrderIds.size(); ++OrderIndexValue)
    {
        if (ShouldCompactTerminalOrder(*this, OrderIndexValue))
        {
            bCompactedAnyOrderValue = true;
            continue;
        }

        RetainedOrderIndicesValue.push_back(OrderIndexValue);
    }

    if (!bCompactedAnyOrderValue)
    {
        return false;
    }

    OrderIds = BuildCompactedVector(OrderIds, RetainedOrderIndicesValue);
    ParentOrderIds = BuildCompactedVector(ParentOrderIds, RetainedOrderIndicesValue);
    SourceGoalIds = BuildCompactedVector(SourceGoalIds, RetainedOrderIndicesValue);
    SourceLayers = BuildCompactedVector(SourceLayers, RetainedOrderIndicesValue);
    LifecycleStates = BuildCompactedVector(LifecycleStates, RetainedOrderIndicesValue);
    TaskPackageKinds = BuildCompactedVector(TaskPackageKinds, RetainedOrderIndicesValue);
    TaskNeedKinds = BuildCompactedVector(TaskNeedKinds, RetainedOrderIndicesValue);
    TaskTypes = BuildCompactedVector(TaskTypes, RetainedOrderIndicesValue);
    RetentionPolicies = BuildCompactedVector(RetentionPolicies, RetainedOrderIndicesValue);
    BlockedTaskWakeKinds = BuildCompactedVector(BlockedTaskWakeKinds, RetainedOrderIndicesValue);
    BasePriorityValues = BuildCompactedVector(BasePriorityValues, RetainedOrderIndicesValue);
    EffectivePriorityValues = BuildCompactedVector(EffectivePriorityValues, RetainedOrderIndicesValue);
    PriorityTiers = BuildCompactedVector(PriorityTiers, RetainedOrderIndicesValue);
    IntentDomains = BuildCompactedVector(IntentDomains, RetainedOrderIndicesValue);
    CreationSteps = BuildCompactedVector(CreationSteps, RetainedOrderIndicesValue);
    DeadlineSteps = BuildCompactedVector(DeadlineSteps, RetainedOrderIndicesValue);
    OwningArmyIndices = BuildCompactedVector(OwningArmyIndices, RetainedOrderIndicesValue);
    OwningSquadIndices = BuildCompactedVector(OwningSquadIndices, RetainedOrderIndicesValue);
    ActorTags = BuildCompactedVector(ActorTags, RetainedOrderIndicesValue);
    AbilityIds = BuildCompactedVector(AbilityIds, RetainedOrderIndicesValue);
    TargetKinds = BuildCompactedVector(TargetKinds, RetainedOrderIndicesValue);
    TargetPoints = BuildCompactedVector(TargetPoints, RetainedOrderIndicesValue);
    TargetUnitTags = BuildCompactedVector(TargetUnitTags, RetainedOrderIndicesValue);
    QueuedValues = BuildCompactedVector(QueuedValues, RetainedOrderIndicesValue);
    RequiresPlacementValidationValues =
        BuildCompactedVector(RequiresPlacementValidationValues, RetainedOrderIndicesValue);
    RequiresPathingValidationValues = BuildCompactedVector(RequiresPathingValidationValues, RetainedOrderIndicesValue);
    PlanStepIds = BuildCompactedVector(PlanStepIds, RetainedOrderIndicesValue);
    TargetCounts = BuildCompactedVector(TargetCounts, RetainedOrderIndicesValue);
    RequestedQueueCounts = BuildCompactedVector(RequestedQueueCounts, RetainedOrderIndicesValue);
    ProducerUnitTypeIds = BuildCompactedVector(ProducerUnitTypeIds, RetainedOrderIndicesValue);
    ResultUnitTypeIds = BuildCompactedVector(ResultUnitTypeIds, RetainedOrderIndicesValue);
    UpgradeIds = BuildCompactedVector(UpgradeIds, RetainedOrderIndicesValue);
    PreferredPlacementSlotTypes = BuildCompactedVector(PreferredPlacementSlotTypes, RetainedOrderIndicesValue);
    PreferredPlacementSlotIdTypes = BuildCompactedVector(PreferredPlacementSlotIdTypes, RetainedOrderIndicesValue);
    PreferredPlacementSlotIdOrdinals =
        BuildCompactedVector(PreferredPlacementSlotIdOrdinals, RetainedOrderIndicesValue);
    ReservedPlacementSlotTypes = BuildCompactedVector(ReservedPlacementSlotTypes, RetainedOrderIndicesValue);
    ReservedPlacementSlotOrdinals = BuildCompactedVector(ReservedPlacementSlotOrdinals, RetainedOrderIndicesValue);
    LastDeferralReasons = BuildCompactedVector(LastDeferralReasons, RetainedOrderIndicesValue);
    LastDeferralSteps = BuildCompactedVector(LastDeferralSteps, RetainedOrderIndicesValue);
    LastDeferralGameLoops = BuildCompactedVector(LastDeferralGameLoops, RetainedOrderIndicesValue);
    ConsecutiveDeferralCounts = BuildCompactedVector(ConsecutiveDeferralCounts, RetainedOrderIndicesValue);
    DispatchSteps = BuildCompactedVector(DispatchSteps, RetainedOrderIndicesValue);
    DispatchGameLoops = BuildCompactedVector(DispatchGameLoops, RetainedOrderIndicesValue);
    ObservedCountsAtDispatch = BuildCompactedVector(ObservedCountsAtDispatch, RetainedOrderIndicesValue);
    ObservedInConstructionCountsAtDispatch =
        BuildCompactedVector(ObservedInConstructionCountsAtDispatch, RetainedOrderIndicesValue);
    DispatchAttemptCounts = BuildCompactedVector(DispatchAttemptCounts, RetainedOrderIndicesValue);

    OrderIdToIndex.clear();
    for (size_t OrderIndexValue = 0U; OrderIndexValue < OrderIds.size(); ++OrderIndexValue)
    {
        OrderIdToIndex.emplace(OrderIds[OrderIndexValue], OrderIndexValue);
    }

    MarkDerivedQueuesDirty();
    return true;
}

size_t FCommandAuthoritySchedulingState::GetActiveOrderCountForLayer(
    const ECommandAuthorityLayer SourceLayerValue) const
{
    size_t ActiveOrderCountValue = 0U;

    for (size_t OrderIndexValue = 0U; OrderIndexValue < OrderIds.size(); ++OrderIndexValue)
    {
        if (SourceLayers[OrderIndexValue] != SourceLayerValue ||
            IsTerminalLifecycleState(LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        ++ActiveOrderCountValue;
    }

    return ActiveOrderCountValue;
}

void FCommandAuthoritySchedulingState::RebuildDerivedQueues()
{
    bDerivedQueuesDirty = false;
    StrategicOrderIndices.clear();
    PlanningProcessIndices.clear();
    ArmyOrderIndices.clear();
    SquadOrderIndices.clear();
    ReadyIntentIndices.clear();
    DispatchedOrderIndices.clear();
    CompletedOrderIndices.clear();
    for (std::vector<size_t>& StrategicQueueValue : StrategicQueues)
    {
        StrategicQueueValue.clear();
    }
    for (std::vector<size_t>& PlanningQueueValue : PlanningQueues)
    {
        PlanningQueueValue.clear();
    }
    for (std::vector<size_t>& ArmyQueueValue : ArmyQueues)
    {
        ArmyQueueValue.clear();
    }
    for (std::vector<size_t>& SquadQueueValue : SquadQueues)
    {
        SquadQueueValue.clear();
    }
    for (std::array<std::vector<size_t>, IntentDomainCountValue>& TierQueueGroupValue : ReadyIntentQueues)
    {
        for (std::vector<size_t>& ReadyQueueValue : TierQueueGroupValue)
        {
            ReadyQueueValue.clear();
        }
    }

    for (size_t OrderIndexValue = 0U; OrderIndexValue < OrderIds.size(); ++OrderIndexValue)
    {
        switch (LifecycleStates[OrderIndexValue])
        {
            case EOrderLifecycleState::Queued:
                AppendQueuedOrderIndex(OrderIndexValue);
                break;
            case EOrderLifecycleState::Preprocessing:
                PlanningProcessIndices.push_back(OrderIndexValue);
                PlanningQueues[GetCommandPriorityTierIndex(PriorityTiers[OrderIndexValue])].push_back(OrderIndexValue);
                break;
            case EOrderLifecycleState::Ready:
                ReadyIntentQueues[GetCommandPriorityTierIndex(PriorityTiers[OrderIndexValue])]
                                 [GetIntentDomainIndex(IntentDomains[OrderIndexValue])]
                                     .push_back(OrderIndexValue);
                break;
            case EOrderLifecycleState::Dispatched:
                DispatchedOrderIndices.push_back(OrderIndexValue);
                break;
            case EOrderLifecycleState::Completed:
            case EOrderLifecycleState::Aborted:
            case EOrderLifecycleState::Expired:
                CompletedOrderIndices.push_back(OrderIndexValue);
                break;
            default:
                break;
        }
    }

    SortDerivedQueues();
    RebuildProcessorState();
    RebuildPlaybackState();
}

void FCommandAuthoritySchedulingState::RebuildProcessorState()
{
    if (!ReadyIntentIndices.empty())
    {
        ProcessorState = EPlanningProcessorState::ReadyToDrain;
        return;
    }

    if (!StrategicOrderIndices.empty() || !PlanningProcessIndices.empty() || !ArmyOrderIndices.empty() ||
        !SquadOrderIndices.empty())
    {
        ProcessorState = EPlanningProcessorState::Processing;
        return;
    }

    ProcessorState = EPlanningProcessorState::Idle;
}

void FCommandAuthoritySchedulingState::RebuildPlaybackState()
{
    if (!ReadyIntentIndices.empty())
    {
        PlaybackState = EIntentPlaybackState::ReadyBufferPending;
        return;
    }

    if (!DispatchedOrderIndices.empty())
    {
        PlaybackState = EIntentPlaybackState::Dispatching;
        return;
    }

    if (!StrategicOrderIndices.empty() || !PlanningProcessIndices.empty() || !ArmyOrderIndices.empty() ||
        !SquadOrderIndices.empty())
    {
        PlaybackState = EIntentPlaybackState::Blocked;
        return;
    }

    PlaybackState = EIntentPlaybackState::Idle;
}

void FCommandAuthoritySchedulingState::AppendQueuedOrderIndex(const size_t OrderIndexValue)
{
    const size_t PriorityTierIndexValue = GetCommandPriorityTierIndex(PriorityTiers[OrderIndexValue]);
    switch (SourceLayers[OrderIndexValue])
    {
        case ECommandAuthorityLayer::Agent:
        case ECommandAuthorityLayer::StrategicDirector:
            StrategicQueues[PriorityTierIndexValue].push_back(OrderIndexValue);
            break;
        case ECommandAuthorityLayer::EconomyAndProduction:
            PlanningQueues[PriorityTierIndexValue].push_back(OrderIndexValue);
            break;
        case ECommandAuthorityLayer::Army:
            ArmyQueues[PriorityTierIndexValue].push_back(OrderIndexValue);
            break;
        case ECommandAuthorityLayer::Squad:
            SquadQueues[PriorityTierIndexValue].push_back(OrderIndexValue);
            break;
        case ECommandAuthorityLayer::UnitExecution:
            PlanningQueues[PriorityTierIndexValue].push_back(OrderIndexValue);
            break;
        default:
            break;
    }
}

void FCommandAuthoritySchedulingState::SortDerivedQueues()
{
    const auto OrderPriorityComparatorValue =
        [this](const size_t LeftOrderIndexValue, const size_t RightOrderIndexValue)
        {
            if (EffectivePriorityValues[LeftOrderIndexValue] != EffectivePriorityValues[RightOrderIndexValue])
            {
                return EffectivePriorityValues[LeftOrderIndexValue] > EffectivePriorityValues[RightOrderIndexValue];
            }
            if (GetIntentDomainOrder(IntentDomains[LeftOrderIndexValue]) !=
                GetIntentDomainOrder(IntentDomains[RightOrderIndexValue]))
            {
                return GetIntentDomainOrder(IntentDomains[LeftOrderIndexValue]) <
                       GetIntentDomainOrder(IntentDomains[RightOrderIndexValue]);
            }
            if (CreationSteps[LeftOrderIndexValue] != CreationSteps[RightOrderIndexValue])
            {
                return CreationSteps[LeftOrderIndexValue] < CreationSteps[RightOrderIndexValue];
            }
            return OrderIds[LeftOrderIndexValue] < OrderIds[RightOrderIndexValue];
        };

    StrategicOrderIndices.clear();
    PlanningProcessIndices.clear();
    ArmyOrderIndices.clear();
    SquadOrderIndices.clear();
    ReadyIntentIndices.clear();

    for (size_t PriorityTierIndexValue = 0U; PriorityTierIndexValue < CommandPriorityTierCountValue;
         ++PriorityTierIndexValue)
    {
        std::stable_sort(StrategicQueues[PriorityTierIndexValue].begin(), StrategicQueues[PriorityTierIndexValue].end(),
                         OrderPriorityComparatorValue);
        StrategicOrderIndices.insert(StrategicOrderIndices.end(), StrategicQueues[PriorityTierIndexValue].begin(),
                                     StrategicQueues[PriorityTierIndexValue].end());

        std::stable_sort(PlanningQueues[PriorityTierIndexValue].begin(), PlanningQueues[PriorityTierIndexValue].end(),
                         OrderPriorityComparatorValue);
        PlanningProcessIndices.insert(PlanningProcessIndices.end(), PlanningQueues[PriorityTierIndexValue].begin(),
                                      PlanningQueues[PriorityTierIndexValue].end());

        std::stable_sort(ArmyQueues[PriorityTierIndexValue].begin(), ArmyQueues[PriorityTierIndexValue].end(),
                         OrderPriorityComparatorValue);
        ArmyOrderIndices.insert(ArmyOrderIndices.end(), ArmyQueues[PriorityTierIndexValue].begin(),
                                ArmyQueues[PriorityTierIndexValue].end());

        std::stable_sort(SquadQueues[PriorityTierIndexValue].begin(), SquadQueues[PriorityTierIndexValue].end(),
                         OrderPriorityComparatorValue);
        SquadOrderIndices.insert(SquadOrderIndices.end(), SquadQueues[PriorityTierIndexValue].begin(),
                                 SquadQueues[PriorityTierIndexValue].end());

        for (size_t IntentDomainIndexValue = 0U; IntentDomainIndexValue < IntentDomainCountValue;
             ++IntentDomainIndexValue)
        {
            std::stable_sort(ReadyIntentQueues[PriorityTierIndexValue][IntentDomainIndexValue].begin(),
                             ReadyIntentQueues[PriorityTierIndexValue][IntentDomainIndexValue].end(),
                             OrderPriorityComparatorValue);
            ReadyIntentIndices.insert(
                ReadyIntentIndices.end(),
                ReadyIntentQueues[PriorityTierIndexValue][IntentDomainIndexValue].begin(),
                ReadyIntentQueues[PriorityTierIndexValue][IntentDomainIndexValue].end());
        }
    }

    std::stable_sort(DispatchedOrderIndices.begin(), DispatchedOrderIndices.end(), OrderPriorityComparatorValue);
    std::stable_sort(CompletedOrderIndices.begin(), CompletedOrderIndices.end(), OrderPriorityComparatorValue);
}

void FCommandAuthoritySchedulingState::MarkDerivedQueuesDirty()
{
    bDerivedQueuesDirty = true;
    bPrioritiesDirty = true;
    if (MutationBatchDepth == 0U)
    {
        RebuildDerivedQueues();
    }
}

}  // namespace sc2
