#pragma once

#include "common/descriptors/EProductionFocus.h"
#include "common/planning/ECommandPriorityTier.h"
#include "common/planning/ECommandTaskType.h"
#include "common/planning/FCommandOrderRecord.h"
#include "common/planning/ICommandTaskPriorityService.h"

namespace sc2
{

class FTerranCommandTaskPriorityService final : public ICommandTaskPriorityService
{
public:
    void UpdateTaskPriorities(FGameStateDescriptor& GameStateDescriptorValue) const final;

private:
    int GetTaskTypeWeight(ECommandTaskType CommandTaskTypeValue) const;
    int GetFocusWeight(EProductionFocus ProductionFocusValue, ECommandTaskType CommandTaskTypeValue) const;
    int GetEmergencyWeight(const FGameStateDescriptor& GameStateDescriptorValue,
                           const FCommandOrderRecord& CommandOrderRecordValue) const;
    ECommandPriorityTier DeterminePriorityTier(const FGameStateDescriptor& GameStateDescriptorValue,
                                               const FCommandOrderRecord& CommandOrderRecordValue,
                                               int EffectivePriorityValue) const;
};

}  // namespace sc2
