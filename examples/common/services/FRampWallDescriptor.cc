#include "common/services/FRampWallDescriptor.h"

namespace sc2
{

FRampWallDescriptor::FRampWallDescriptor()
{
    Reset();
}

void FRampWallDescriptor::Reset()
{
    bIsValid = false;
    WallCenterPoint = Point2D();
    InsideStagingPoint = Point2D();
    OutsideStagingPoint = Point2D();
    LeftDepotSlot.Reset();
    BarracksSlot.Reset();
    RightDepotSlot.Reset();
}

}  // namespace sc2
