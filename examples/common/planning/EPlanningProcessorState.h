#pragma once

#include <cstdint>

namespace sc2
{

enum class EPlanningProcessorState : uint8_t
{
    Idle,
    Processing,
    ReadyToDrain,
};

const char* ToString(const EPlanningProcessorState PlanningProcessorStateValue);

}  // namespace sc2
