#include "common/descriptors/EGamePlan.h"

namespace sc2
{

const char* ToString(const EGamePlan GamePlanValue)
{
    switch (GamePlanValue)
    {
        case EGamePlan::Unknown:
            return "Unknown";
        case EGamePlan::Macro:
            return "Macro";
        case EGamePlan::Aggressive:
            return "Aggressive";
        case EGamePlan::TimingAttack:
            return "TimingAttack";
        case EGamePlan::AllIn:
            return "AllIn";
        case EGamePlan::Recovery:
            return "Recovery";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
