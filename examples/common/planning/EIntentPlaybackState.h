#pragma once

#include <cstdint>

namespace sc2
{

enum class EIntentPlaybackState : uint8_t
{
    Idle,
    ReadyBufferPending,
    Dispatching,
    Blocked,
};

const char* ToString(const EIntentPlaybackState IntentPlaybackStateValue);

}  // namespace sc2
