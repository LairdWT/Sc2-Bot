#pragma once

#include <cstdint>

namespace sc2
{

enum class EArmyGoal : uint8_t
{
    Unknown,
    MapControl,
    HoldBase,
    FrontalAssault,
    TimingAttack,
    Feint,
    Backstab,
    Surround,
};

const char* ToString(const EArmyGoal ArmyGoalValue);

}  // namespace sc2
