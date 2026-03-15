#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "common/planning/FBlockedTaskRecord.h"
#include "common/planning/FSchedulerStimulusState.h"

namespace sc2
{

class FBlockedTaskRingBuffer
{
public:
    FBlockedTaskRingBuffer();

    void Reset(size_t CapacityValue);
    size_t GetCapacity() const;
    size_t GetCount() const;
    bool IsFull() const;
    bool HasEquivalentRecord(const FBlockedTaskRecord& BlockedTaskRecordValue) const;
    bool HasEquivalentOrderSignature(const FCommandOrderRecord& CommandOrderRecordValue) const;
    const FBlockedTaskRecord* GetRecordAtOrderedIndex(size_t OrderedIndexValue) const;
    bool TryPushOrCoalesce(const FBlockedTaskRecord& BlockedTaskRecordValue, bool& bOutCoalescedValue,
                           bool& bOutDroppedValue, bool& bOutRejectedMustRunValue);
    void CollectReactivatableRecords(const uint64_t CurrentGameLoopValue,
                                     const FSchedulerStimulusState& SchedulerStimulusStateValue,
                                     std::vector<FBlockedTaskRecord>& OutBlockedTaskRecordsValue);
    uint32_t CountRecordsByDeferralReason(ECommandOrderDeferralReason DeferralReasonValue) const;

private:
    size_t GetStorageIndex(size_t OrderedIndexValue) const;
    bool DoesWakeConditionMatch(const FBlockedTaskRecord& BlockedTaskRecordValue, uint64_t CurrentGameLoopValue,
                                const FSchedulerStimulusState& SchedulerStimulusStateValue) const;
    void RemoveOrderedIndex(size_t OrderedIndexValue);

private:
    std::vector<FBlockedTaskRecord> BlockedTaskRecords;
    size_t HeadIndex;
    size_t ActiveCount;
};

}  // namespace sc2
