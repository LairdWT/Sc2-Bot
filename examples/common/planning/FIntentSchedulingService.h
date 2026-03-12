#pragma once

#include "common/planning/IIntentSchedulingService.h"

namespace sc2
{

struct FIntentBuffer;

class FIntentSchedulingService final : public IIntentSchedulingService
{
public:
    void ResetSchedulingState(FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue) const final;
    uint32_t SubmitOrder(FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                         const FCommandOrderRecord& CommandOrderRecordValue) const final;
    bool UpdateOrderLifecycleState(FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                   uint32_t OrderIdValue, EOrderLifecycleState LifecycleStateValue) const final;
    uint32_t DrainReadyIntents(FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                               FIntentBuffer& IntentBufferValue, uint32_t MaxIntentCountValue) const final;

private:
    void AppendCommandOrderToIntentBuffer(const FCommandOrderRecord& CommandOrderRecordValue,
                                          FIntentBuffer& IntentBufferValue) const;
};

}  // namespace sc2
