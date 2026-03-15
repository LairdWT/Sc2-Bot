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
    LastPlacementFingerprint = 0U;
    LastArmyMissionFingerprint = 0U;
    LastAvailableMinerals = 0U;
    LastAvailableVespene = 0U;
    LastAvailableSupply = 0U;
    LastWorkerCount = 0U;
    LastTownHallCount = 0U;
    LastBarracksCount = 0U;
    LastFactoryCount = 0U;
    LastStarportCount = 0U;
}

}  // namespace sc2
