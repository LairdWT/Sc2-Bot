#pragma once

#include <cstdint>

namespace sc2
{

struct FSchedulerStimulusState
{
public:
    FSchedulerStimulusState();

    void Reset();

public:
    uint64_t GoalRevision;
    uint64_t ResourceRevision;
    uint64_t ProducerRevision;
    uint64_t PlacementRevision;
    uint64_t ArmyMissionRevision;
    uint64_t LastGoalFingerprint;
    uint64_t LastPlacementFingerprint;
    uint64_t LastArmyMissionFingerprint;
    uint32_t LastAvailableMinerals;
    uint32_t LastAvailableVespene;
    uint32_t LastAvailableSupply;
    uint32_t LastWorkerCount;
    uint32_t LastTownHallCount;
    uint32_t LastBarracksCount;
    uint32_t LastFactoryCount;
    uint32_t LastStarportCount;
};

}  // namespace sc2
