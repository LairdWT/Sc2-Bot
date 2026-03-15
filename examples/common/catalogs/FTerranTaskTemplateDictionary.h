#pragma once

#include <cstddef>

#include "common/catalogs/ETerranTaskTemplateId.h"
#include "common/planning/FCommandTaskDescriptor.h"

namespace sc2
{

struct FTerranTaskTemplateDefinition;

class FTerranTaskTemplateDictionary
{
public:
    static size_t GetDefinitionCount();
    static const FTerranTaskTemplateDefinition& GetDefinitionByIndex(size_t DefinitionIndexValue);
    static const FTerranTaskTemplateDefinition* TryGetByTemplateId(ETerranTaskTemplateId TemplateIdValue);
    static const FTerranTaskTemplateDefinition* TryFindByAction(AbilityID AbilityIdValue,
                                                                UNIT_TYPEID ResultUnitTypeIdValue,
                                                                UpgradeID UpgradeIdValue);
    static bool TryCreateTaskDescriptor(ETerranTaskTemplateId TemplateIdValue,
                                        FCommandTaskDescriptor& OutTaskDescriptorValue);
    static bool TryCreateTaskDescriptorForAction(AbilityID AbilityIdValue, UNIT_TYPEID ResultUnitTypeIdValue,
                                                 UpgradeID UpgradeIdValue,
                                                 FCommandTaskDescriptor& OutTaskDescriptorValue);
};

}  // namespace sc2

