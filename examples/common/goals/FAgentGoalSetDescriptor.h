#pragma once

#include <vector>

#include "common/goals/FGoalDescriptor.h"

namespace sc2
{

struct FAgentGoalSetDescriptor
{
public:
    FAgentGoalSetDescriptor();

    void Reset();

public:
    std::vector<FGoalDescriptor> ImmediateGoals;
    std::vector<FGoalDescriptor> NearTermGoals;
    std::vector<FGoalDescriptor> StrategicGoals;
};

}  // namespace sc2
