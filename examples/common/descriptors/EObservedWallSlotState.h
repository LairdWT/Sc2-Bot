#pragma once

#include <cstdint>

namespace sc2
{

enum class EObservedWallSlotState : uint8_t
{
    Unknown,
    Empty,
    Occupied,
};

const char* ToString(EObservedWallSlotState ObservedWallSlotStateValue);

}  // namespace sc2
