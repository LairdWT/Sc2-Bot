#pragma once

#include <cstdint>

namespace sc2
{

enum class ETerranGoalDefinitionId : uint16_t
{
    Invalid = 0,
    HoldOwnedBase,
    SaturateWorkers,
    MaintainSupply,
    ExpandBaseCount,
    BuildRefineryCapacity,
    BuildBarracksCapacity,
    BuildFactoryCapacity,
    BuildStarportCapacity,
    UnlockBarracksReactor,
    UnlockFactoryTechLab,
    UnlockBarracksTechLab,
    UnlockEngineeringBay,
    UnlockSecondEngineeringBay,
    UnlockStarportReactor,
    ResearchStimpack,
    ResearchCombatShield,
    ResearchInfantryWeaponsLevel1,
    ResearchInfantryArmorLevel1,
    ResearchConcussiveShells,
    ProduceMarines,
    ProduceMarauders,
    ProduceCyclones,
    ProduceSiegeTanks,
    ProduceMedivacs,
    ProduceLiberators,
    PressureEnemy,
    ClearEnemyPresence,
    ScoutExpansionLocations,
    Count
};

}  // namespace sc2
