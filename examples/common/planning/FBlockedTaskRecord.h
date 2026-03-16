#pragma once

#include <cstdint>

#include "common/planning/EBlockedTaskWakeKind.h"
#include "common/planning/ECommandAuthorityLayer.h"
#include "common/planning/ECommandCommitmentClass.h"
#include "common/planning/ECommandOrderDeferralReason.h"
#include "common/planning/ECommandTaskOrigin.h"
#include "common/planning/ECommandTaskExecutionGuarantee.h"
#include "common/planning/ECommandTaskRetentionPolicy.h"
#include "common/planning/ECommandTaskType.h"
#include "common/planning/FCommandTaskDescriptor.h"
#include "common/services/FBuildPlacementSlotId.h"
#include "sc2api/sc2_typeenums.h"

namespace sc2
{

struct FCommandOrderRecord;

struct FBlockedTaskRecord
{
public:
    FBlockedTaskRecord();

    void Reset();
    bool MatchesSignature(const FBlockedTaskRecord& BlockedTaskRecordValue) const;
    bool MatchesOrderSignature(const FCommandOrderRecord& CommandOrderRecordValue) const;
    bool MatchesTaskSignature(const FCommandTaskDescriptor& CommandTaskDescriptorValue) const;

public:
    uint32_t TaskId;
    uint32_t SourceGoalId;
    ECommandAuthorityLayer SourceLayer;
    ECommandTaskPackageKind PackageKind;
    ECommandTaskNeedKind NeedKind;
    ECommandTaskType TaskType;
    ECommandTaskOrigin Origin;
    ECommandCommitmentClass CommitmentClass;
    ECommandTaskExecutionGuarantee ExecutionGuarantee;
    ECommandTaskRetentionPolicy RetentionPolicy;
    int BasePriorityValue;
    AbilityID AbilityId;
    UNIT_TYPEID ProducerUnitTypeId;
    UNIT_TYPEID ResultUnitTypeId;
    UpgradeID UpgradeId;
    FBuildPlacementSlotId PreferredPlacementSlotId;
    FBuildPlacementSlotId PreferredProducerPlacementSlotId;
    ECommandOrderDeferralReason BlockingReason;
    EBlockedTaskWakeKind WakeKind;
    uint64_t NextEligibleGameLoop;
    uint64_t LastSeenStimulusRevision;
    uint32_t RetryCount;
    uint32_t TargetCount;
    uint32_t RequestedQueueCount;
};

}  // namespace sc2
