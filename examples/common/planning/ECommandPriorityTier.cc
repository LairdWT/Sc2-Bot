#include "common/planning/ECommandPriorityTier.h"

namespace sc2
{

size_t GetCommandPriorityTierIndex(const ECommandPriorityTier CommandPriorityTierValue)
{
    switch (CommandPriorityTierValue)
    {
        case ECommandPriorityTier::Critical:
            return 0U;
        case ECommandPriorityTier::High:
            return 1U;
        case ECommandPriorityTier::Normal:
            return 2U;
        case ECommandPriorityTier::Low:
            return 3U;
        default:
            return CommandPriorityTierCountValue - 1U;
    }
}

const char* ToString(const ECommandPriorityTier CommandPriorityTierValue)
{
    switch (CommandPriorityTierValue)
    {
        case ECommandPriorityTier::Critical:
            return "Critical";
        case ECommandPriorityTier::High:
            return "High";
        case ECommandPriorityTier::Normal:
            return "Normal";
        case ECommandPriorityTier::Low:
            return "Low";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
