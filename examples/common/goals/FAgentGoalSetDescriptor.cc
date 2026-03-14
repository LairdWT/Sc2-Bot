#include "common/goals/FAgentGoalSetDescriptor.h"

namespace sc2
{

FAgentGoalSetDescriptor::FAgentGoalSetDescriptor()
{
    Reset();
}

void FAgentGoalSetDescriptor::Reset()
{
    ImmediateGoals.clear();
    NearTermGoals.clear();
    StrategicGoals.clear();
}

}  // namespace sc2
