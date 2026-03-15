#pragma once

#include <unordered_map>

#include "common/planning/FUnitExecutionCacheEntry.h"
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

private:
    mutable std::unordered_map<Tag, FUnitExecutionCacheEntry> UnitExecutionCacheEntries;
};

}  // namespace sc2
