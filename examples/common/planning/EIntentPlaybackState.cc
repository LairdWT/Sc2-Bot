#include "common/planning/EIntentPlaybackState.h"

namespace sc2
{

const char* ToString(const EIntentPlaybackState IntentPlaybackStateValue)
{
    switch (IntentPlaybackStateValue)
    {
        case EIntentPlaybackState::Idle:
            return "Idle";
        case EIntentPlaybackState::ReadyBufferPending:
            return "ReadyBufferPending";
        case EIntentPlaybackState::Dispatching:
            return "Dispatching";
        case EIntentPlaybackState::Blocked:
            return "Blocked";
        default:
            return "Idle";
    }
}

}  // namespace sc2
