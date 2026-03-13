#pragma once

#include <cstdint>

namespace sc2
{

enum class EArmyPosture : uint8_t
{
    Unknown,
    Assemble,
    Hold,
    Advance,
    Engage,
    Regroup,
};

const char* ToString(const EArmyPosture ArmyPostureValue);

}  // namespace sc2
