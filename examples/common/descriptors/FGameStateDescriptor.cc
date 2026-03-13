#include "common/descriptors/FGameStateDescriptor.h"

namespace sc2
{

FGameStateDescriptor::FGameStateDescriptor()
{
    Reset();
}

void FGameStateDescriptor::Reset()
{
    CurrentStep = 0;
    CurrentGameLoop = 0;
    MacroState.Reset();
    ArmyState.Reset();
    BuildPlanning.Reset();
    OpeningPlanExecutionState.Reset();
    CommandAuthoritySchedulingState.Reset();
    SpatialFields.Reset();
}

}  // namespace sc2
