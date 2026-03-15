#pragma once

#include <cstdint>

namespace sc2
{

enum class ECommandTaskOrigin : uint8_t
{
    Opening,
    GoalMacro,
    Recovery,
};

const char* ToString(ECommandTaskOrigin CommandTaskOriginValue);

}  // namespace sc2
