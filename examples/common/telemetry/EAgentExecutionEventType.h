#pragma once

namespace sc2
{

enum class EAgentExecutionEventType
{
    SupplyBlockedStarted,
    SupplyBlockedEnded,
    MineralBankStarted,
    MineralBankEnded,
    ActorIntentConflict,
    IdleProductionStructure,
    SchedulerOrderDeferred,
};

const char* ToString(EAgentExecutionEventType AgentExecutionEventTypeValue);

}  // namespace sc2
