#pragma once

#include <cstdint>

#include "common/armies/FArmyDomainState.h"
#include "common/build_orders/FOpeningPlanExecutionState.h"
#include "common/build_planning/FBuildPlanningState.h"
#include "common/descriptors/FObservedRampWallState.h"
#include "common/descriptors/FMacroStateDescriptor.h"
#include "common/planning/FCommandAuthoritySchedulingState.h"
#include "common/services/FRampWallDescriptor.h"
#include "common/spatial/FSpatialFieldSet.h"

namespace sc2
{

struct FGameStateDescriptor
{
    uint64_t CurrentStep;
    uint64_t CurrentGameLoop;
    FMacroStateDescriptor MacroState;
    FArmyDomainState ArmyState;
    FBuildPlanningState BuildPlanning;
    FOpeningPlanExecutionState OpeningPlanExecutionState;
    FCommandAuthoritySchedulingState CommandAuthoritySchedulingState;
    FSpatialFieldSet SpatialFields;
    FRampWallDescriptor RampWallDescriptor;
    FObservedRampWallState ObservedRampWallState;

    FGameStateDescriptor();

    void Reset();
};

}  // namespace sc2
