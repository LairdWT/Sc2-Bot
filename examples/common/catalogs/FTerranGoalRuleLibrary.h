#pragma once

#include <cstdint>

#include "common/descriptors/EGamePlan.h"
#include "common/descriptors/EProductionFocus.h"
#include "common/goals/EGoalStatus.h"

namespace sc2
{

struct FGameStateDescriptor;
struct FGoalDescriptor;
struct FTerranGoalDefinition;

class FTerranGoalRuleLibrary
{
public:
    static bool IsGoalActive(const FGoalDescriptor& GoalDescriptorValue);
    static EGoalStatus EvaluateGoalStatus(const FTerranGoalDefinition& TerranGoalDefinitionValue,
                                          const FGameStateDescriptor& GameStateDescriptorValue);
    static uint32_t EvaluateGoalTargetCount(const FTerranGoalDefinition& TerranGoalDefinitionValue,
                                            const FGameStateDescriptor& GameStateDescriptorValue);
    static uint32_t DetermineDesiredArmyCount(const FGameStateDescriptor& GameStateDescriptorValue);
    static uint32_t DetermineDesiredBaseCount(const FGameStateDescriptor& GameStateDescriptorValue);
    static uint32_t DetermineDesiredWorkerCount(const FGameStateDescriptor& GameStateDescriptorValue);
    static uint32_t DetermineDesiredRefineryCount(const FGameStateDescriptor& GameStateDescriptorValue);
    static uint32_t DetermineDesiredSupplyDepotCount(const FGameStateDescriptor& GameStateDescriptorValue);
    static uint32_t DetermineDesiredBarracksCount(const FGameStateDescriptor& GameStateDescriptorValue);
    static uint32_t DetermineDesiredFactoryCount(const FGameStateDescriptor& GameStateDescriptorValue);
    static uint32_t DetermineDesiredStarportCount(const FGameStateDescriptor& GameStateDescriptorValue);
    static uint32_t DetermineDesiredMarineCount(const FGameStateDescriptor& GameStateDescriptorValue);
    static uint32_t DetermineDesiredMarauderCount(const FGameStateDescriptor& GameStateDescriptorValue);
    static uint32_t DetermineDesiredCycloneCount(const FGameStateDescriptor& GameStateDescriptorValue);
    static uint32_t DetermineDesiredSiegeTankCount(const FGameStateDescriptor& GameStateDescriptorValue);
    static uint32_t DetermineDesiredMedivacCount(const FGameStateDescriptor& GameStateDescriptorValue);
    static uint32_t DetermineDesiredLiberatorCount(const FGameStateDescriptor& GameStateDescriptorValue);
    static bool ShouldPrioritizeUpgrades(const FGameStateDescriptor& GameStateDescriptorValue);
    static EProductionFocus DeterminePrimaryProductionFocus(const FGameStateDescriptor& GameStateDescriptorValue);
    static EGamePlan DetermineGamePlan(const FGameStateDescriptor& GameStateDescriptorValue);
};

}  // namespace sc2
