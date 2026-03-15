#pragma once

#include <cstdint>

namespace sc2
{

enum class EBlockedTaskWakeKind : uint8_t
{
    ProducerAvailability,
    Resources,
    Placement,
    GoalRevision,
    CooldownOnly,
};

const char* ToString(EBlockedTaskWakeKind BlockedTaskWakeKindValue);

}  // namespace sc2
