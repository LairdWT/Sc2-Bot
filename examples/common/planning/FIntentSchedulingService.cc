#include "common/planning/FIntentSchedulingService.h"

#include <algorithm>
#include <vector>

#include "common/agent_framework.h"

namespace sc2
{

void FIntentSchedulingService::ResetSchedulingState(
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue) const
{
    CommandAuthoritySchedulingStateValue.Reset();
}

uint32_t FIntentSchedulingService::SubmitOrder(
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
    const FCommandOrderRecord& CommandOrderRecordValue) const
{
    return CommandAuthoritySchedulingStateValue.EnqueueOrder(CommandOrderRecordValue);
}

bool FIntentSchedulingService::UpdateOrderLifecycleState(
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue, const uint32_t OrderIdValue,
    const EOrderLifecycleState LifecycleStateValue) const
{
    return CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(OrderIdValue, LifecycleStateValue);
}

uint32_t FIntentSchedulingService::DrainReadyIntents(
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue, FIntentBuffer& IntentBufferValue,
    const uint32_t MaxIntentCountValue) const
{
    const uint32_t EffectiveMaxIntentCountValue =
        std::min(MaxIntentCountValue, CommandAuthoritySchedulingStateValue.MaxUnitIntentsPerStep);
    if (EffectiveMaxIntentCountValue == 0U)
    {
        return 0U;
    }

    std::vector<uint32_t> DispatchedOrderIds;
    std::vector<uint32_t> AbortedOrderIds;
    uint32_t DrainedIntentCountValue = 0U;

    const size_t ReadyOrderCountValue = CommandAuthoritySchedulingStateValue.ReadyIntentIndices.size();
    for (size_t ReadyOrderIndexValue = 0U; ReadyOrderIndexValue < ReadyOrderCountValue; ++ReadyOrderIndexValue)
    {
        if (DrainedIntentCountValue >= EffectiveMaxIntentCountValue)
        {
            break;
        }

        const size_t OrderIndexValue = CommandAuthoritySchedulingStateValue.ReadyIntentIndices[ReadyOrderIndexValue];
        const FCommandOrderRecord CommandOrderRecordValue =
            CommandAuthoritySchedulingStateValue.GetOrderRecord(OrderIndexValue);
        if (CommandOrderRecordValue.ActorTag == NullTag || CommandOrderRecordValue.AbilityId == ABILITY_ID::INVALID)
        {
            AbortedOrderIds.push_back(CommandOrderRecordValue.OrderId);
            continue;
        }

        AppendCommandOrderToIntentBuffer(CommandOrderRecordValue, IntentBufferValue);
        DispatchedOrderIds.push_back(CommandOrderRecordValue.OrderId);
        ++DrainedIntentCountValue;
    }

    for (const uint32_t AbortedOrderIdValue : AbortedOrderIds)
    {
        CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(AbortedOrderIdValue, EOrderLifecycleState::Aborted);
    }
    for (const uint32_t DispatchedOrderIdValue : DispatchedOrderIds)
    {
        CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(DispatchedOrderIdValue,
                                                                    EOrderLifecycleState::Dispatched);
    }

    return DrainedIntentCountValue;
}

void FIntentSchedulingService::AppendCommandOrderToIntentBuffer(const FCommandOrderRecord& CommandOrderRecordValue,
                                                                FIntentBuffer& IntentBufferValue) const
{
    switch (CommandOrderRecordValue.TargetKind)
    {
        case EIntentTargetKind::None:
            IntentBufferValue.Add(FUnitIntent::CreateNoTarget(CommandOrderRecordValue.ActorTag,
                                                              CommandOrderRecordValue.AbilityId,
                                                              CommandOrderRecordValue.EffectivePriorityValue,
                                                              CommandOrderRecordValue.IntentDomain,
                                                              CommandOrderRecordValue.Queued));
            break;
        case EIntentTargetKind::Point:
            IntentBufferValue.Add(FUnitIntent::CreatePointTarget(CommandOrderRecordValue.ActorTag,
                                                                 CommandOrderRecordValue.AbilityId,
                                                                 CommandOrderRecordValue.TargetPoint,
                                                                 CommandOrderRecordValue.EffectivePriorityValue,
                                                                 CommandOrderRecordValue.IntentDomain,
                                                                 CommandOrderRecordValue.RequiresPathingValidation,
                                                                 CommandOrderRecordValue.RequiresPlacementValidation,
                                                                 CommandOrderRecordValue.Queued));
            break;
        case EIntentTargetKind::Unit:
            IntentBufferValue.Add(FUnitIntent::CreateUnitTarget(CommandOrderRecordValue.ActorTag,
                                                                CommandOrderRecordValue.AbilityId,
                                                                CommandOrderRecordValue.TargetUnitTag,
                                                                CommandOrderRecordValue.EffectivePriorityValue,
                                                                CommandOrderRecordValue.IntentDomain,
                                                                CommandOrderRecordValue.Queued));
            break;
        default:
            break;
    }
}

}  // namespace sc2
