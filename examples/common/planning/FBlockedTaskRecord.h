#pragma once

#include <cstdint>

#include "common/planning/EBlockedTaskWakeKind.h"
#include "common/planning/ECommandOrderDeferralReason.h"
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
    ECommandTaskPackageKind PackageKind;
    ECommandTaskNeedKind NeedKind;
    ECommandTaskType TaskType;
    ECommandTaskRetentionPolicy RetentionPolicy;
    AbilityID AbilityId;
    UNIT_TYPEID ResultUnitTypeId;
    UpgradeID UpgradeId;
    FBuildPlacementSlotId PreferredPlacementSlotId;
    ECommandOrderDeferralReason BlockingReason;
    EBlockedTaskWakeKind WakeKind;
    uint64_t NextEligibleGameLoop;
    uint64_t LastSeenStimulusRevision;
    uint32_t RetryCount;
    uint32_t RequestedQueueCount;
};

}  // namespace sc2
