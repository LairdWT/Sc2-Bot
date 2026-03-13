#include "common/services/FBuildPlacementSlot.h"

namespace sc2
{

FBuildPlacementSlot::FBuildPlacementSlot()
{
    Reset();
}

void FBuildPlacementSlot::Reset()
{
    SlotType = EBuildPlacementSlotType::MainSupportStructure;
    FootprintPolicy = EBuildPlacementFootprintPolicy::StructureOnly;
    BuildPoint = Point2D();
}

}  // namespace sc2
