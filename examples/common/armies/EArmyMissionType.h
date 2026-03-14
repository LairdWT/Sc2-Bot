#pragma once

#include <cstdint>

namespace sc2
{

enum class EArmyMissionType : uint8_t
{
    DefendOwnedBase,
    AssembleAtRally,
    PressureKnownEnemyBase,
    ClearKnownEnemyStructures,
    SweepExpansionLocations,
    Regroup,
};

const char* ToString(EArmyMissionType ArmyMissionTypeValue);

}  // namespace sc2
