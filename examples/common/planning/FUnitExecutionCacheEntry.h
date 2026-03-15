#pragma once

#include <cstdint>

#include "common/planning/EUnitTacticalBehavior.h"
#include "sc2api/sc2_common.h"
#include "sc2api/sc2_gametypes.h"

namespace sc2
{

enum class EIntentTargetKind : uint8_t;

struct FUnitExecutionCacheEntry
{
public:
    FUnitExecutionCacheEntry();

    void Reset();

public:
    uint64_t LastMissionRevision;
    EUnitTacticalBehavior LastTacticalBehavior;
    EIntentTargetKind LastTargetKind;
    Point2D LastTargetPoint;
    Tag LastTargetUnitTag;
    uint64_t LastIssuedGameLoop;
    bool bInsideAssemblyRadius;
};

}  // namespace sc2
