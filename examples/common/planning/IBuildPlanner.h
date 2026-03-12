#pragma once

#include "common/build_planning/FBuildPlanningState.h"
#include "common/descriptors/FGameStateDescriptor.h"

namespace sc2
{

class IBuildPlanner
{
public:
    virtual ~IBuildPlanner();

    virtual void ProduceBuildPlan(const FGameStateDescriptor& GameStateDescriptorValue,
                                  FBuildPlanningState& BuildPlanningStateValue) const = 0;
};

}  // namespace sc2
