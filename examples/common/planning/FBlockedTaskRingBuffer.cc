#include "common/planning/FBlockedTaskRingBuffer.h"

namespace sc2
{

FBlockedTaskRingBuffer::FBlockedTaskRingBuffer()
    : HeadIndex(0U),
      ActiveCount(0U)
{
}

void FBlockedTaskRingBuffer::Reset(const size_t CapacityValue)
{
    BlockedTaskRecords.clear();
    BlockedTaskRecords.resize(CapacityValue);
    HeadIndex = 0U;
    ActiveCount = 0U;
}

size_t FBlockedTaskRingBuffer::GetCapacity() const
{
    return BlockedTaskRecords.size();
}

size_t FBlockedTaskRingBuffer::GetCount() const
{
    return ActiveCount;
}

bool FBlockedTaskRingBuffer::IsFull() const
{
    return ActiveCount >= BlockedTaskRecords.size() && !BlockedTaskRecords.empty();
}

bool FBlockedTaskRingBuffer::HasEquivalentRecord(const FBlockedTaskRecord& BlockedTaskRecordValue) const
{
    for (size_t OrderedIndexValue = 0U; OrderedIndexValue < ActiveCount; ++OrderedIndexValue)
    {
        if (BlockedTaskRecords[GetStorageIndex(OrderedIndexValue)].MatchesSignature(BlockedTaskRecordValue))
        {
            return true;
        }
    }

    return false;
}

bool FBlockedTaskRingBuffer::HasEquivalentOrderSignature(const FCommandOrderRecord& CommandOrderRecordValue) const
{
    for (size_t OrderedIndexValue = 0U; OrderedIndexValue < ActiveCount; ++OrderedIndexValue)
    {
        if (BlockedTaskRecords[GetStorageIndex(OrderedIndexValue)].MatchesOrderSignature(CommandOrderRecordValue))
        {
            return true;
        }
    }

    return false;
}

const FBlockedTaskRecord* FBlockedTaskRingBuffer::GetRecordAtOrderedIndex(const size_t OrderedIndexValue) const
{
    if (OrderedIndexValue >= ActiveCount)
    {
        return nullptr;
    }

    return &BlockedTaskRecords[GetStorageIndex(OrderedIndexValue)];
}

bool FBlockedTaskRingBuffer::TryPushOrCoalesce(const FBlockedTaskRecord& BlockedTaskRecordValue,
                                               bool& bOutCoalescedValue, bool& bOutDroppedValue,
                                               bool& bOutRejectedMustRunValue)
{
    bOutCoalescedValue = false;
    bOutDroppedValue = false;
    bOutRejectedMustRunValue = false;

    if (BlockedTaskRecords.empty())
    {
        bOutRejectedMustRunValue =
            BlockedTaskRecordValue.RetentionPolicy == ECommandTaskRetentionPolicy::HotMustRun;
        bOutDroppedValue = !bOutRejectedMustRunValue;
        return false;
    }

    for (size_t OrderedIndexValue = 0U; OrderedIndexValue < ActiveCount; ++OrderedIndexValue)
    {
        const size_t StorageIndexValue = GetStorageIndex(OrderedIndexValue);
        if (!BlockedTaskRecords[StorageIndexValue].MatchesSignature(BlockedTaskRecordValue))
        {
            continue;
        }

        BlockedTaskRecords[StorageIndexValue] = BlockedTaskRecordValue;
        bOutCoalescedValue = true;
        return true;
    }

    if (ActiveCount < BlockedTaskRecords.size())
    {
        BlockedTaskRecords[GetStorageIndex(ActiveCount)] = BlockedTaskRecordValue;
        ++ActiveCount;
        return true;
    }

    for (size_t OrderedIndexValue = 0U; OrderedIndexValue < ActiveCount; ++OrderedIndexValue)
    {
        const size_t StorageIndexValue = GetStorageIndex(OrderedIndexValue);
        if (BlockedTaskRecords[StorageIndexValue].RetentionPolicy !=
            ECommandTaskRetentionPolicy::DiscardableDuplicate)
        {
            continue;
        }

        RemoveOrderedIndex(OrderedIndexValue);
        BlockedTaskRecords[GetStorageIndex(ActiveCount)] = BlockedTaskRecordValue;
        ++ActiveCount;
        return true;
    }

    if (BlockedTaskRecordValue.RetentionPolicy == ECommandTaskRetentionPolicy::HotMustRun)
    {
        bOutRejectedMustRunValue = true;
        return false;
    }

    bOutDroppedValue = true;
    return false;
}

void FBlockedTaskRingBuffer::CollectReactivatableRecords(
    const uint64_t CurrentGameLoopValue, const FSchedulerStimulusState& SchedulerStimulusStateValue,
    std::vector<FBlockedTaskRecord>& OutBlockedTaskRecordsValue)
{
    size_t OrderedIndexValue = 0U;
    while (OrderedIndexValue < ActiveCount)
    {
        const size_t StorageIndexValue = GetStorageIndex(OrderedIndexValue);
        const FBlockedTaskRecord& BlockedTaskRecordValue = BlockedTaskRecords[StorageIndexValue];
        if (!DoesWakeConditionMatch(BlockedTaskRecordValue, CurrentGameLoopValue, SchedulerStimulusStateValue))
        {
            ++OrderedIndexValue;
            continue;
        }

        OutBlockedTaskRecordsValue.push_back(BlockedTaskRecordValue);
        RemoveOrderedIndex(OrderedIndexValue);
    }
}

uint32_t FBlockedTaskRingBuffer::CountRecordsByDeferralReason(
    const ECommandOrderDeferralReason DeferralReasonValue) const
{
    uint32_t MatchingRecordCountValue = 0U;
    for (size_t OrderedIndexValue = 0U; OrderedIndexValue < ActiveCount; ++OrderedIndexValue)
    {
        if (BlockedTaskRecords[GetStorageIndex(OrderedIndexValue)].BlockingReason == DeferralReasonValue)
        {
            ++MatchingRecordCountValue;
        }
    }

    return MatchingRecordCountValue;
}

size_t FBlockedTaskRingBuffer::GetStorageIndex(const size_t OrderedIndexValue) const
{
    if (BlockedTaskRecords.empty())
    {
        return 0U;
    }

    return (HeadIndex + OrderedIndexValue) % BlockedTaskRecords.size();
}

bool FBlockedTaskRingBuffer::DoesWakeConditionMatch(
    const FBlockedTaskRecord& BlockedTaskRecordValue, const uint64_t CurrentGameLoopValue,
    const FSchedulerStimulusState& SchedulerStimulusStateValue) const
{
    if (CurrentGameLoopValue >= BlockedTaskRecordValue.NextEligibleGameLoop)
    {
        return true;
    }

    uint64_t CurrentRevisionValue = 0U;
    switch (BlockedTaskRecordValue.WakeKind)
    {
        case EBlockedTaskWakeKind::ProducerAvailability:
            CurrentRevisionValue = SchedulerStimulusStateValue.ProducerRevision;
            break;
        case EBlockedTaskWakeKind::Resources:
            CurrentRevisionValue = SchedulerStimulusStateValue.ResourceRevision;
            break;
        case EBlockedTaskWakeKind::Placement:
            CurrentRevisionValue = SchedulerStimulusStateValue.PlacementRevision;
            break;
        case EBlockedTaskWakeKind::GoalRevision:
            CurrentRevisionValue = SchedulerStimulusStateValue.GoalRevision;
            break;
        case EBlockedTaskWakeKind::CooldownOnly:
        default:
            CurrentRevisionValue = BlockedTaskRecordValue.LastSeenStimulusRevision;
            break;
    }

    return CurrentRevisionValue != BlockedTaskRecordValue.LastSeenStimulusRevision;
}

void FBlockedTaskRingBuffer::RemoveOrderedIndex(const size_t OrderedIndexValue)
{
    if (OrderedIndexValue >= ActiveCount || BlockedTaskRecords.empty())
    {
        return;
    }

    for (size_t ShiftIndexValue = OrderedIndexValue; ShiftIndexValue + 1U < ActiveCount; ++ShiftIndexValue)
    {
        const size_t DestinationStorageIndexValue = GetStorageIndex(ShiftIndexValue);
        const size_t SourceStorageIndexValue = GetStorageIndex(ShiftIndexValue + 1U);
        BlockedTaskRecords[DestinationStorageIndexValue] = BlockedTaskRecords[SourceStorageIndexValue];
    }

    BlockedTaskRecords[GetStorageIndex(ActiveCount - 1U)].Reset();
    --ActiveCount;
}

}  // namespace sc2
