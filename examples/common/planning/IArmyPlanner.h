#pragma once

#include "common/armies/FArmyDomainState.h"
#include "common/descriptors/FGameStateDescriptor.h"

namespace sc2
{

class IArmyPlanner
{
public:
    virtual ~IArmyPlanner();

    virtual void ProduceArmyPlan(const FGameStateDescriptor& GameStateDescriptorValue,
                                 FArmyDomainState& ArmyDomainStateValue) const = 0;
};

}  // namespace sc2
