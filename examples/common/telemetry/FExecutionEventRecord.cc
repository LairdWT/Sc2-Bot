#include "common/telemetry/FExecutionEventRecord.h"

namespace sc2
{

FExecutionEventRecord::FExecutionEventRecord()
{
    Reset();
}

void FExecutionEventRecord::Reset()
{
    EventType = EAgentExecutionEventType::ActorIntentConflict;
    Step = 0U;
    GameLoop = 0U;
    ActorTag = NullTag;
    AbilityId = ABILITY_ID::INVALID;
    IntentDomain = EIntentDomain::Recovery;
    UnitTypeId = UNIT_TYPEID::INVALID;
    OrderId = 0U;
    PlanStepId = 0U;
    DeferralReason = ECommandOrderDeferralReason::None;
    MetricValue = 0U;
}

}  // namespace sc2
