#pragma once

#include "common/planning/IUnitExecutionPlanner.h"

namespace sc2
{

class FTerranArmyUnitExecutionPlanner final : public IUnitExecutionPlanner
{
public:
    uint32_t ExpandUnitExecutionOrders(const FFrameContext& FrameValue, const FAgentState& AgentStateValue,
                                       const FGameStateDescriptor& GameStateDescriptorValue,
                                       const Point2D& RallyPointValue,
                                       FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue) const final;
};

}  // namespace sc2
