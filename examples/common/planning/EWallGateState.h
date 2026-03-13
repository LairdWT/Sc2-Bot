#pragma once

#include <cstdint>

namespace sc2
{

enum class EWallGateState : uint8_t
{
    Unavailable,
    Open,
    Closed,
};

const char* ToString(EWallGateState WallGateStateValue);

}  // namespace sc2
