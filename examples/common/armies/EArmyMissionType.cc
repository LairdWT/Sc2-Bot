#include "common/armies/EArmyMissionType.h"

namespace sc2
{

const char* ToString(const EArmyMissionType ArmyMissionTypeValue)
{
    switch (ArmyMissionTypeValue)
    {
        case EArmyMissionType::DefendOwnedBase:
            return "DefendOwnedBase";
        case EArmyMissionType::AssembleAtRally:
            return "AssembleAtRally";
        case EArmyMissionType::PressureKnownEnemyBase:
            return "PressureKnownEnemyBase";
        case EArmyMissionType::ClearKnownEnemyStructures:
            return "ClearKnownEnemyStructures";
        case EArmyMissionType::SweepExpansionLocations:
            return "SweepExpansionLocations";
        case EArmyMissionType::Regroup:
            return "Regroup";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
