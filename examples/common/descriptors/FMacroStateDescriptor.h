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
    uint64_t CurrentGameLoop;
    uint32_t DesiredBaseCount;
    uint32_t ActiveBaseCount;
    uint32_t DesiredArmyCount;
    uint32_t WorkerCount;
    uint32_t ArmyUnitCount;
    uint32_t ArmySupply;
    uint32_t BarracksCount;
    uint32_t FactoryCount;
    uint32_t StarportCount;
    uint32_t SupplyUsed;
    uint32_t SupplyCap;

    FMacroStateDescriptor();

    void Reset();
};

}  // namespace sc2
