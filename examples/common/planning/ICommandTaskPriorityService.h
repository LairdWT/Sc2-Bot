#pragma once

#include "common/descriptors/FGameStateDescriptor.h"

namespace sc2
{

class ICommandTaskPriorityService
{
public:
    virtual ~ICommandTaskPriorityService();

    virtual void UpdateTaskPriorities(FGameStateDescriptor& GameStateDescriptorValue) const = 0;
};

}  // namespace sc2
