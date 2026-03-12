#pragma once

#include <cstdint>

#include "common/planning/EOrderLifecycleState.h"
#include "common/planning/FCommandAuthoritySchedulingState.h"

namespace sc2
{

struct FIntentBuffer;

class IIntentSchedulingService
{
public:
    virtual ~IIntentSchedulingService();

    virtual void ResetSchedulingState(FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue) const = 0;
    virtual uint32_t SubmitOrder(FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                 const FCommandOrderRecord& CommandOrderRecordValue) const = 0;
    virtual bool UpdateOrderLifecycleState(FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                           uint32_t OrderIdValue, EOrderLifecycleState LifecycleStateValue) const = 0;
    virtual uint32_t DrainReadyIntents(FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                       FIntentBuffer& IntentBufferValue, uint32_t MaxIntentCountValue) const = 0;
};

}  // namespace sc2
