#pragma once

#include <cstdint>

#include "common/goals/EGoalDomain.h"
#include "common/goals/EGoalHorizon.h"
#include "common/goals/EGoalStatus.h"
#include "common/goals/EGoalType.h"
#include "sc2api/sc2_common.h"
#include "sc2api/sc2_typeenums.h"

namespace sc2
{

struct FGoalDescriptor
{
public:
    FGoalDescriptor();

    void Reset();

public:
    uint32_t GoalId;
    uint32_t ParentGoalId;
    EGoalDomain GoalDomain;
    EGoalHorizon GoalHorizon;
    EGoalType GoalType;
    EGoalStatus GoalStatus;
    int BasePriorityValue;
    uint32_t TargetCount;
    UNIT_TYPEID TargetUnitTypeId;
    UpgradeID TargetUpgradeId;
    Point2D TargetPoint;
    float TargetRadius;
    int32_t OwningArmyIndex;
    int32_t OwningSquadIndex;
};

}  // namespace sc2
