#pragma once

#include <cstdint>

#include "common/catalogs/ETerranTaskTemplateId.h"
#include "common/planning/EBlockedTaskWakeKind.h"
#include "common/planning/ECommandCommitmentClass.h"
#include "common/planning/ECommandTaskOrigin.h"
#include "common/planning/ECommandTaskExecutionGuarantee.h"
#include "common/planning/ECommandTaskRetentionPolicy.h"
#include "common/planning/ECommandTaskType.h"
#include "common/planning/FCommandTaskDescriptor.h"
#include "common/services/EBuildPlacementSlotType.h"
#include "sc2api/sc2_typeenums.h"

namespace sc2
{

struct FTerranTaskTemplateDefinition
{
public:
    FTerranTaskTemplateDefinition();
    FTerranTaskTemplateDefinition(ETerranTaskTemplateId TemplateIdValue, const char* DisplayNameValue,
                                  ECommandTaskPackageKind PackageKindValue,
                                  ECommandTaskNeedKind NeedKindValue, ECommandTaskActionKind ActionKindValue,
                                  ECommandTaskCompletionKind CompletionKindValue,
                                  ECommandTaskType TaskTypeValue, ECommandTaskOrigin OriginValue,
                                  ECommandCommitmentClass CommitmentClassValue,
                                  ECommandTaskExecutionGuarantee ExecutionGuaranteeValue,
                                  ECommandTaskRetentionPolicy RetentionPolicyValue,
                                  EBlockedTaskWakeKind BlockedTaskWakeKindValue, AbilityID ActionAbilityIdValue,
                                  UNIT_TYPEID ActionProducerUnitTypeIdValue,
                                  UNIT_TYPEID ActionResultUnitTypeIdValue, UpgradeID ActionUpgradeIdValue,
                                  uint32_t DefaultTargetCountValue, uint32_t DefaultRequestedQueueCountValue,
                                  EBuildPlacementSlotType DefaultPreferredPlacementSlotTypeValue);

    void Reset();

public:
    ETerranTaskTemplateId TemplateId;
    const char* DisplayName;
    ECommandTaskPackageKind PackageKind;
    ECommandTaskNeedKind NeedKind;
    ECommandTaskActionKind ActionKind;
    ECommandTaskCompletionKind CompletionKind;
    ECommandTaskType TaskType;
    ECommandTaskOrigin Origin;
    ECommandCommitmentClass CommitmentClass;
    ECommandTaskExecutionGuarantee ExecutionGuarantee;
    ECommandTaskRetentionPolicy RetentionPolicy;
    EBlockedTaskWakeKind BlockedTaskWakeKind;
    AbilityID ActionAbilityId;
    UNIT_TYPEID ActionProducerUnitTypeId;
    UNIT_TYPEID ActionResultUnitTypeId;
    UpgradeID ActionUpgradeId;
    uint32_t DefaultTargetCount;
    uint32_t DefaultRequestedQueueCount;
    EBuildPlacementSlotType DefaultPreferredPlacementSlotType;
};

}  // namespace sc2
