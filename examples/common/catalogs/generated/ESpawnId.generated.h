#pragma once

#include <cstdint>

namespace sc2
{

// Identifies a starting location on a map by its cardinal position.
// Each map has 2-4 spawns; each spawn has unique layout parameters.
enum class ESpawnId : uint8_t
{
    Invalid = 0,
    NorthWest,
    NorthEast,
    SouthWest,
    SouthEast,
    Count
};

}  // namespace sc2
