#pragma once

#include <cstdint>

namespace sc2
{

enum class EGoalDomain : uint8_t
{
    Economy,
    Army,
    Production,
    Technology,
    Defense,
    Scouting,
};

const char* ToString(EGoalDomain GoalDomainValue);

}  // namespace sc2
