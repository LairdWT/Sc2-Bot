#pragma once

#include <cstddef>
#include <vector>

#include "common/catalogs/FMapLayoutTypes.h"

namespace sc2
{

// Number of authored map descriptors in the generated data
constexpr size_t MapDescriptorCountValue = 1U;

// Static array of all authored map descriptors, generated from MapLayoutDictionary.yaml
extern const std::vector<FMapDescriptor> GMapDescriptors;

}  // namespace sc2
