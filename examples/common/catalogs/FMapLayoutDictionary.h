#pragma once

#include <string>

#include "common/catalogs/FMapLayoutTypes.h"
#include "sc2api/sc2_common.h"

namespace sc2
{

// Provides lookup functions for authored map layout data.
// All data is compiled from MapLayoutDictionary.yaml into static arrays.
// Lookups match by normalized map name and nearest base location.
struct FMapLayoutDictionary
{
public:
    // Returns the map descriptor matching the normalized map name, or nullptr
    static const FMapDescriptor* TryGetMapByName(const std::string& NormalizedMapNameValue);

    // Returns the spawn layout matching the start location on the given map, or nullptr
    static const FMapSpawnLayout* TryGetSpawnLayout(
        const FMapDescriptor& MapDescriptorValue,
        const Point2D& StartLocationValue);

    // Returns the spawn layout for the enemy at the opposing position, or nullptr
    static const FMapSpawnLayout* TryGetEnemySpawnLayout(
        const FMapDescriptor& MapDescriptorValue,
        const FMapSpawnLayout& OwnSpawnLayoutValue);

    // Normalizes a map name by removing non-alphanumeric characters and lowercasing
    static std::string NormalizeMapName(const std::string& MapNameValue);
};

}  // namespace sc2
