#include "common/services/FBuildPlacementSlot.h"

namespace sc2
{

FBuildPlacementSlot::FBuildPlacementSlot()
{
    Reset();
}

void FBuildPlacementSlot::Reset()
{
    SlotId.Reset();
    FootprintPolicy = EBuildPlacementFootprintPolicy::StructureOnly;
    BuildPoint = Point2D();
}

}  // namespace sc2
