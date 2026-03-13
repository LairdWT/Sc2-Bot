#include "common/services/FBuildPlacementSlotId.h"

namespace sc2
{

FBuildPlacementSlotId::FBuildPlacementSlotId()
{
    Reset();
}

void FBuildPlacementSlotId::Reset()
{
    SlotType = EBuildPlacementSlotType::Unknown;
    Ordinal = 0U;
}

bool FBuildPlacementSlotId::IsValid() const
{
    return SlotType != EBuildPlacementSlotType::Unknown;
}

bool operator==(const FBuildPlacementSlotId& LeftValue, const FBuildPlacementSlotId& RightValue)
{
    return LeftValue.SlotType == RightValue.SlotType && LeftValue.Ordinal == RightValue.Ordinal;
}

bool operator!=(const FBuildPlacementSlotId& LeftValue, const FBuildPlacementSlotId& RightValue)
{
    return !(LeftValue == RightValue);
}

}  // namespace sc2
