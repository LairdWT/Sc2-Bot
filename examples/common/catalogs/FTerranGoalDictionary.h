#pragma once

#include <cstddef>
#include <cstdint>

#include "common/catalogs/ETerranGoalDefinitionId.h"

namespace sc2
{

struct FTerranGoalDefinition;

class FTerranGoalDictionary
{
public:
    static size_t GetDefinitionCount();
    static const FTerranGoalDefinition& GetDefinitionByIndex(size_t DefinitionIndexValue);
    static const FTerranGoalDefinition* TryGetByDefinitionId(ETerranGoalDefinitionId DefinitionIdValue);
    static const FTerranGoalDefinition* TryGetByGoalId(uint32_t GoalIdValue);
};

}  // namespace sc2

