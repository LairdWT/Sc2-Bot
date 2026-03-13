#pragma once

#include "common/planning/IBuildPlanner.h"

namespace sc2
{

class FTerranTimingAttackBuildPlanner final : public IBuildPlanner
{
public:
    void ProduceBuildPlan(const FGameStateDescriptor& GameStateDescriptorValue,
                          FBuildPlanningState& BuildPlanningStateValue) const final;

private:
    void ProduceRecoveryBuildPlan(const FGameStateDescriptor& GameStateDescriptorValue,
                                  FBuildPlanningState& BuildPlanningStateValue) const;
    void ProduceFrameOpeningBuildPlan(const FGameStateDescriptor& GameStateDescriptorValue,
                                      FBuildPlanningState& BuildPlanningStateValue) const;
    void ProduceTimingAttackBuildPlan(const FGameStateDescriptor& GameStateDescriptorValue,
                                      FBuildPlanningState& BuildPlanningStateValue) const;
    void ProduceMacroBuildPlan(const FGameStateDescriptor& GameStateDescriptorValue,
                               FBuildPlanningState& BuildPlanningStateValue) const;
    uint32_t CountOutstandingNeeds(const FGameStateDescriptor& GameStateDescriptorValue,
                                   const FBuildPlanningState& BuildPlanningStateValue) const;
    static uint64_t ConvertClockTimeToGameLoops(uint32_t MinutesValue, uint32_t SecondsValue);
};

}  // namespace sc2
