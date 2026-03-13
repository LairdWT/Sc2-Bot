#pragma once

#include <cstdint>
#include <vector>

#include "common/services/FBuildPlacementSlotId.h"
#include "sc2api/sc2_gametypes.h"
#include "sc2api/sc2_typeenums.h"

namespace sc2
{

struct FOpeningPlanStep
{
public:
    FOpeningPlanStep();

    void Reset();

public:
    uint32_t StepId;
    uint64_t MinGameLoop;
    int PriorityValue;
    AbilityID AbilityId;
    UNIT_TYPEID ProducerUnitTypeId;
    UNIT_TYPEID ResultUnitTypeId;
    UpgradeID UpgradeId;
    uint32_t TargetCount;
    uint32_t ParallelGroupId;
    EBuildPlacementSlotType PreferredPlacementSlotType;
    FBuildPlacementSlotId PreferredPlacementSlotId;
    std::vector<uint32_t> RequiredCompletedStepIds;
};

}  // namespace sc2
