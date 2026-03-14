#include "common/goals/EGoalType.h"

namespace sc2
{

const char* ToString(const EGoalType GoalTypeValue)
{
    switch (GoalTypeValue)
    {
        case EGoalType::HoldOwnedBase:
            return "HoldOwnedBase";
        case EGoalType::SaturateWorkers:
            return "SaturateWorkers";
        case EGoalType::MaintainSupply:
            return "MaintainSupply";
        case EGoalType::ExpandBaseCount:
            return "ExpandBaseCount";
        case EGoalType::BuildProductionCapacity:
            return "BuildProductionCapacity";
        case EGoalType::UnlockTechnology:
            return "UnlockTechnology";
        case EGoalType::ResearchUpgrade:
            return "ResearchUpgrade";
        case EGoalType::ProduceArmy:
            return "ProduceArmy";
        case EGoalType::PressureEnemy:
            return "PressureEnemy";
        case EGoalType::ClearEnemyPresence:
            return "ClearEnemyPresence";
        case EGoalType::ScoutExpansionLocations:
            return "ScoutExpansionLocations";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
