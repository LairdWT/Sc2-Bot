#pragma once

#include <cstdint>

#include "common/planning/EProductionRallyApplyState.h"
#include "sc2api/sc2_common.h"

namespace sc2
{

struct FProductionRallyState
{
public:
    FProductionRallyState();

    void Reset();

public:
    Point2D DesiredRallyPoint;
    Point2D LastAppliedRallyPoint;
    uint64_t LastAppliedGameLoop;
    EProductionRallyApplyState ApplyState;
};

}  // namespace sc2
