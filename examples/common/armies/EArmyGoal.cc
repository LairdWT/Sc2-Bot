#include "common/armies/EArmyGoal.h"

namespace sc2
{

const char* ToString(const EArmyGoal ArmyGoalValue)
{
    switch (ArmyGoalValue)
    {
        case EArmyGoal::Unknown:
            return "Unknown";
        case EArmyGoal::MapControl:
            return "MapControl";
        case EArmyGoal::HoldBase:
            return "HoldBase";
        case EArmyGoal::FrontalAssault:
            return "FrontalAssault";
        case EArmyGoal::TimingAttack:
            return "TimingAttack";
        case EArmyGoal::Feint:
            return "Feint";
        case EArmyGoal::Backstab:
            return "Backstab";
        case EArmyGoal::Surround:
            return "Surround";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
