#include "common/telemetry/EAgentExecutionEventType.h"

namespace sc2
{

const char* ToString(const EAgentExecutionEventType AgentExecutionEventTypeValue)
{
    switch (AgentExecutionEventTypeValue)
    {
        case EAgentExecutionEventType::SupplyBlockedStarted:
            return "SupplyBlockedStarted";
        case EAgentExecutionEventType::SupplyBlockedEnded:
            return "SupplyBlockedEnded";
        case EAgentExecutionEventType::MineralBankStarted:
            return "MineralBankStarted";
        case EAgentExecutionEventType::MineralBankEnded:
            return "MineralBankEnded";
        case EAgentExecutionEventType::ActorIntentConflict:
            return "ActorIntentConflict";
        case EAgentExecutionEventType::IdleProductionStructure:
            return "IdleProductionStructure";
        case EAgentExecutionEventType::SchedulerOrderDeferred:
            return "SchedulerOrderDeferred";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
