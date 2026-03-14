#pragma once

#include <cstddef>
#include <cstdint>

namespace sc2
{

enum class ECommandPriorityTier : uint8_t
{
    Critical,
    High,
    Normal,
    Low,
};

constexpr size_t CommandPriorityTierCountValue = 4U;

size_t GetCommandPriorityTierIndex(ECommandPriorityTier CommandPriorityTierValue);
const char* ToString(ECommandPriorityTier CommandPriorityTierValue);

}  // namespace sc2
