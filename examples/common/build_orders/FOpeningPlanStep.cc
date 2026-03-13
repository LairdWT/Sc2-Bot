#include "common/build_orders/FOpeningPlanStep.h"

namespace sc2
{

FOpeningPlanStep::FOpeningPlanStep()
{
    Reset();
}

void FOpeningPlanStep::Reset()
{
    StepId = 0U;
    MinGameLoop = 0U;
    PriorityValue = 0;
    AbilityId = ABILITY_ID::INVALID;
    ProducerUnitTypeId = UNIT_TYPEID::INVALID;
    ResultUnitTypeId = UNIT_TYPEID::INVALID;
    UpgradeId = UpgradeID(UPGRADE_ID::INVALID);
    TargetCount = 0U;
    RequestedQueueCount = 1U;
    ParallelGroupId = 0U;
    PreferredPlacementSlotType = EBuildPlacementSlotType::Unknown;
    PreferredPlacementSlotId.Reset();
    RequiredCompletedStepIds.clear();
}

}  // namespace sc2
