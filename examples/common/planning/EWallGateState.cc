#include "common/planning/EWallGateState.h"

namespace sc2
{

const char* ToString(const EWallGateState WallGateStateValue)
{
    switch (WallGateStateValue)
    {
        case EWallGateState::Unavailable:
            return "Unavailable";
        case EWallGateState::Open:
            return "Open";
        case EWallGateState::Closed:
            return "Closed";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
