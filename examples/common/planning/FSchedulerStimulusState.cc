#include "common/planning/FSchedulerStimulusState.h"

namespace sc2
{

FSchedulerStimulusState::FSchedulerStimulusState()
{
    Reset();
}

void FSchedulerStimulusState::Reset()
{
    GoalRevision = 1U;
    ResourceRevision = 1U;
    ProducerRevision = 1U;
    PlacementRevision = 1U;
    ArmyMissionRevision = 1U;
    LastGoalFingerprint = 0U;
    LastResourceFingerprint = 0U;
    LastProducerFingerprint = 0U;
    LastPlacementFingerprint = 0U;
    LastArmyMissionFingerprint = 0U;
}

}  // namespace sc2
