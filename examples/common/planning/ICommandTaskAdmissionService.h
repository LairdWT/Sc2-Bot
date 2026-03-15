#pragma once

#include <cstdint>

namespace sc2
{

struct FCommandTaskDescriptor;
struct FCommandOrderRecord;
struct FGameStateDescriptor;

class ICommandTaskAdmissionService
{
public:
    virtual ~ICommandTaskAdmissionService() = default;

    virtual void RefreshStimulusState(FGameStateDescriptor& GameStateDescriptorValue) const = 0;
    virtual void ReactivateBlockedTasks(FGameStateDescriptor& GameStateDescriptorValue) const = 0;
    virtual bool TryAdmitOpeningTask(FGameStateDescriptor& GameStateDescriptorValue,
                                     const FCommandTaskDescriptor& CommandTaskDescriptorValue,
                                     uint32_t& OutOrderIdValue) const = 0;
    virtual bool TryAdmitGoalDrivenOrder(FGameStateDescriptor& GameStateDescriptorValue,
                                         const FCommandOrderRecord& CommandOrderRecordValue,
                                         uint32_t& OutOrderIdValue) const = 0;
    virtual void ParkDeferredOrders(FGameStateDescriptor& GameStateDescriptorValue) const = 0;
};

}  // namespace sc2
