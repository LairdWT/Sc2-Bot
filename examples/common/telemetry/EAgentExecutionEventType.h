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
    WallDescriptorInvalid,
    WallThreatDetected,
    WallOpened,
    WallClosed,
};

const char* ToString(EAgentExecutionEventType AgentExecutionEventTypeValue);

}  // namespace sc2
