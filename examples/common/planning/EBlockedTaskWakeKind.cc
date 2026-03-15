#include "common/planning/EBlockedTaskWakeKind.h"

namespace sc2
{

const char* ToString(const EBlockedTaskWakeKind BlockedTaskWakeKindValue)
{
    switch (BlockedTaskWakeKindValue)
    {
        case EBlockedTaskWakeKind::ProducerAvailability:
            return "ProducerAvailability";
        case EBlockedTaskWakeKind::Resources:
            return "Resources";
        case EBlockedTaskWakeKind::Placement:
            return "Placement";
        case EBlockedTaskWakeKind::GoalRevision:
            return "GoalRevision";
        case EBlockedTaskWakeKind::CooldownOnly:
            return "CooldownOnly";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
