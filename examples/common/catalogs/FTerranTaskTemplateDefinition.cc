#include "common/catalogs/FTerranTaskTemplateDefinition.h"

namespace sc2
{

FTerranTaskTemplateDefinition::FTerranTaskTemplateDefinition()
{
    Reset();
}

FTerranTaskTemplateDefinition::FTerranTaskTemplateDefinition(
    const ETerranTaskTemplateId TemplateIdValue, const char* DisplayNameValue,
    const ECommandTaskPackageKind PackageKindValue, const ECommandTaskNeedKind NeedKindValue,
    const ECommandTaskActionKind ActionKindValue, const ECommandTaskCompletionKind CompletionKindValue,
    const ECommandTaskType TaskTypeValue, const ECommandTaskOrigin OriginValue,
    const ECommandCommitmentClass CommitmentClassValue,
    const ECommandTaskExecutionGuarantee ExecutionGuaranteeValue,
    const ECommandTaskRetentionPolicy RetentionPolicyValue,
    const EBlockedTaskWakeKind BlockedTaskWakeKindValue, const AbilityID ActionAbilityIdValue,
    const UNIT_TYPEID ActionProducerUnitTypeIdValue, const UNIT_TYPEID ActionResultUnitTypeIdValue,
    const UpgradeID ActionUpgradeIdValue, const uint32_t DefaultTargetCountValue,
    const uint32_t DefaultRequestedQueueCountValue,
    const EBuildPlacementSlotType DefaultPreferredPlacementSlotTypeValue)
{
    TemplateId = TemplateIdValue;
    DisplayName = DisplayNameValue;
    PackageKind = PackageKindValue;
    NeedKind = NeedKindValue;
    ActionKind = ActionKindValue;
    CompletionKind = CompletionKindValue;
    TaskType = TaskTypeValue;
    Origin = OriginValue;
    CommitmentClass = CommitmentClassValue;
    ExecutionGuarantee = ExecutionGuaranteeValue;
    RetentionPolicy = RetentionPolicyValue;
    BlockedTaskWakeKind = BlockedTaskWakeKindValue;
    ActionAbilityId = ActionAbilityIdValue;
    ActionProducerUnitTypeId = ActionProducerUnitTypeIdValue;
    ActionResultUnitTypeId = ActionResultUnitTypeIdValue;
    ActionUpgradeId = ActionUpgradeIdValue;
    DefaultTargetCount = DefaultTargetCountValue;
    DefaultRequestedQueueCount = DefaultRequestedQueueCountValue;
    DefaultPreferredPlacementSlotType = DefaultPreferredPlacementSlotTypeValue;
}

void FTerranTaskTemplateDefinition::Reset()
{
    TemplateId = ETerranTaskTemplateId::Invalid;
    DisplayName = "";
    PackageKind = ECommandTaskPackageKind::Unknown;
    NeedKind = ECommandTaskNeedKind::Unknown;
    ActionKind = ECommandTaskActionKind::Unknown;
    CompletionKind = ECommandTaskCompletionKind::CountAtLeast;
    TaskType = ECommandTaskType::Unknown;
    Origin = ECommandTaskOrigin::GoalMacro;
    CommitmentClass = ECommandCommitmentClass::FlexibleMacro;
    ExecutionGuarantee = ECommandTaskExecutionGuarantee::Preferred;
    RetentionPolicy = ECommandTaskRetentionPolicy::BufferedRetry;
    BlockedTaskWakeKind = EBlockedTaskWakeKind::Resources;
    ActionAbilityId = ABILITY_ID::INVALID;
    ActionProducerUnitTypeId = UNIT_TYPEID::INVALID;
    ActionResultUnitTypeId = UNIT_TYPEID::INVALID;
    ActionUpgradeId = UpgradeID(UPGRADE_ID::INVALID);
    DefaultTargetCount = 0U;
    DefaultRequestedQueueCount = 1U;
    DefaultPreferredPlacementSlotType = EBuildPlacementSlotType::Unknown;
}

}  // namespace sc2
