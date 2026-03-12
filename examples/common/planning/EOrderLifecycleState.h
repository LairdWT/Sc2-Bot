#pragma once

#include <cstdint>

namespace sc2
{

enum class EOrderLifecycleState : uint8_t
{
    Queued,
    Preprocessing,
    Ready,
    Dispatched,
    Completed,
    Aborted,
    Expired,
};

const char* ToString(const EOrderLifecycleState OrderLifecycleStateValue);

}  // namespace sc2
