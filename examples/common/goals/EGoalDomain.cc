#include "common/goals/EGoalDomain.h"

namespace sc2
{

const char* ToString(const EGoalDomain GoalDomainValue)
{
    switch (GoalDomainValue)
    {
        case EGoalDomain::Economy:
            return "Economy";
        case EGoalDomain::Army:
            return "Army";
        case EGoalDomain::Production:
            return "Production";
        case EGoalDomain::Technology:
            return "Technology";
        case EGoalDomain::Defense:
            return "Defense";
        case EGoalDomain::Scouting:
            return "Scouting";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
