#include "common/planning/FProductionRallyState.h"

namespace sc2
{

FProductionRallyState::FProductionRallyState()
{
    Reset();
}

void FProductionRallyState::Reset()
{
    DesiredRallyPoint = Point2D();
    LastAppliedRallyPoint = Point2D();
    LastAppliedGameLoop = 0U;
    NextAllowedApplyGameLoop = 0U;
    PendingApplyAttemptCount = DefaultMaxApplyAttemptCount;
    bNeedsInitialApply = true;
}

}  // namespace sc2
