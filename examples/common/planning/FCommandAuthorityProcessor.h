#pragma once

#include "common/build_orders/FOpeningPlanStep.h"
#include "common/descriptors/FGameStateDescriptor.h"

namespace sc2
{

class FCommandAuthorityProcessor
{
public:
    void ProcessSchedulerStep(FGameStateDescriptor& GameStateDescriptorValue) const;

private:
    void InitializeOpeningPlan(FGameStateDescriptor& GameStateDescriptorValue) const;
    void UpdateCompletedOpeningSteps(FGameStateDescriptor& GameStateDescriptorValue) const;
    void SeedReadyStrategicOrders(FGameStateDescriptor& GameStateDescriptorValue) const;
    void SeedGoalDrivenStrategicOrders(FGameStateDescriptor& GameStateDescriptorValue) const;
    void EnsureStrategicChildOrders(FGameStateDescriptor& GameStateDescriptorValue) const;
    bool AreRequiredTasksCompleted(const FOpeningPlanExecutionState& OpeningPlanExecutionStateValue,
                                   const FCommandTaskDescriptor& CommandTaskDescriptorValue) const;
    bool DoesOrderTargetMatchObservedState(const FGameStateDescriptor& GameStateDescriptorValue,
                                           const FCommandOrderRecord& CommandOrderRecordValue) const;
    uint32_t GetObservedCountForOrder(const FBuildPlanningState& BuildPlanningStateValue,
                                      const FCommandOrderRecord& CommandOrderRecordValue) const;
};

}  // namespace sc2
