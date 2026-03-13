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
};

const char* ToString(EAgentExecutionEventType AgentExecutionEventTypeValue);

}  // namespace sc2
