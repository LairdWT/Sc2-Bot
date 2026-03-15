#pragma once

#include <cstdint>

#include "sc2api/sc2_common.h"

namespace sc2
{

struct FProductionRallyState
{
public:
    static constexpr uint32_t DefaultMaxApplyAttemptCount = 4U;

    FProductionRallyState();

    void Reset();

public:
    Point2D DesiredRallyPoint;
    Point2D LastAppliedRallyPoint;
    uint64_t LastAppliedGameLoop;
    uint64_t NextAllowedApplyGameLoop;
    uint32_t PendingApplyAttemptCount;
    bool bNeedsInitialApply;
};

}  // namespace sc2
