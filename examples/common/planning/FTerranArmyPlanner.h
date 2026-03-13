#pragma once

#include "common/planning/IArmyPlanner.h"

namespace sc2
{

class FTerranArmyPlanner final : public IArmyPlanner
{
public:
    void ProduceArmyPlan(const FGameStateDescriptor& GameStateDescriptorValue,
                         FArmyDomainState& ArmyDomainStateValue) const final;

private:
    EArmyPosture DeterminePrimaryArmyPosture(const FGameStateDescriptor& GameStateDescriptorValue,
                                             const FArmyDomainState& ArmyDomainStateValue) const;
};

}  // namespace sc2
