#pragma once

#include "common/planning/EUnitTacticalBehavior.h"
#include "sc2api/sc2_common.h"
#include "sc2api/sc2_gametypes.h"

namespace sc2
{

struct FTacticalBehaviorScore
{
public:
    FTacticalBehaviorScore();

    void Reset();

public:
    EUnitTacticalBehavior Behavior;
    int ScoreValue;
    Point2D TargetPoint;
    Tag TargetUnitTag;
};

}  // namespace sc2
