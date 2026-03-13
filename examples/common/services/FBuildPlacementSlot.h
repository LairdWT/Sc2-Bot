#pragma once

#include "common/services/EBuildPlacementFootprintPolicy.h"
#include "common/services/FBuildPlacementSlotId.h"
#include "sc2api/sc2_common.h"

namespace sc2
{

struct FBuildPlacementSlot
{
public:
    FBuildPlacementSlot();

    void Reset();

public:
    FBuildPlacementSlotId SlotId;
    EBuildPlacementFootprintPolicy FootprintPolicy;
    Point2D BuildPoint;
};

}  // namespace sc2
