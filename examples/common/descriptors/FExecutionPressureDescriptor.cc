#include "common/descriptors/FExecutionPressureDescriptor.h"

namespace sc2
{

FExecutionPressureDescriptor::FExecutionPressureDescriptor()
{
    Reset();
}

void FExecutionPressureDescriptor::Reset()
{
    SupplyBlockState = EExecutionConditionState::Inactive;
    SupplyBlockDurationGameLoops = 0U;
    MineralBankState = EExecutionConditionState::Inactive;
    MineralBankDurationGameLoops = 0U;
    CurrentMineralBankAmount = 0U;
    IdleTownHallCount = 0U;
    IdleBarracksCount = 0U;
    IdleFactoryCount = 0U;
    IdleStarportCount = 0U;
    RecentIdleProductionConflictCount = 0U;
    RecentSchedulerOrderDeferralCount = 0U;
    RecentNoProducerDeferralCount = 0U;
    RecentProducerBusyDeferralCount = 0U;
    RecentInsufficientResourcesDeferralCount = 0U;
    RecentPlacementDeferralCount = 0U;
}

uint32_t FExecutionPressureDescriptor::GetIdleCombatProductionStructureCount() const
{
    return IdleBarracksCount + IdleFactoryCount + IdleStarportCount;
}

}  // namespace sc2
