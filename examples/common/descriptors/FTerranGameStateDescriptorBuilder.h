#pragma once

#include <cstdint>

#include "common/descriptors/IGameStateDescriptorBuilder.h"

namespace sc2
{

struct FAgentState;
struct FArmyDomainState;
struct FBuildPlanningState;
struct FMacroStateDescriptor;

class FTerranGameStateDescriptorBuilder final : public IGameStateDescriptorBuilder
{
public:
    void RebuildGameStateDescriptor(uint64_t CurrentStepValue, uint64_t CurrentGameLoopValue,
                                    const FAgentState& AgentStateValue,
                                    FGameStateDescriptor& GameStateDescriptorValue) const final;

private:
    void RebuildMacroStateDescriptor(uint64_t CurrentGameLoopValue, const FAgentState& AgentStateValue,
                                     FMacroStateDescriptor& MacroStateDescriptorValue) const;
    void RebuildArmyDomainState(const FAgentState& AgentStateValue, FArmyDomainState& ArmyDomainStateValue) const;
    void RebuildBuildPlanningState(uint64_t CurrentGameLoopValue, const FAgentState& AgentStateValue,
                                   FBuildPlanningState& BuildPlanningStateValue) const;
    EMacroPhase DetermineMacroPhase(const FMacroStateDescriptor& MacroStateDescriptorValue) const;
};

}  // namespace sc2
