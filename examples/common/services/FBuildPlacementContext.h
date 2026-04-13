#pragma once

#include <string>

#include "common/catalogs/FMapLayoutTypes.h"
#include "common/services/FMainBaseLayoutDescriptor.h"
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
    bool HasPlayableBounds() const;

public:
    std::string MapName;
    Point2D BaseLocation;
    Point2D NaturalLocation;
    Point2D PlayableMin;
    Point2D PlayableMax;
    FRampWallDescriptor RampWallDescriptor;
    FMainBaseLayoutDescriptor MainBaseLayoutDescriptor;

    // Per-map static layout data (nullptr if map not authored)
    const FMapDescriptor* MapDescriptorPtr;

    // Per-spawn layout for the current starting location (nullptr if unknown)
    const FMapSpawnLayout* SpawnLayoutPtr;
};

}  // namespace sc2
