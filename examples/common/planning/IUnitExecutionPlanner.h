#pragma once

#include <cstdint>

#include "common/agent_framework.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/planning/FCommandAuthoritySchedulingState.h"

namespace sc2
{

struct FAgentState;

class IUnitExecutionPlanner
{
public:
    virtual ~IUnitExecutionPlanner();

    virtual uint32_t ExpandUnitExecutionOrders(const FFrameContext& FrameValue, const FAgentState& AgentStateValue,
                                               const FGameStateDescriptor& GameStateDescriptorValue,
                                               const Point2D& RallyPointValue,
                                               FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue) const = 0;
};

}  // namespace sc2
