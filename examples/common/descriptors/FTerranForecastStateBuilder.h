#pragma once

#include "common/economy/FEconomyDomainState.h"

namespace sc2
{

struct FAgentState;
struct FGameStateDescriptor;

class FTerranForecastStateBuilder final
{
public:
    void RebuildForecastState(const FAgentState& AgentStateValue, FEconomyDomainState& EconomyDomainStateValue,
                              FGameStateDescriptor& GameStateDescriptorValue) const;
};

}  // namespace sc2
