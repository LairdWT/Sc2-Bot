#pragma once

#include <cstdint>

#include "common/planning/ECommandOrderDeferralReason.h"
#include "common/planning/EIntentDomain.h"
#include "common/telemetry/EAgentExecutionEventType.h"
#include "sc2api/sc2_common.h"
#include "sc2api/sc2_gametypes.h"
#include "sc2api/sc2_typeenums.h"

namespace sc2
{

struct FExecutionEventRecord
{
public:
    FExecutionEventRecord();

    void Reset();

public:
    EAgentExecutionEventType EventType;
    uint64_t Step;
    uint64_t GameLoop;
    Tag ActorTag;
    AbilityID AbilityId;
    EIntentDomain IntentDomain;
    UNIT_TYPEID UnitTypeId;
    uint32_t OrderId;
    uint32_t PlanStepId;
    ECommandOrderDeferralReason DeferralReason;
    uint64_t MetricValue;
};

}  // namespace sc2
