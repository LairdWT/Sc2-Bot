#include "common/planning/EPlanningProcessorState.h"

namespace sc2
{

const char* ToString(const EPlanningProcessorState PlanningProcessorStateValue)
{
    switch (PlanningProcessorStateValue)
    {
        case EPlanningProcessorState::Idle:
            return "Idle";
        case EPlanningProcessorState::Processing:
            return "Processing";
        case EPlanningProcessorState::ReadyToDrain:
            return "ReadyToDrain";
        default:
            return "Idle";
    }
}

}  // namespace sc2
