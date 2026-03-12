#pragma once

#include <cstdint>

#include "common/armies/FArmyDomainState.h"
#include "common/build_planning/FBuildPlanningState.h"
#include "common/descriptors/FMacroStateDescriptor.h"
#include "common/planning/FCommandAuthoritySchedulingState.h"
#include "common/spatial/FSpatialFieldSet.h"

namespace sc2
{

struct FGameStateDescriptor
{
    uint64_t CurrentStep;
    FMacroStateDescriptor MacroState;
    FArmyDomainState ArmyState;
    FBuildPlanningState BuildPlanning;
    FCommandAuthoritySchedulingState CommandAuthoritySchedulingState;
    FSpatialFieldSet SpatialFields;

    FGameStateDescriptor();

    void Reset();
};

}  // namespace sc2
