#pragma once

#include <cstdint>

#include "common/planning/ICommandTaskAdmissionService.h"

namespace sc2
{

class FTerranCommandTaskAdmissionService : public ICommandTaskAdmissionService
{
public:
    void RefreshStimulusState(FGameStateDescriptor& GameStateDescriptorValue) const final;
    void ReactivateBlockedTasks(FGameStateDescriptor& GameStateDescriptorValue) const final;
    bool TryAdmitOpeningTask(FGameStateDescriptor& GameStateDescriptorValue,
                             const FCommandTaskDescriptor& CommandTaskDescriptorValue,
                             uint32_t& OutOrderIdValue) const final;
    bool TryAdmitGoalDrivenOrder(FGameStateDescriptor& GameStateDescriptorValue,
                                 const FCommandOrderRecord& CommandOrderRecordValue,
                                 uint32_t& OutOrderIdValue) const final;
    void ParkDeferredOrders(FGameStateDescriptor& GameStateDescriptorValue) const final;
};

}  // namespace sc2
