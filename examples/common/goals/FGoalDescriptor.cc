#include "common/goals/FGoalDescriptor.h"

namespace sc2
{

FGoalDescriptor::FGoalDescriptor()
{
    Reset();
}

void FGoalDescriptor::Reset()
{
    GoalId = 0U;
    ParentGoalId = 0U;
    GoalDomain = EGoalDomain::Economy;
    GoalHorizon = EGoalHorizon::Immediate;
    GoalType = EGoalType::HoldOwnedBase;
    GoalStatus = EGoalStatus::Proposed;
    BasePriorityValue = 0;
    TargetCount = 0U;
    TargetUnitTypeId = UNIT_TYPEID::INVALID;
    TargetUpgradeId = UpgradeID(UPGRADE_ID::INVALID);
    TargetPoint = Point2D();
    TargetRadius = 0.0f;
    OwningArmyIndex = -1;
    OwningSquadIndex = -1;
}

}  // namespace sc2
