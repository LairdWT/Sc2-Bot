#pragma once

#include <cstdint>

namespace sc2
{

enum class EMacroPhase : uint8_t
{
    Opening,
    EarlyGame,
    MidGame,
    LateGame,
    Recovery,
};

const char* ToString(const EMacroPhase MacroPhaseValue);

}  // namespace sc2
