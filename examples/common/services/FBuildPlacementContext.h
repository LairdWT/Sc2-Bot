#pragma once

#include "common/services/FRampWallDescriptor.h"
#include "sc2api/sc2_common.h"

namespace sc2
{

struct FBuildPlacementContext
{
public:
    FBuildPlacementContext();

    void Reset();
    bool HasNaturalLocation() const;

public:
    Point2D BaseLocation;
    Point2D NaturalLocation;
    FRampWallDescriptor RampWallDescriptor;
};

}  // namespace sc2
