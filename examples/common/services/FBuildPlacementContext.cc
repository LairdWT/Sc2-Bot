#include "common/services/FBuildPlacementContext.h"

#include <cmath>
#include <limits>

namespace sc2
{

FBuildPlacementContext::FBuildPlacementContext()
{
    Reset();
}

void FBuildPlacementContext::Reset()
{
    MapName.clear();
    BaseLocation = Point2D();
    NaturalLocation = Point2D(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN());
    PlayableMin = Point2D(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN());
    PlayableMax = Point2D(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN());
    RampWallDescriptor.Reset();
    MainBaseLayoutDescriptor.Reset();
    MapDescriptorPtr = nullptr;
    SpawnLayoutPtr = nullptr;
}

bool FBuildPlacementContext::HasNaturalLocation() const
{
    return std::isfinite(NaturalLocation.x) && std::isfinite(NaturalLocation.y);
}

bool FBuildPlacementContext::HasPlayableBounds() const
{
    return std::isfinite(PlayableMin.x) && std::isfinite(PlayableMin.y) && std::isfinite(PlayableMax.x) &&
           std::isfinite(PlayableMax.y) && PlayableMax.x > PlayableMin.x && PlayableMax.y > PlayableMin.y;
}

}  // namespace sc2
