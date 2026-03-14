#pragma once

#include <cstdint>
#include <vector>

#include "common/armies/EArmyGoal.h"
#include "common/descriptors/EGamePlan.h"
#include "common/descriptors/EProductionFocus.h"
#include "common/goals/FGoalDescriptor.h"
#include "common/planning/IStrategicDirector.h"

namespace sc2
{

class FDefaultStrategicDirector final : public IStrategicDirector
{
public:
    void UpdateGameStateDescriptor(FGameStateDescriptor& GameStateDescriptorValue) const final;

private:
    void RebuildGoalSet(FGameStateDescriptor& GameStateDescriptorValue) const;
    void RebuildArmyGoals(FGameStateDescriptor& GameStateDescriptorValue) const;
    void AppendImmediateGoals(const FGameStateDescriptor& GameStateDescriptorValue,
                              std::vector<FGoalDescriptor>& GoalDescriptorsValue) const;
    void AppendNearTermGoals(const FGameStateDescriptor& GameStateDescriptorValue,
                             std::vector<FGoalDescriptor>& GoalDescriptorsValue) const;
    void AppendStrategicGoals(const FGameStateDescriptor& GameStateDescriptorValue,
                              std::vector<FGoalDescriptor>& GoalDescriptorsValue) const;
    EProductionFocus DeterminePrimaryProductionFocus(const FGameStateDescriptor& GameStateDescriptorValue) const;
    EGamePlan DetermineGamePlan(const FGameStateDescriptor& GameStateDescriptorValue) const;
    uint32_t DetermineDesiredArmyCount(const FGameStateDescriptor& GameStateDescriptorValue) const;
    uint32_t DetermineDesiredBaseCount(const FGameStateDescriptor& GameStateDescriptorValue) const;
    uint32_t DetermineDesiredWorkerCount(const FGameStateDescriptor& GameStateDescriptorValue) const;
    uint32_t DetermineDesiredRefineryCount(const FGameStateDescriptor& GameStateDescriptorValue) const;
    uint32_t DetermineDesiredSupplyDepotCount(const FGameStateDescriptor& GameStateDescriptorValue) const;
    uint32_t DetermineDesiredBarracksCount(const FGameStateDescriptor& GameStateDescriptorValue) const;
    uint32_t DetermineDesiredFactoryCount(const FGameStateDescriptor& GameStateDescriptorValue) const;
    uint32_t DetermineDesiredStarportCount(const FGameStateDescriptor& GameStateDescriptorValue) const;
    uint32_t DetermineDesiredMarineCount(const FGameStateDescriptor& GameStateDescriptorValue) const;
    uint32_t DetermineDesiredMarauderCount(const FGameStateDescriptor& GameStateDescriptorValue) const;
    uint32_t DetermineDesiredCycloneCount(const FGameStateDescriptor& GameStateDescriptorValue) const;
    uint32_t DetermineDesiredSiegeTankCount(const FGameStateDescriptor& GameStateDescriptorValue) const;
    uint32_t DetermineDesiredMedivacCount(const FGameStateDescriptor& GameStateDescriptorValue) const;
    uint32_t DetermineDesiredLiberatorCount(const FGameStateDescriptor& GameStateDescriptorValue) const;
    bool ShouldPrioritizeUpgrades(const FGameStateDescriptor& GameStateDescriptorValue) const;
    EArmyGoal DeterminePrimaryArmyGoal(const FGameStateDescriptor& GameStateDescriptorValue) const;
};

}  // namespace sc2
