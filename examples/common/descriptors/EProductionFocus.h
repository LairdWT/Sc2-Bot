#pragma once

#include <cstdint>

namespace sc2
{

enum class EProductionFocus : uint8_t
{
    Recovery,
    Supply,
    Workers,
    Expansion,
    Production,
    Army,
    Upgrades,
    Defense,
};

const char* ToString(EProductionFocus ProductionFocusValue);

}  // namespace sc2
