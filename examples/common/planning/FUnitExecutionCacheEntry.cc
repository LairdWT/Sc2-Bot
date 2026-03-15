#include "common/planning/FUnitExecutionCacheEntry.h"

#include "common/agent_framework.h"

namespace sc2
{

FUnitExecutionCacheEntry::FUnitExecutionCacheEntry()
{
    Reset();
}

void FUnitExecutionCacheEntry::Reset()
{
    LastMissionRevision = 0U;
    LastTacticalBehavior = EUnitTacticalBehavior::AdvanceToMissionAnchor;
    LastTargetKind = EIntentTargetKind::None;
    LastTargetPoint = Point2D();
    LastTargetUnitTag = NullTag;
    LastIssuedGameLoop = 0U;
    bInsideAssemblyRadius = false;
}

}  // namespace sc2
