#include "common/descriptors/EObservedWallSlotState.h"

namespace sc2
{

const char* ToString(const EObservedWallSlotState ObservedWallSlotStateValue)
{
    switch (ObservedWallSlotStateValue)
    {
        case EObservedWallSlotState::Unknown:
            return "Unknown";
        case EObservedWallSlotState::Empty:
            return "Empty";
        case EObservedWallSlotState::Occupied:
            return "Occupied";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
