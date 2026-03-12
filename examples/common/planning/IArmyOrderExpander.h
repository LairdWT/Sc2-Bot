#pragma once

#include "common/descriptors/FGameStateDescriptor.h"
#include "common/planning/FCommandAuthoritySchedulingState.h"

namespace sc2
{

class IArmyOrderExpander
{
public:
    virtual ~IArmyOrderExpander();

    virtual void ExpandArmyOrders(const FGameStateDescriptor& GameStateDescriptorValue,
                                  FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue) const = 0;
};

}  // namespace sc2
