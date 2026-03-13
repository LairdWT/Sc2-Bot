#pragma once

#include "common/services/FBuildPlacementSlot.h"
#include "sc2api/sc2_common.h"

namespace sc2
{

struct FRampWallDescriptor
{
public:
    FRampWallDescriptor();

    void Reset();

public:
    bool bIsValid;
    Point2D WallCenterPoint;
    Point2D InsideStagingPoint;
    Point2D OutsideStagingPoint;
    FBuildPlacementSlot LeftDepotSlot;
    FBuildPlacementSlot BarracksSlot;
    FBuildPlacementSlot RightDepotSlot;
};

}  // namespace sc2
