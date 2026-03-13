#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "common/planning/ECommandOrderDeferralReason.h"
#include "common/planning/EIntentDomain.h"
#include "common/telemetry/EExecutionConditionState.h"
#include "common/telemetry/FExecutionEventRecord.h"
#include "sc2api/sc2_common.h"
#include "sc2api/sc2_gametypes.h"
#include "sc2api/sc2_typeenums.h"

namespace sc2
{

struct FAgentExecutionTelemetry
{
public:
    FAgentExecutionTelemetry();

    void Reset();
    void UpdateSupplyBlockState(EExecutionConditionState NextStateValue, uint64_t CurrentStepValue,
                                uint64_t CurrentGameLoopValue);
    void UpdateMineralBankState(EExecutionConditionState NextStateValue, uint64_t CurrentStepValue,
                                uint64_t CurrentGameLoopValue, uint32_t MineralCountValue);
    void RecordActorIntentConflict(uint64_t CurrentStepValue, uint64_t CurrentGameLoopValue, Tag ActorTagValue,
                                   AbilityID AbilityIdValue, EIntentDomain IntentDomainValue);
    void RecordIdleProductionConflict(uint64_t CurrentStepValue, uint64_t CurrentGameLoopValue, Tag ActorTagValue,
                                      UNIT_TYPEID UnitTypeIdValue, AbilityID AbilityIdValue);
    void RecordSchedulerOrderDeferred(uint64_t CurrentStepValue, uint64_t CurrentGameLoopValue, uint32_t OrderIdValue,
                                      uint32_t PlanStepIdValue, Tag ActorTagValue, AbilityID AbilityIdValue,
                                      EIntentDomain IntentDomainValue,
                                      ECommandOrderDeferralReason DeferralReasonValue);
    void RecordWallDescriptorInvalid(uint64_t CurrentStepValue, uint64_t CurrentGameLoopValue);
    void RecordWallThreatDetected(uint64_t CurrentStepValue, uint64_t CurrentGameLoopValue);
    void RecordWallOpened(uint64_t CurrentStepValue, uint64_t CurrentGameLoopValue);
    void RecordWallClosed(uint64_t CurrentStepValue, uint64_t CurrentGameLoopValue);
    uint64_t GetCurrentSupplyBlockDurationGameLoops(uint64_t CurrentGameLoopValue) const;
    uint64_t GetCurrentMineralBankDurationGameLoops(uint64_t CurrentGameLoopValue) const;

public:
    EExecutionConditionState SupplyBlockState;
    uint64_t SupplyBlockStartGameLoop;
    uint64_t LastSupplyBlockDurationGameLoops;
    EExecutionConditionState MineralBankState;
    uint64_t MineralBankStartGameLoop;
    uint64_t LastMineralBankDurationGameLoops;
    uint32_t LastMineralBankAmount;
    uint32_t TotalActorIntentConflictCount;
    uint32_t TotalIdleProductionConflictCount;
    uint32_t TotalSchedulerOrderDeferralCount;
    std::vector<FExecutionEventRecord> RecentEvents;

protected:
    void AppendEvent(const FExecutionEventRecord& ExecutionEventRecordValue);

protected:
    std::unordered_map<Tag, uint64_t> LastActorConflictStepByActor;
    std::unordered_map<Tag, uint64_t> LastIdleProductionConflictStepByActor;
    std::unordered_map<uint32_t, uint64_t> LastSchedulerDeferralStepByOrderId;
    std::unordered_map<uint32_t, ECommandOrderDeferralReason> LastSchedulerDeferralReasonByOrderId;
};

}  // namespace sc2
