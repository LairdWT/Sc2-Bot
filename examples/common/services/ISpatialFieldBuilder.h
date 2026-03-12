#pragma once

#include "common/agent_framework.h"
#include "common/spatial/FSpatialFieldSet.h"

namespace sc2
{

class ISpatialFieldBuilder
{
public:
    virtual ~ISpatialFieldBuilder();

    virtual void RebuildSpatialFieldSet(const FFrameContext& FrameContextValue,
                                        FSpatialFieldSet& SpatialFieldSetValue) const = 0;
};

}  // namespace sc2
