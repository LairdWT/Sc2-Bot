#include "common/planning/FCommandAuthoritySchedulingState.h"

#include <algorithm>

#include "common/agent_framework.h"

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
    RebuildDerivedQueues();
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
