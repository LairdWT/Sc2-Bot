#pragma once

#include <cstdint>

#include "common/descriptors/EGamePlan.h"
#include "common/descriptors/EMacroPhase.h"

namespace sc2
{

struct FMacroStateDescriptor
{
    EGamePlan ActiveGamePlan;
    EMacroPhase ActiveMacroPhase;
    uint32_t DesiredBaseCount;
    uint32_t ActiveBaseCount;
    uint32_t DesiredArmyCount;

    FMacroStateDescriptor();

    void Reset();
};

}  // namespace sc2
