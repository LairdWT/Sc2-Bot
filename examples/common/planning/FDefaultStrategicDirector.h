#pragma once

#include <cstdint>

#include "common/armies/EArmyGoal.h"
#include "common/descriptors/EGamePlan.h"
#include "common/planning/IStrategicDirector.h"

namespace sc2
{

class FDefaultStrategicDirector final : public IStrategicDirector
{
public:
    void UpdateGameStateDescriptor(FGameStateDescriptor& GameStateDescriptorValue) const final;

private:
    EGamePlan DetermineGamePlan(const FGameStateDescriptor& GameStateDescriptorValue) const;
    uint32_t DetermineDesiredArmyCount(const FGameStateDescriptor& GameStateDescriptorValue) const;
    uint32_t DetermineDesiredBaseCount(const FGameStateDescriptor& GameStateDescriptorValue) const;
    EArmyGoal DeterminePrimaryArmyGoal(const FGameStateDescriptor& GameStateDescriptorValue) const;
};

}  // namespace sc2
