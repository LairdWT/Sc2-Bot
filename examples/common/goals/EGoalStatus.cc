#include "common/goals/EGoalStatus.h"

namespace sc2
{

const char* ToString(const EGoalStatus GoalStatusValue)
{
    switch (GoalStatusValue)
    {
        case EGoalStatus::Proposed:
            return "Proposed";
        case EGoalStatus::Active:
            return "Active";
        case EGoalStatus::Satisfied:
            return "Satisfied";
        case EGoalStatus::Blocked:
            return "Blocked";
        case EGoalStatus::Abandoned:
            return "Abandoned";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
