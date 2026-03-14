#pragma once

#include <cstdint>

namespace sc2
{

enum class EGoalType : uint8_t
{
    HoldOwnedBase,
    SaturateWorkers,
    MaintainSupply,
    ExpandBaseCount,
    BuildProductionCapacity,
    UnlockTechnology,
    ResearchUpgrade,
    ProduceArmy,
    PressureEnemy,
    ClearEnemyPresence,
    ScoutExpansionLocations,
};

const char* ToString(EGoalType GoalTypeValue);

}  // namespace sc2
