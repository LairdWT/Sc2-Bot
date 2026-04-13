#pragma once

#include <cstdint>

namespace sc2
{

// Identifies a specific playable map. Each map has a unique identifier
// used for compile-time lookup of authored layout data.
enum class EMapId : uint8_t
{
    Invalid = 0,
    BelShirVestige,
    Count
};

}  // namespace sc2
