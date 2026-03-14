#pragma once

#include <vector>

#include "common/agent_framework.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/planning/FCommandAuthoritySchedulingState.h"

namespace sc2
{

struct FAgentState;

class IArmyOrderExpander
{
public:
    virtual ~IArmyOrderExpander();

    virtual void ExpandArmyOrders(const FFrameContext& FrameValue, const FAgentState& AgentStateValue,
                                  FGameStateDescriptor& GameStateDescriptorValue,
                                  const std::vector<Point2D>& ExpansionLocationsValue,
                                  const Point2D& RallyPointValue,
                                  FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue) const = 0;
};

}  // namespace sc2
