#include "common/planning/ECommandTaskOrigin.h"

namespace sc2
{

const char* ToString(const ECommandTaskOrigin CommandTaskOriginValue)
{
    switch (CommandTaskOriginValue)
    {
        case ECommandTaskOrigin::Opening:
            return "Opening";
        case ECommandTaskOrigin::GoalMacro:
            return "GoalMacro";
        case ECommandTaskOrigin::Recovery:
            return "Recovery";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
