#pragma once

#include <cstdint>

#include "common/telemetry/EExecutionConditionState.h"

namespace sc2
{

struct FExecutionPressureDescriptor
{
public:
    FExecutionPressureDescriptor();

    void Reset();
    uint32_t GetIdleCombatProductionStructureCount() const;

public:
    EExecutionConditionState SupplyBlockState;
    uint64_t SupplyBlockDurationGameLoops;
    EExecutionConditionState MineralBankState;
    uint64_t MineralBankDurationGameLoops;
    uint32_t CurrentMineralBankAmount;
    uint32_t IdleTownHallCount;
    uint32_t IdleBarracksCount;
    uint32_t IdleFactoryCount;
    uint32_t IdleStarportCount;
    uint32_t RecentIdleProductionConflictCount;
    uint32_t RecentSchedulerOrderDeferralCount;
    uint32_t RecentNoProducerDeferralCount;
    uint32_t RecentProducerBusyDeferralCount;
    uint32_t RecentInsufficientResourcesDeferralCount;
    uint32_t RecentPlacementDeferralCount;
};

}  // namespace sc2
