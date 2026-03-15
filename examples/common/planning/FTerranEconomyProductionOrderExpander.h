#pragma once

#include <unordered_map>

#include "common/planning/FCommandTaskSignatureKey.h"
#include "common/planning/FProductionBlockerResolution.h"
#include "common/planning/IEconomyProductionOrderExpander.h"

namespace sc2
{

class FTerranEconomyProductionOrderExpander final : public IEconomyProductionOrderExpander
{
public:
    void ExpandEconomyAndProductionOrders(const FFrameContext& FrameValue, const FAgentState& AgentStateValue,
                                          FGameStateDescriptor& GameStateDescriptorValue,
                                          FIntentBuffer& IntentBufferValue,
                                          const IBuildPlacementService& BuildPlacementServiceValue,
                                          const std::vector<Point2D>& ExpansionLocationsValue) const final;

private:
    mutable std::unordered_map<FCommandTaskSignatureKey, FProductionBlockerResolution, FCommandTaskSignatureKeyHash>
        ProductionBlockerResolutions;
};

}  // namespace sc2
