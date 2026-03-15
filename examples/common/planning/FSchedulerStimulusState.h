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
    uint64_t LastResourceFingerprint;
    uint64_t LastProducerFingerprint;
    uint64_t LastPlacementFingerprint;
    uint64_t LastArmyMissionFingerprint;
};

}  // namespace sc2
