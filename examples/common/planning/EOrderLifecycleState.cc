#include "common/planning/EOrderLifecycleState.h"

namespace sc2
{

const char* ToString(const EOrderLifecycleState OrderLifecycleStateValue)
{
    switch (OrderLifecycleStateValue)
    {
        case EOrderLifecycleState::Queued:
            return "Queued";
        case EOrderLifecycleState::Preprocessing:
            return "Preprocessing";
        case EOrderLifecycleState::Ready:
            return "Ready";
        case EOrderLifecycleState::Dispatched:
            return "Dispatched";
        case EOrderLifecycleState::Completed:
            return "Completed";
        case EOrderLifecycleState::Aborted:
            return "Aborted";
        case EOrderLifecycleState::Expired:
            return "Expired";
        default:
            return "Queued";
    }
}

bool IsTerminalLifecycleState(const EOrderLifecycleState OrderLifecycleStateValue)
{
    switch (OrderLifecycleStateValue)
    {
        case EOrderLifecycleState::Completed:
        case EOrderLifecycleState::Aborted:
        case EOrderLifecycleState::Expired:
            return true;
        default:
            return false;
    }
}

}  // namespace sc2
