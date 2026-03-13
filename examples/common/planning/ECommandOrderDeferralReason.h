#pragma once

#include <cstdint>

namespace sc2
{

enum class ECommandOrderDeferralReason : uint8_t
{
    None,
    InsufficientResources,
    NoProducer,
    NoValidPlacement,
    ProducerBusy,
    TargetAlreadySatisfied,
    AwaitingObservedCompletion,
    ReservedSlotOccupied,
    ReservedSlotInvalidated,
};

const char* ToString(ECommandOrderDeferralReason CommandOrderDeferralReasonValue);

}  // namespace sc2
