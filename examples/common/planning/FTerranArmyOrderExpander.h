#pragma once

#include "common/planning/IArmyOrderExpander.h"

namespace sc2
{

class FTerranArmyOrderExpander final : public IArmyOrderExpander
{
public:
    void ExpandArmyOrders(const FFrameContext& FrameValue, const FAgentState& AgentStateValue,
                          FGameStateDescriptor& GameStateDescriptorValue,
                          const std::vector<Point2D>& ExpansionLocationsValue,
                          const Point2D& RallyPointValue,
                          FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue) const final;
};

}  // namespace sc2
