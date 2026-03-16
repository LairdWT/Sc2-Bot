#pragma once

#include <cstdint>

namespace sc2
{

enum class EProductionRallyApplyState : uint8_t
{
    Unknown,
    PendingApply,
    Applied,
};

}  // namespace sc2
