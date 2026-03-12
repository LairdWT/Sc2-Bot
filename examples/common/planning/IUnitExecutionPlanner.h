#pragma once

#include "common/descriptors/FGameStateDescriptor.h"
#include "common/planning/FCommandAuthoritySchedulingState.h"

namespace sc2
{

class IUnitExecutionPlanner
{
public:
    virtual ~IUnitExecutionPlanner();

    virtual void ExpandUnitExecutionOrders(const FGameStateDescriptor& GameStateDescriptorValue,
                                           FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue) const = 0;
};

}  // namespace sc2
