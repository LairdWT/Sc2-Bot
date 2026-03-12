#pragma once

#include <cstdint>
#include <vector>

#include "common/armies/EArmyGoal.h"

namespace sc2
{

struct FArmyDomainState
{
    uint32_t MinimumArmyCount;
    uint32_t ActiveArmyCount;
    uint32_t ActiveSquadCount;
    uint32_t ReserveUnitCount;
    std::vector<EArmyGoal> ArmyGoals;

    FArmyDomainState();

    void Reset();
    void EnsurePrimaryArmyExists();
};

}  // namespace sc2
