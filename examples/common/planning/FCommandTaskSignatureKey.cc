#include "common/planning/FCommandTaskSignatureKey.h"

#include "common/planning/FCommandOrderRecord.h"
#include "common/planning/FCommandTaskDescriptor.h"

namespace sc2
{
namespace
{

size_t CombineHash(const size_t SeedValue, const size_t ValueToMix)
{
    return SeedValue ^ (ValueToMix + 0x9e3779b9U + (SeedValue << 6U) + (SeedValue >> 2U));
}

}  // namespace

FCommandTaskSignatureKey::FCommandTaskSignatureKey()
{
    Reset();
}

void FCommandTaskSignatureKey::Reset()
{
    TaskId = 0U;
    SourceGoalId = 0U;
    AbilityId = ABILITY_ID::INVALID;
    ResultUnitTypeId = UNIT_TYPEID::INVALID;
    UpgradeId = UpgradeID(UPGRADE_ID::INVALID);
    PreferredPlacementSlotId.Reset();
    PreferredProducerPlacementSlotId.Reset();
}

FCommandTaskSignatureKey FCommandTaskSignatureKey::FromOrderRecord(const FCommandOrderRecord& CommandOrderRecordValue)
{
    FCommandTaskSignatureKey CommandTaskSignatureKeyValue;
    CommandTaskSignatureKeyValue.TaskId = CommandOrderRecordValue.PlanStepId;
    CommandTaskSignatureKeyValue.SourceGoalId = CommandOrderRecordValue.SourceGoalId;
    CommandTaskSignatureKeyValue.AbilityId = CommandOrderRecordValue.AbilityId;
    CommandTaskSignatureKeyValue.ResultUnitTypeId = CommandOrderRecordValue.ResultUnitTypeId;
    CommandTaskSignatureKeyValue.UpgradeId = CommandOrderRecordValue.UpgradeId;
    CommandTaskSignatureKeyValue.PreferredPlacementSlotId = CommandOrderRecordValue.PreferredPlacementSlotId;
    CommandTaskSignatureKeyValue.PreferredProducerPlacementSlotId =
        CommandOrderRecordValue.PreferredProducerPlacementSlotId;
    return CommandTaskSignatureKeyValue;
}

FCommandTaskSignatureKey FCommandTaskSignatureKey::FromTaskDescriptor(
    const FCommandTaskDescriptor& CommandTaskDescriptorValue)
{
    FCommandTaskSignatureKey CommandTaskSignatureKeyValue;
    CommandTaskSignatureKeyValue.TaskId = CommandTaskDescriptorValue.TaskId;
    CommandTaskSignatureKeyValue.SourceGoalId = CommandTaskDescriptorValue.SourceGoalId;
    CommandTaskSignatureKeyValue.AbilityId = CommandTaskDescriptorValue.ActionAbilityId;
    CommandTaskSignatureKeyValue.ResultUnitTypeId = CommandTaskDescriptorValue.ActionResultUnitTypeId;
    CommandTaskSignatureKeyValue.UpgradeId = CommandTaskDescriptorValue.ActionUpgradeId;
    CommandTaskSignatureKeyValue.PreferredPlacementSlotId =
        CommandTaskDescriptorValue.ActionPreferredPlacementSlotId;
    CommandTaskSignatureKeyValue.PreferredProducerPlacementSlotId =
        CommandTaskDescriptorValue.ActionPreferredProducerPlacementSlotId;
    return CommandTaskSignatureKeyValue;
}

bool FCommandTaskSignatureKey::operator==(const FCommandTaskSignatureKey& CommandTaskSignatureKeyValue) const
{
    if (TaskId != 0U || CommandTaskSignatureKeyValue.TaskId != 0U)
    {
        return TaskId == CommandTaskSignatureKeyValue.TaskId;
    }

    return SourceGoalId == CommandTaskSignatureKeyValue.SourceGoalId &&
           AbilityId == CommandTaskSignatureKeyValue.AbilityId &&
           ResultUnitTypeId == CommandTaskSignatureKeyValue.ResultUnitTypeId &&
           UpgradeId == CommandTaskSignatureKeyValue.UpgradeId &&
           PreferredPlacementSlotId == CommandTaskSignatureKeyValue.PreferredPlacementSlotId &&
           PreferredProducerPlacementSlotId == CommandTaskSignatureKeyValue.PreferredProducerPlacementSlotId;
}

size_t FCommandTaskSignatureKeyHash::operator()(
    const FCommandTaskSignatureKey& CommandTaskSignatureKeyValue) const
{
    size_t HashValue = std::hash<uint32_t>{}(CommandTaskSignatureKeyValue.TaskId);
    HashValue = CombineHash(HashValue, std::hash<uint32_t>{}(CommandTaskSignatureKeyValue.SourceGoalId));
    HashValue = CombineHash(
        HashValue, std::hash<int>{}(static_cast<int>(CommandTaskSignatureKeyValue.AbilityId.ToType())));
    HashValue = CombineHash(
        HashValue, std::hash<int>{}(static_cast<int>(CommandTaskSignatureKeyValue.ResultUnitTypeId)));
    HashValue = CombineHash(
        HashValue, std::hash<int>{}(static_cast<int>(CommandTaskSignatureKeyValue.UpgradeId.ToType())));
    HashValue = CombineHash(
        HashValue, std::hash<int>{}(static_cast<int>(CommandTaskSignatureKeyValue.PreferredPlacementSlotId.SlotType)));
    HashValue = CombineHash(
        HashValue, std::hash<uint8_t>{}(CommandTaskSignatureKeyValue.PreferredPlacementSlotId.Ordinal));
    HashValue = CombineHash(HashValue, std::hash<int>{}(
                                           static_cast<int>(
                                               CommandTaskSignatureKeyValue.PreferredProducerPlacementSlotId.SlotType)));
    HashValue = CombineHash(
        HashValue, std::hash<uint8_t>{}(CommandTaskSignatureKeyValue.PreferredProducerPlacementSlotId.Ordinal));
    return HashValue;
}

}  // namespace sc2
