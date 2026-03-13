#include "common/telemetry/FAgentExecutionTelemetry.h"

namespace sc2
{
namespace
{

constexpr uint64_t EventCooldownStepsValue = 120U;
constexpr size_t MaxRecentEventCountValue = 16U;

void PopulateCommonEventFields(FExecutionEventRecord& ExecutionEventRecordValue,
                               const EAgentExecutionEventType AgentExecutionEventTypeValue,
                               const uint64_t CurrentStepValue, const uint64_t CurrentGameLoopValue)
{
    ExecutionEventRecordValue.EventType = AgentExecutionEventTypeValue;
    ExecutionEventRecordValue.Step = CurrentStepValue;
    ExecutionEventRecordValue.GameLoop = CurrentGameLoopValue;
}

void PopulateWallEventFields(FExecutionEventRecord& ExecutionEventRecordValue,
                             const EAgentExecutionEventType AgentExecutionEventTypeValue,
                             const uint64_t CurrentStepValue, const uint64_t CurrentGameLoopValue)
{
    PopulateCommonEventFields(ExecutionEventRecordValue, AgentExecutionEventTypeValue, CurrentStepValue,
                              CurrentGameLoopValue);
    ExecutionEventRecordValue.IntentDomain = EIntentDomain::StructureControl;
}

}  // namespace

FAgentExecutionTelemetry::FAgentExecutionTelemetry()
{
    Reset();
}

void FAgentExecutionTelemetry::Reset()
{
    SupplyBlockState = EExecutionConditionState::Inactive;
    SupplyBlockStartGameLoop = 0U;
    LastSupplyBlockDurationGameLoops = 0U;
    MineralBankState = EExecutionConditionState::Inactive;
    MineralBankStartGameLoop = 0U;
    LastMineralBankDurationGameLoops = 0U;
    LastMineralBankAmount = 0U;
    TotalActorIntentConflictCount = 0U;
    TotalIdleProductionConflictCount = 0U;
    TotalSchedulerOrderDeferralCount = 0U;
    RecentEvents.clear();
    LastActorConflictStepByActor.clear();
    LastIdleProductionConflictStepByActor.clear();
    LastSchedulerDeferralStepByOrderId.clear();
    LastSchedulerDeferralReasonByOrderId.clear();
}

void FAgentExecutionTelemetry::UpdateSupplyBlockState(const EExecutionConditionState NextStateValue,
                                                      const uint64_t CurrentStepValue,
                                                      const uint64_t CurrentGameLoopValue)
{
    if (SupplyBlockState == NextStateValue)
    {
        return;
    }

    if (NextStateValue == EExecutionConditionState::Active)
    {
        SupplyBlockState = EExecutionConditionState::Active;
        SupplyBlockStartGameLoop = CurrentGameLoopValue;

        FExecutionEventRecord ExecutionEventRecordValue;
        PopulateCommonEventFields(ExecutionEventRecordValue, EAgentExecutionEventType::SupplyBlockedStarted,
                                  CurrentStepValue, CurrentGameLoopValue);
        AppendEvent(ExecutionEventRecordValue);
        return;
    }

    LastSupplyBlockDurationGameLoops =
        CurrentGameLoopValue >= SupplyBlockStartGameLoop ? (CurrentGameLoopValue - SupplyBlockStartGameLoop) : 0U;
    SupplyBlockState = EExecutionConditionState::Inactive;

    FExecutionEventRecord ExecutionEventRecordValue;
    PopulateCommonEventFields(ExecutionEventRecordValue, EAgentExecutionEventType::SupplyBlockedEnded,
                              CurrentStepValue, CurrentGameLoopValue);
    ExecutionEventRecordValue.MetricValue = LastSupplyBlockDurationGameLoops;
    AppendEvent(ExecutionEventRecordValue);
}

void FAgentExecutionTelemetry::UpdateMineralBankState(const EExecutionConditionState NextStateValue,
                                                      const uint64_t CurrentStepValue,
                                                      const uint64_t CurrentGameLoopValue,
                                                      const uint32_t MineralCountValue)
{
    LastMineralBankAmount = MineralCountValue;
    if (MineralBankState == NextStateValue)
    {
        return;
    }

    if (NextStateValue == EExecutionConditionState::Active)
    {
        MineralBankState = EExecutionConditionState::Active;
        MineralBankStartGameLoop = CurrentGameLoopValue;

        FExecutionEventRecord ExecutionEventRecordValue;
        PopulateCommonEventFields(ExecutionEventRecordValue, EAgentExecutionEventType::MineralBankStarted,
                                  CurrentStepValue, CurrentGameLoopValue);
        ExecutionEventRecordValue.MetricValue = MineralCountValue;
        AppendEvent(ExecutionEventRecordValue);
        return;
    }

    LastMineralBankDurationGameLoops =
        CurrentGameLoopValue >= MineralBankStartGameLoop ? (CurrentGameLoopValue - MineralBankStartGameLoop) : 0U;
    MineralBankState = EExecutionConditionState::Inactive;

    FExecutionEventRecord ExecutionEventRecordValue;
    PopulateCommonEventFields(ExecutionEventRecordValue, EAgentExecutionEventType::MineralBankEnded,
                              CurrentStepValue, CurrentGameLoopValue);
    ExecutionEventRecordValue.MetricValue = LastMineralBankDurationGameLoops;
    AppendEvent(ExecutionEventRecordValue);
}

void FAgentExecutionTelemetry::RecordActorIntentConflict(const uint64_t CurrentStepValue,
                                                         const uint64_t CurrentGameLoopValue,
                                                         const Tag ActorTagValue, const AbilityID AbilityIdValue,
                                                         const EIntentDomain IntentDomainValue)
{
    const std::unordered_map<Tag, uint64_t>::const_iterator FoundStepValue =
        LastActorConflictStepByActor.find(ActorTagValue);
    if (FoundStepValue != LastActorConflictStepByActor.end() &&
        CurrentStepValue < (FoundStepValue->second + EventCooldownStepsValue))
    {
        return;
    }

    LastActorConflictStepByActor[ActorTagValue] = CurrentStepValue;
    ++TotalActorIntentConflictCount;

    FExecutionEventRecord ExecutionEventRecordValue;
    PopulateCommonEventFields(ExecutionEventRecordValue, EAgentExecutionEventType::ActorIntentConflict,
                              CurrentStepValue, CurrentGameLoopValue);
    ExecutionEventRecordValue.ActorTag = ActorTagValue;
    ExecutionEventRecordValue.AbilityId = AbilityIdValue;
    ExecutionEventRecordValue.IntentDomain = IntentDomainValue;
    AppendEvent(ExecutionEventRecordValue);
}

void FAgentExecutionTelemetry::RecordIdleProductionConflict(const uint64_t CurrentStepValue,
                                                            const uint64_t CurrentGameLoopValue,
                                                            const Tag ActorTagValue,
                                                            const UNIT_TYPEID UnitTypeIdValue,
                                                            const AbilityID AbilityIdValue)
{
    const std::unordered_map<Tag, uint64_t>::const_iterator FoundStepValue =
        LastIdleProductionConflictStepByActor.find(ActorTagValue);
    if (FoundStepValue != LastIdleProductionConflictStepByActor.end() &&
        CurrentStepValue < (FoundStepValue->second + EventCooldownStepsValue))
    {
        return;
    }

    LastIdleProductionConflictStepByActor[ActorTagValue] = CurrentStepValue;
    ++TotalIdleProductionConflictCount;

    FExecutionEventRecord ExecutionEventRecordValue;
    PopulateCommonEventFields(ExecutionEventRecordValue, EAgentExecutionEventType::IdleProductionStructure,
                              CurrentStepValue, CurrentGameLoopValue);
    ExecutionEventRecordValue.ActorTag = ActorTagValue;
    ExecutionEventRecordValue.UnitTypeId = UnitTypeIdValue;
    ExecutionEventRecordValue.AbilityId = AbilityIdValue;
    AppendEvent(ExecutionEventRecordValue);
}

void FAgentExecutionTelemetry::RecordSchedulerOrderDeferred(
    const uint64_t CurrentStepValue, const uint64_t CurrentGameLoopValue, const uint32_t OrderIdValue,
    const uint32_t PlanStepIdValue, const Tag ActorTagValue, const AbilityID AbilityIdValue,
    const EIntentDomain IntentDomainValue, const ECommandOrderDeferralReason DeferralReasonValue)
{
    const std::unordered_map<uint32_t, uint64_t>::const_iterator FoundStepValue =
        LastSchedulerDeferralStepByOrderId.find(OrderIdValue);
    const std::unordered_map<uint32_t, ECommandOrderDeferralReason>::const_iterator FoundReasonValue =
        LastSchedulerDeferralReasonByOrderId.find(OrderIdValue);
    if (FoundStepValue != LastSchedulerDeferralStepByOrderId.end() &&
        FoundReasonValue != LastSchedulerDeferralReasonByOrderId.end() &&
        FoundReasonValue->second == DeferralReasonValue &&
        CurrentStepValue < (FoundStepValue->second + EventCooldownStepsValue))
    {
        return;
    }

    LastSchedulerDeferralStepByOrderId[OrderIdValue] = CurrentStepValue;
    LastSchedulerDeferralReasonByOrderId[OrderIdValue] = DeferralReasonValue;
    ++TotalSchedulerOrderDeferralCount;

    FExecutionEventRecord ExecutionEventRecordValue;
    PopulateCommonEventFields(ExecutionEventRecordValue, EAgentExecutionEventType::SchedulerOrderDeferred,
                              CurrentStepValue, CurrentGameLoopValue);
    ExecutionEventRecordValue.OrderId = OrderIdValue;
    ExecutionEventRecordValue.PlanStepId = PlanStepIdValue;
    ExecutionEventRecordValue.ActorTag = ActorTagValue;
    ExecutionEventRecordValue.AbilityId = AbilityIdValue;
    ExecutionEventRecordValue.IntentDomain = IntentDomainValue;
    ExecutionEventRecordValue.DeferralReason = DeferralReasonValue;
    AppendEvent(ExecutionEventRecordValue);
}

void FAgentExecutionTelemetry::RecordWallDescriptorInvalid(const uint64_t CurrentStepValue,
                                                           const uint64_t CurrentGameLoopValue)
{
    FExecutionEventRecord ExecutionEventRecordValue;
    PopulateWallEventFields(ExecutionEventRecordValue, EAgentExecutionEventType::WallDescriptorInvalid,
                            CurrentStepValue, CurrentGameLoopValue);
    AppendEvent(ExecutionEventRecordValue);
}

void FAgentExecutionTelemetry::RecordWallThreatDetected(const uint64_t CurrentStepValue,
                                                        const uint64_t CurrentGameLoopValue)
{
    FExecutionEventRecord ExecutionEventRecordValue;
    PopulateWallEventFields(ExecutionEventRecordValue, EAgentExecutionEventType::WallThreatDetected,
                            CurrentStepValue, CurrentGameLoopValue);
    AppendEvent(ExecutionEventRecordValue);
}

void FAgentExecutionTelemetry::RecordWallOpened(const uint64_t CurrentStepValue,
                                                const uint64_t CurrentGameLoopValue)
{
    FExecutionEventRecord ExecutionEventRecordValue;
    PopulateWallEventFields(ExecutionEventRecordValue, EAgentExecutionEventType::WallOpened, CurrentStepValue,
                            CurrentGameLoopValue);
    AppendEvent(ExecutionEventRecordValue);
}

void FAgentExecutionTelemetry::RecordWallClosed(const uint64_t CurrentStepValue,
                                                const uint64_t CurrentGameLoopValue)
{
    FExecutionEventRecord ExecutionEventRecordValue;
    PopulateWallEventFields(ExecutionEventRecordValue, EAgentExecutionEventType::WallClosed, CurrentStepValue,
                            CurrentGameLoopValue);
    AppendEvent(ExecutionEventRecordValue);
}

uint64_t FAgentExecutionTelemetry::GetCurrentSupplyBlockDurationGameLoops(const uint64_t CurrentGameLoopValue) const
{
    if (SupplyBlockState != EExecutionConditionState::Active || CurrentGameLoopValue < SupplyBlockStartGameLoop)
    {
        return 0U;
    }

    return CurrentGameLoopValue - SupplyBlockStartGameLoop;
}

uint64_t FAgentExecutionTelemetry::GetCurrentMineralBankDurationGameLoops(const uint64_t CurrentGameLoopValue) const
{
    if (MineralBankState != EExecutionConditionState::Active || CurrentGameLoopValue < MineralBankStartGameLoop)
    {
        return 0U;
    }

    return CurrentGameLoopValue - MineralBankStartGameLoop;
}

void FAgentExecutionTelemetry::AppendEvent(const FExecutionEventRecord& ExecutionEventRecordValue)
{
    RecentEvents.push_back(ExecutionEventRecordValue);
    if (RecentEvents.size() > MaxRecentEventCountValue)
    {
        RecentEvents.erase(RecentEvents.begin());
    }
}

}  // namespace sc2
