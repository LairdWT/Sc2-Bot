#pragma once

#include <array>

#include <cstddef>

#include "common/catalogs/FTerranTaskTemplateDefinition.h"

namespace sc2
{

constexpr size_t TerranTaskTemplateDefinitionCountValue = 29U;

extern const std::array<FTerranTaskTemplateDefinition, TerranTaskTemplateDefinitionCountValue> GTerranTaskTemplateDefinitions;

}  // namespace sc2
