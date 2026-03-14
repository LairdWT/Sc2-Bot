#include "common/goals/EGoalHorizon.h"

namespace sc2
{

const char* ToString(const EGoalHorizon GoalHorizonValue)
{
    switch (GoalHorizonValue)
    {
        case EGoalHorizon::Immediate:
            return "Immediate";
        case EGoalHorizon::NearTerm:
            return "NearTerm";
        case EGoalHorizon::Strategic:
            return "Strategic";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
