#include "common/planning/ECommandOrderDeferralReason.h"

namespace sc2
{

const char* ToString(const ECommandOrderDeferralReason CommandOrderDeferralReasonValue)
{
    switch (CommandOrderDeferralReasonValue)
    {
        case ECommandOrderDeferralReason::None:
            return "None";
        case ECommandOrderDeferralReason::InsufficientResources:
            return "InsufficientResources";
        case ECommandOrderDeferralReason::NoProducer:
            return "NoProducer";
        case ECommandOrderDeferralReason::NoValidPlacement:
            return "NoValidPlacement";
        case ECommandOrderDeferralReason::ProducerBusy:
            return "ProducerBusy";
        case ECommandOrderDeferralReason::TargetAlreadySatisfied:
            return "TargetAlreadySatisfied";
        case ECommandOrderDeferralReason::AwaitingObservedCompletion:
            return "AwaitingObservedCompletion";
        case ECommandOrderDeferralReason::ReservedSlotOccupied:
            return "ReservedSlotOccupied";
        case ECommandOrderDeferralReason::ReservedSlotInvalidated:
            return "ReservedSlotInvalidated";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
