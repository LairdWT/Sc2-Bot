#pragma once

#include <vector>

#include "common/agent_framework.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/services/IBuildPlacementService.h"

namespace sc2
{

struct FAgentState;

class IEconomyProductionOrderExpander
{
public:
    virtual ~IEconomyProductionOrderExpander();

    virtual void ExpandEconomyAndProductionOrders(const FFrameContext& FrameValue, const FAgentState& AgentStateValue,
                                                  FGameStateDescriptor& GameStateDescriptorValue,
                                                  FIntentBuffer& IntentBufferValue,
                                                  const IBuildPlacementService& BuildPlacementServiceValue,
                                                  const std::vector<Point2D>& ExpansionLocationsValue) const = 0;
};

}  // namespace sc2
