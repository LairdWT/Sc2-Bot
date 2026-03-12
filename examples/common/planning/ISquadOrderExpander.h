#pragma once

#include "common/descriptors/FGameStateDescriptor.h"
#include "common/planning/FCommandAuthoritySchedulingState.h"

namespace sc2
{

class ISquadOrderExpander
{
public:
    virtual ~ISquadOrderExpander();

    virtual void ExpandSquadOrders(const FGameStateDescriptor& GameStateDescriptorValue,
                                   FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue) const = 0;
};

}  // namespace sc2
