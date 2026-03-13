#include "common/planning/FCommandAuthoritySchedulingState.h"

#include <algorithm>

namespace sc2
{

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

    OrderIds.clear();
    ParentOrderIds.clear();
    SourceLayers.clear();
    LifecycleStates.clear();
    PriorityValues.clear();
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
    ProducerUnitTypeIds.clear();
    ResultUnitTypeIds.clear();
    UpgradeIds.clear();
    PreferredPlacementSlotTypes.clear();
    ReservedPlacementSlotTypes.clear();
    ReservedPlacementSlotOrdinals.clear();
    LastDeferralReasons.clear();
    LastDeferralSteps.clear();
    LastDeferralGameLoops.clear();
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
    CompletedOrderIndices.clear();
}

void FCommandAuthoritySchedulingState::Reserve(const size_t OrderCapacityValue)
{
    OrderIds.reserve(OrderCapacityValue);
    ParentOrderIds.reserve(OrderCapacityValue);
    SourceLayers.reserve(OrderCapacityValue);
    LifecycleStates.reserve(OrderCapacityValue);
    PriorityValues.reserve(OrderCapacityValue);
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
    ProducerUnitTypeIds.reserve(OrderCapacityValue);
    ResultUnitTypeIds.reserve(OrderCapacityValue);
    UpgradeIds.reserve(OrderCapacityValue);
    PreferredPlacementSlotTypes.reserve(OrderCapacityValue);
    ReservedPlacementSlotTypes.reserve(OrderCapacityValue);
    ReservedPlacementSlotOrdinals.reserve(OrderCapacityValue);
    LastDeferralReasons.reserve(OrderCapacityValue);
    LastDeferralSteps.reserve(OrderCapacityValue);
    LastDeferralGameLoops.reserve(OrderCapacityValue);
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
    SourceLayers.push_back(StoredOrderValue.SourceLayer);
    LifecycleStates.push_back(StoredOrderValue.LifecycleState);
    PriorityValues.push_back(StoredOrderValue.PriorityValue);
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
    ProducerUnitTypeIds.push_back(StoredOrderValue.ProducerUnitTypeId);
    ResultUnitTypeIds.push_back(StoredOrderValue.ResultUnitTypeId);
    UpgradeIds.push_back(StoredOrderValue.UpgradeId);
    PreferredPlacementSlotTypes.push_back(StoredOrderValue.PreferredPlacementSlotType);
    ReservedPlacementSlotTypes.push_back(StoredOrderValue.ReservedPlacementSlotId.SlotType);
    ReservedPlacementSlotOrdinals.push_back(StoredOrderValue.ReservedPlacementSlotId.Ordinal);
    LastDeferralReasons.push_back(StoredOrderValue.LastDeferralReason);
    LastDeferralSteps.push_back(StoredOrderValue.LastDeferralStep);
    LastDeferralGameLoops.push_back(StoredOrderValue.LastDeferralGameLoop);
    DispatchSteps.push_back(StoredOrderValue.DispatchStep);
    DispatchGameLoops.push_back(StoredOrderValue.DispatchGameLoop);
    ObservedCountsAtDispatch.push_back(StoredOrderValue.ObservedCountAtDispatch);
    ObservedInConstructionCountsAtDispatch.push_back(StoredOrderValue.ObservedInConstructionCountAtDispatch);
    DispatchAttemptCounts.push_back(StoredOrderValue.DispatchAttemptCount);
    OrderIdToIndex[StoredOrderValue.OrderId] = OrderIndexValue;

    RebuildDerivedQueues();
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
    CommandOrderRecordValue.SourceLayer = SourceLayers[OrderIndexValue];
    CommandOrderRecordValue.LifecycleState = LifecycleStates[OrderIndexValue];
    CommandOrderRecordValue.PriorityValue = PriorityValues[OrderIndexValue];
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
    CommandOrderRecordValue.ProducerUnitTypeId = ProducerUnitTypeIds[OrderIndexValue];
    CommandOrderRecordValue.ResultUnitTypeId = ResultUnitTypeIds[OrderIndexValue];
    CommandOrderRecordValue.UpgradeId = UpgradeIds[OrderIndexValue];
    CommandOrderRecordValue.PreferredPlacementSlotType = PreferredPlacementSlotTypes[OrderIndexValue];
    CommandOrderRecordValue.ReservedPlacementSlotId.SlotType = ReservedPlacementSlotTypes[OrderIndexValue];
    CommandOrderRecordValue.ReservedPlacementSlotId.Ordinal = ReservedPlacementSlotOrdinals[OrderIndexValue];
    CommandOrderRecordValue.LastDeferralReason = LastDeferralReasons[OrderIndexValue];
    CommandOrderRecordValue.LastDeferralStep = LastDeferralSteps[OrderIndexValue];
    CommandOrderRecordValue.LastDeferralGameLoop = LastDeferralGameLoops[OrderIndexValue];
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

    LifecycleStates[OrderIndexValue] = LifecycleStateValue;
    if (IsTerminalLifecycleState(LifecycleStateValue))
    {
        ReservedPlacementSlotTypes[OrderIndexValue] = EBuildPlacementSlotType::Unknown;
        ReservedPlacementSlotOrdinals[OrderIndexValue] = 0U;
    }
    RebuildDerivedQueues();
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

    LastDeferralReasons[OrderIndexValue] = DeferralReasonValue;
    LastDeferralSteps[OrderIndexValue] = CurrentStepValue;
    LastDeferralGameLoops[OrderIndexValue] = CurrentGameLoopValue;
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

void FCommandAuthoritySchedulingState::RebuildDerivedQueues()
{
    StrategicOrderIndices.clear();
    PlanningProcessIndices.clear();
    ArmyOrderIndices.clear();
    SquadOrderIndices.clear();
    ReadyIntentIndices.clear();
    CompletedOrderIndices.clear();

    for (size_t OrderIndexValue = 0U; OrderIndexValue < OrderIds.size(); ++OrderIndexValue)
    {
        switch (LifecycleStates[OrderIndexValue])
        {
            case EOrderLifecycleState::Queued:
                AppendQueuedOrderIndex(OrderIndexValue);
                break;
            case EOrderLifecycleState::Preprocessing:
                PlanningProcessIndices.push_back(OrderIndexValue);
                break;
            case EOrderLifecycleState::Ready:
                ReadyIntentIndices.push_back(OrderIndexValue);
                break;
            case EOrderLifecycleState::Completed:
            case EOrderLifecycleState::Aborted:
            case EOrderLifecycleState::Expired:
                CompletedOrderIndices.push_back(OrderIndexValue);
                break;
            case EOrderLifecycleState::Dispatched:
                break;
            default:
                break;
        }
    }

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

    if (std::find(LifecycleStates.begin(), LifecycleStates.end(), EOrderLifecycleState::Dispatched) != LifecycleStates.end())
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
    switch (SourceLayers[OrderIndexValue])
    {
        case ECommandAuthorityLayer::Agent:
        case ECommandAuthorityLayer::StrategicDirector:
            StrategicOrderIndices.push_back(OrderIndexValue);
            break;
        case ECommandAuthorityLayer::EconomyAndProduction:
            PlanningProcessIndices.push_back(OrderIndexValue);
            break;
        case ECommandAuthorityLayer::Army:
            ArmyOrderIndices.push_back(OrderIndexValue);
            break;
        case ECommandAuthorityLayer::Squad:
            SquadOrderIndices.push_back(OrderIndexValue);
            break;
        case ECommandAuthorityLayer::UnitExecution:
            PlanningProcessIndices.push_back(OrderIndexValue);
            break;
        default:
            break;
    }
}

}  // namespace sc2
