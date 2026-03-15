#include "common/planning/ECommandTaskRetentionPolicy.h"

namespace sc2
{

const char* ToString(const ECommandTaskRetentionPolicy CommandTaskRetentionPolicyValue)
{
    switch (CommandTaskRetentionPolicyValue)
    {
        case ECommandTaskRetentionPolicy::HotMustRun:
            return "HotMustRun";
        case ECommandTaskRetentionPolicy::BufferedRetry:
            return "BufferedRetry";
        case ECommandTaskRetentionPolicy::DiscardableDuplicate:
            return "DiscardableDuplicate";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
