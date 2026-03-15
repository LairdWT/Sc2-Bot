#include "common/catalogs/FTerranGoalDictionary.h"

#include <stdexcept>

#include "common/catalogs/FTerranGoalDefinition.h"
#include "common/catalogs/generated/FTerranGoalDictionaryData.generated.h"

namespace sc2
{

size_t FTerranGoalDictionary::GetDefinitionCount()
{
    return GTerranGoalDefinitions.size();
}

const FTerranGoalDefinition& FTerranGoalDictionary::GetDefinitionByIndex(const size_t DefinitionIndexValue)
{
    if (DefinitionIndexValue >= GTerranGoalDefinitions.size())
    {
        throw std::out_of_range("Terran goal definition index is out of range.");
    }

    return GTerranGoalDefinitions[DefinitionIndexValue];
}

const FTerranGoalDefinition* FTerranGoalDictionary::TryGetByDefinitionId(
    const ETerranGoalDefinitionId DefinitionIdValue)
{
    for (const FTerranGoalDefinition& TerranGoalDefinitionValue : GTerranGoalDefinitions)
    {
        if (TerranGoalDefinitionValue.DefinitionId == DefinitionIdValue)
        {
            return &TerranGoalDefinitionValue;
        }
    }

    return nullptr;
}

const FTerranGoalDefinition* FTerranGoalDictionary::TryGetByGoalId(const uint32_t GoalIdValue)
{
    for (const FTerranGoalDefinition& TerranGoalDefinitionValue : GTerranGoalDefinitions)
    {
        if (TerranGoalDefinitionValue.GoalId == GoalIdValue)
        {
            return &TerranGoalDefinitionValue;
        }
    }

    return nullptr;
}

}  // namespace sc2

