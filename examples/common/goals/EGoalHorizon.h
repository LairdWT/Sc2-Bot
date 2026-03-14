#pragma once

#include <cstdint>

namespace sc2
{

enum class EGoalHorizon : uint8_t
{
    Immediate,
    NearTerm,
    Strategic,
};

const char* ToString(EGoalHorizon GoalHorizonValue);

}  // namespace sc2
