#pragma once

#include <array>

#include <cstddef>

#include "common/catalogs/FTerranGoalDefinition.h"

namespace sc2
{

constexpr size_t TerranGoalDefinitionCountValue = 28U;

extern const std::array<FTerranGoalDefinition, TerranGoalDefinitionCountValue> GTerranGoalDefinitions;

}  // namespace sc2
