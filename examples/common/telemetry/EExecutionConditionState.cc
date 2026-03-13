#include "common/telemetry/EExecutionConditionState.h"

namespace sc2
{

const char* ToString(const EExecutionConditionState ExecutionConditionStateValue)
{
    switch (ExecutionConditionStateValue)
    {
        case EExecutionConditionState::Inactive:
            return "Inactive";
        case EExecutionConditionState::Active:
            return "Active";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
