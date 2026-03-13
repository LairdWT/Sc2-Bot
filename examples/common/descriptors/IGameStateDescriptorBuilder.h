#pragma once

#include <cstdint>

#include "common/descriptors/FGameStateDescriptor.h"

namespace sc2
{

struct FAgentState;

class IGameStateDescriptorBuilder
{
public:
    virtual ~IGameStateDescriptorBuilder();

    virtual void RebuildGameStateDescriptor(uint64_t CurrentStepValue, uint64_t CurrentGameLoopValue,
                                            const FAgentState& AgentStateValue,
                                            FGameStateDescriptor& GameStateDescriptorValue) const = 0;
};

}  // namespace sc2
