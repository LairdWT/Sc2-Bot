#pragma once

#include "common/descriptors/FGameStateDescriptor.h"

namespace sc2
{

class IStrategicDirector
{
public:
    virtual ~IStrategicDirector();

    virtual void UpdateGameStateDescriptor(FGameStateDescriptor& GameStateDescriptorValue) const = 0;
};

}  // namespace sc2
