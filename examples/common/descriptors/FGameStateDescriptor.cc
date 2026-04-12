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
    GoalSet.Reset();
    MacroState.Reset();
    ArmyState.Reset();
    BuildPlanning.Reset();
    CommitmentLedger.Reset();
    EconomyState.Reset();
    ProductionState.Reset();
    ExecutionPressure.Reset();
    OpeningPlanExecutionState.Reset();
    CommandAuthoritySchedulingState.Reset();
    SchedulerOutlook.Reset();
    SpatialFields.Reset();
    RampWallDescriptor.Reset();
    MainBaseLayoutDescriptor.Reset();
    ObservedPlacementSlotState.Reset();
    ObservedRampWallState.Reset();
    EnemyObservation.Reset();
}

}  // namespace sc2
