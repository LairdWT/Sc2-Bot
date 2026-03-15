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
    bNeedsInitialApply = true;
}

}  // namespace sc2
