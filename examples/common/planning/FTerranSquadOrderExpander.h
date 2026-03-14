#pragma once

#include "common/planning/ISquadOrderExpander.h"

namespace sc2
{

class FTerranSquadOrderExpander final : public ISquadOrderExpander
{
public:
    void ExpandSquadOrders(const FFrameContext& FrameValue, const FAgentState& AgentStateValue,
                           const FGameStateDescriptor& GameStateDescriptorValue, const Point2D& RallyPointValue,
                           FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue) const final;
};

}  // namespace sc2
