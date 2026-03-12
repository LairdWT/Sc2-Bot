#pragma once

#include <cstdint>

namespace sc2
{

enum class EGamePlan : uint8_t
{
    Unknown,
    Macro,
    Aggressive,
    TimingAttack,
    AllIn,
    Recovery,
};

const char* ToString(const EGamePlan GamePlanValue);

}  // namespace sc2
