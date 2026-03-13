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
    BaseLocation = Point2D();
    NaturalLocation = Point2D(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN());
    RampWallDescriptor.Reset();
}

bool FBuildPlacementContext::HasNaturalLocation() const
{
    return std::isfinite(NaturalLocation.x) && std::isfinite(NaturalLocation.y);
}

}  // namespace sc2
