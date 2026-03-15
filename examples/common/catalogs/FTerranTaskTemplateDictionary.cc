#include "common/catalogs/FTerranTaskTemplateDictionary.h"

#include <stdexcept>

#include "common/catalogs/FTerranTaskTemplateDefinition.h"
#include "common/catalogs/generated/FTerranTaskTemplateDictionaryData.generated.h"

namespace sc2
{
namespace
{

bool DoesTaskTemplateMatchAction(const FTerranTaskTemplateDefinition& TerranTaskTemplateDefinitionValue,
                                 const AbilityID AbilityIdValue, const UNIT_TYPEID ResultUnitTypeIdValue,
                                 const UpgradeID UpgradeIdValue)
{
    if (TerranTaskTemplateDefinitionValue.ActionAbilityId != AbilityIdValue)
    {
        return false;
    }

    if (TerranTaskTemplateDefinitionValue.ActionUpgradeId != UpgradeID(UPGRADE_ID::INVALID) ||
        UpgradeIdValue != UpgradeID(UPGRADE_ID::INVALID))
    {
        return TerranTaskTemplateDefinitionValue.ActionUpgradeId == UpgradeIdValue;
    }

    if (TerranTaskTemplateDefinitionValue.ActionResultUnitTypeId != UNIT_TYPEID::INVALID ||
        ResultUnitTypeIdValue != UNIT_TYPEID::INVALID)
    {
        return TerranTaskTemplateDefinitionValue.ActionResultUnitTypeId == ResultUnitTypeIdValue;
    }

    return true;
}

}  // namespace

size_t FTerranTaskTemplateDictionary::GetDefinitionCount()
{
    return GTerranTaskTemplateDefinitions.size();
}

const FTerranTaskTemplateDefinition& FTerranTaskTemplateDictionary::GetDefinitionByIndex(
    const size_t DefinitionIndexValue)
{
    if (DefinitionIndexValue >= GTerranTaskTemplateDefinitions.size())
    {
        throw std::out_of_range("Terran task template definition index is out of range.");
    }

    return GTerranTaskTemplateDefinitions[DefinitionIndexValue];
}

const FTerranTaskTemplateDefinition* FTerranTaskTemplateDictionary::TryGetByTemplateId(
    const ETerranTaskTemplateId TemplateIdValue)
{
    for (const FTerranTaskTemplateDefinition& TerranTaskTemplateDefinitionValue : GTerranTaskTemplateDefinitions)
    {
        if (TerranTaskTemplateDefinitionValue.TemplateId == TemplateIdValue)
        {
            return &TerranTaskTemplateDefinitionValue;
        }
    }

    return nullptr;
}

const FTerranTaskTemplateDefinition* FTerranTaskTemplateDictionary::TryFindByAction(
    const AbilityID AbilityIdValue, const UNIT_TYPEID ResultUnitTypeIdValue, const UpgradeID UpgradeIdValue)
{
    for (const FTerranTaskTemplateDefinition& TerranTaskTemplateDefinitionValue : GTerranTaskTemplateDefinitions)
    {
        if (DoesTaskTemplateMatchAction(TerranTaskTemplateDefinitionValue, AbilityIdValue, ResultUnitTypeIdValue,
                                        UpgradeIdValue))
        {
            return &TerranTaskTemplateDefinitionValue;
        }
    }

    return nullptr;
}

bool FTerranTaskTemplateDictionary::TryCreateTaskDescriptor(const ETerranTaskTemplateId TemplateIdValue,
                                                            FCommandTaskDescriptor& OutTaskDescriptorValue)
{
    const FTerranTaskTemplateDefinition* TerranTaskTemplateDefinitionValue = TryGetByTemplateId(TemplateIdValue);
    if (TerranTaskTemplateDefinitionValue == nullptr)
    {
        return false;
    }

    OutTaskDescriptorValue.Reset();
    OutTaskDescriptorValue.PackageKind = TerranTaskTemplateDefinitionValue->PackageKind;
    OutTaskDescriptorValue.NeedKind = TerranTaskTemplateDefinitionValue->NeedKind;
    OutTaskDescriptorValue.ActionKind = TerranTaskTemplateDefinitionValue->ActionKind;
    OutTaskDescriptorValue.CompletionKind = TerranTaskTemplateDefinitionValue->CompletionKind;
    OutTaskDescriptorValue.TaskType = TerranTaskTemplateDefinitionValue->TaskType;
    OutTaskDescriptorValue.Origin = TerranTaskTemplateDefinitionValue->Origin;
    OutTaskDescriptorValue.CommitmentClass = TerranTaskTemplateDefinitionValue->CommitmentClass;
    OutTaskDescriptorValue.ExecutionGuarantee = TerranTaskTemplateDefinitionValue->ExecutionGuarantee;
    OutTaskDescriptorValue.RetentionPolicy = TerranTaskTemplateDefinitionValue->RetentionPolicy;
    OutTaskDescriptorValue.BlockedTaskWakeKind = TerranTaskTemplateDefinitionValue->BlockedTaskWakeKind;
    OutTaskDescriptorValue.ActionAbilityId = TerranTaskTemplateDefinitionValue->ActionAbilityId;
    OutTaskDescriptorValue.ActionProducerUnitTypeId = TerranTaskTemplateDefinitionValue->ActionProducerUnitTypeId;
    OutTaskDescriptorValue.ActionResultUnitTypeId = TerranTaskTemplateDefinitionValue->ActionResultUnitTypeId;
    OutTaskDescriptorValue.ActionUpgradeId = TerranTaskTemplateDefinitionValue->ActionUpgradeId;
    OutTaskDescriptorValue.ActionTargetCount = TerranTaskTemplateDefinitionValue->DefaultTargetCount;
    OutTaskDescriptorValue.ActionRequestedQueueCount = TerranTaskTemplateDefinitionValue->DefaultRequestedQueueCount;
    OutTaskDescriptorValue.ActionPreferredPlacementSlotType =
        TerranTaskTemplateDefinitionValue->DefaultPreferredPlacementSlotType;
    OutTaskDescriptorValue.CompletionObservedCountAtLeast = TerranTaskTemplateDefinitionValue->DefaultTargetCount;
    return true;
}

bool FTerranTaskTemplateDictionary::TryCreateTaskDescriptorForAction(
    const AbilityID AbilityIdValue, const UNIT_TYPEID ResultUnitTypeIdValue, const UpgradeID UpgradeIdValue,
    FCommandTaskDescriptor& OutTaskDescriptorValue)
{
    const FTerranTaskTemplateDefinition* TerranTaskTemplateDefinitionValue =
        TryFindByAction(AbilityIdValue, ResultUnitTypeIdValue, UpgradeIdValue);
    if (TerranTaskTemplateDefinitionValue == nullptr)
    {
        return false;
    }

    return TryCreateTaskDescriptor(TerranTaskTemplateDefinitionValue->TemplateId, OutTaskDescriptorValue);
}

}  // namespace sc2
