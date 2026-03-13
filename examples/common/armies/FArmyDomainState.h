#pragma once

#include <cstdint>
#include <vector>

#include "common/armies/EArmyGoal.h"
#include "common/armies/EArmyPosture.h"

namespace sc2
{

struct FArmyDomainState
{
    uint32_t MinimumArmyCount;
    uint32_t ActiveArmyCount;
    uint32_t ActiveSquadCount;
    uint32_t ReserveUnitCount;
    uint32_t PrimaryArmyAttackSupplyThreshold;
    uint32_t PrimaryArmyDisengageSupplyThreshold;
    std::vector<EArmyGoal> ArmyGoals;
    std::vector<EArmyPosture> ArmyPostures;

    FArmyDomainState();

    void Reset();
    void EnsurePrimaryArmyExists();
};

}  // namespace sc2
