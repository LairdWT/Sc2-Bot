#include "common/planning/FTacticalBehaviorScore.h"

#include <limits>

namespace sc2
{

FTacticalBehaviorScore::FTacticalBehaviorScore()
{
    Reset();
}

void FTacticalBehaviorScore::Reset()
{
    Behavior = EUnitTacticalBehavior::AdvanceToMissionAnchor;
    ScoreValue = std::numeric_limits<int>::min();
    TargetPoint = Point2D();
    TargetUnitTag = NullTag;
}

}  // namespace sc2
