#include "common/planning/FCommandTaskDescriptor.h"

namespace sc2
{

FCommandTaskDescriptor::FCommandTaskDescriptor()
{
    Reset();
}

void FCommandTaskDescriptor::Reset()
{
    TaskId = 0U;
    PackageKind = ECommandTaskPackageKind::Unknown;
    NeedKind = ECommandTaskNeedKind::Unknown;
    ActionKind = ECommandTaskActionKind::Unknown;
    CompletionKind = ECommandTaskCompletionKind::Unknown;
    TaskType = ECommandTaskType::Unknown;
    Origin = ECommandTaskOrigin::GoalMacro;
    RetentionPolicy = ECommandTaskRetentionPolicy::BufferedRetry;
    BlockedTaskWakeKind = EBlockedTaskWakeKind::CooldownOnly;
    BasePriorityValue = 0;
    SourceGoalId = 0U;
    TriggerMinGameLoop = 0U;
    TriggerMinSupplyUsed = 0U;
    TriggerMaxSupplyUsed = 0U;
    TriggerRetryCadenceGameLoops = 0U;
    TriggerReferenceClockTime.clear();
    TriggerRequiredCompletedTaskIds.clear();
    ActionAbilityId = ABILITY_ID::INVALID;
    ActionProducerUnitTypeId = UNIT_TYPEID::INVALID;
    ActionResultUnitTypeId = UNIT_TYPEID::INVALID;
    ActionUpgradeId = UpgradeID(UPGRADE_ID::INVALID);
    ActionTargetCount = 0U;
    ActionRequestedQueueCount = 1U;
    ParallelGroupId = 0U;
    ActionPreferredPlacementSlotType = EBuildPlacementSlotType::Unknown;
    ActionPreferredPlacementSlotId.Reset();
    CompletionObservedCountAtLeast = 0U;
    CompletionExpectedCompleteByGameLoop = 0U;
}

}  // namespace sc2
