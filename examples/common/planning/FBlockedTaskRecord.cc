#include "common/planning/FBlockedTaskRecord.h"

#include "common/planning/FCommandOrderRecord.h"

namespace sc2
{
namespace
{

bool MatchesTaskSignatureValues(const uint32_t LeftTaskIdValue, const uint32_t LeftSourceGoalIdValue,
                                const AbilityID LeftAbilityIdValue, const UNIT_TYPEID LeftResultUnitTypeIdValue,
                                const UpgradeID LeftUpgradeIdValue,
                                const FBuildPlacementSlotId& LeftPreferredPlacementSlotIdValue,
                                const FBuildPlacementSlotId& LeftPreferredProducerPlacementSlotIdValue,
                                const uint32_t RightTaskIdValue, const uint32_t RightSourceGoalIdValue,
                                const AbilityID RightAbilityIdValue, const UNIT_TYPEID RightResultUnitTypeIdValue,
                                const UpgradeID RightUpgradeIdValue,
                                const FBuildPlacementSlotId& RightPreferredPlacementSlotIdValue,
                                const FBuildPlacementSlotId& RightPreferredProducerPlacementSlotIdValue)
{
    if (LeftTaskIdValue != 0U || RightTaskIdValue != 0U)
    {
        return LeftTaskIdValue == RightTaskIdValue;
    }

    return LeftSourceGoalIdValue == RightSourceGoalIdValue && LeftAbilityIdValue == RightAbilityIdValue &&
           LeftResultUnitTypeIdValue == RightResultUnitTypeIdValue && LeftUpgradeIdValue == RightUpgradeIdValue &&
           LeftPreferredPlacementSlotIdValue == RightPreferredPlacementSlotIdValue &&
           LeftPreferredProducerPlacementSlotIdValue == RightPreferredProducerPlacementSlotIdValue;
}

}  // namespace

FBlockedTaskRecord::FBlockedTaskRecord()
{
    Reset();
}

void FBlockedTaskRecord::Reset()
{
    TaskId = 0U;
    SourceGoalId = 0U;
    SourceLayer = ECommandAuthorityLayer::StrategicDirector;
    PackageKind = ECommandTaskPackageKind::Unknown;
    NeedKind = ECommandTaskNeedKind::Unknown;
    TaskType = ECommandTaskType::Unknown;
    Origin = ECommandTaskOrigin::GoalMacro;
    CommitmentClass = ECommandCommitmentClass::FlexibleMacro;
    ExecutionGuarantee = ECommandTaskExecutionGuarantee::Preferred;
    RetentionPolicy = ECommandTaskRetentionPolicy::BufferedRetry;
    BasePriorityValue = 0;
    AbilityId = ABILITY_ID::INVALID;
    ProducerUnitTypeId = UNIT_TYPEID::INVALID;
    ResultUnitTypeId = UNIT_TYPEID::INVALID;
    UpgradeId = UpgradeID(UPGRADE_ID::INVALID);
    PreferredPlacementSlotId.Reset();
    PreferredProducerPlacementSlotId.Reset();
    BlockingReason = ECommandOrderDeferralReason::None;
    WakeKind = EBlockedTaskWakeKind::CooldownOnly;
    NextEligibleGameLoop = 0U;
    LastSeenStimulusRevision = 0U;
    RetryCount = 0U;
    TargetCount = 0U;
    RequestedQueueCount = 1U;
}

bool FBlockedTaskRecord::MatchesSignature(const FBlockedTaskRecord& BlockedTaskRecordValue) const
{
    return MatchesTaskSignatureValues(TaskId, SourceGoalId, AbilityId, ResultUnitTypeId, UpgradeId,
                                      PreferredPlacementSlotId, PreferredProducerPlacementSlotId,
                                      BlockedTaskRecordValue.TaskId,
                                      BlockedTaskRecordValue.SourceGoalId, BlockedTaskRecordValue.AbilityId,
                                      BlockedTaskRecordValue.ResultUnitTypeId, BlockedTaskRecordValue.UpgradeId,
                                      BlockedTaskRecordValue.PreferredPlacementSlotId,
                                      BlockedTaskRecordValue.PreferredProducerPlacementSlotId);
}

bool FBlockedTaskRecord::MatchesOrderSignature(const FCommandOrderRecord& CommandOrderRecordValue) const
{
    return MatchesTaskSignatureValues(TaskId, SourceGoalId, AbilityId, ResultUnitTypeId, UpgradeId,
                                      PreferredPlacementSlotId, PreferredProducerPlacementSlotId,
                                      CommandOrderRecordValue.PlanStepId,
                                      CommandOrderRecordValue.SourceGoalId, CommandOrderRecordValue.AbilityId,
                                      CommandOrderRecordValue.ResultUnitTypeId, CommandOrderRecordValue.UpgradeId,
                                      CommandOrderRecordValue.PreferredPlacementSlotId,
                                      CommandOrderRecordValue.PreferredProducerPlacementSlotId);
}

bool FBlockedTaskRecord::MatchesTaskSignature(const FCommandTaskDescriptor& CommandTaskDescriptorValue) const
{
    return MatchesTaskSignatureValues(TaskId, SourceGoalId, AbilityId, ResultUnitTypeId, UpgradeId,
                                      PreferredPlacementSlotId, PreferredProducerPlacementSlotId,
                                      CommandTaskDescriptorValue.TaskId,
                                      CommandTaskDescriptorValue.SourceGoalId, CommandTaskDescriptorValue.ActionAbilityId,
                                      CommandTaskDescriptorValue.ActionResultUnitTypeId,
                                      CommandTaskDescriptorValue.ActionUpgradeId,
                                      CommandTaskDescriptorValue.ActionPreferredPlacementSlotId,
                                      CommandTaskDescriptorValue.ActionPreferredProducerPlacementSlotId);
}

}  // namespace sc2
