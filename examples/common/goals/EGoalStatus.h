#pragma once

#include <cstdint>

namespace sc2
{

enum class EGoalStatus : uint8_t
{
    Proposed,
    Active,
    Satisfied,
    Blocked,
    Abandoned,
};

const char* ToString(EGoalStatus GoalStatusValue);

}  // namespace sc2
