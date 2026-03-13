#include "common/armies/EArmyPosture.h"

namespace sc2
{

const char* ToString(const EArmyPosture ArmyPostureValue)
{
    switch (ArmyPostureValue)
    {
        case EArmyPosture::Unknown:
            return "Unknown";
        case EArmyPosture::Assemble:
            return "Assemble";
        case EArmyPosture::Hold:
            return "Hold";
        case EArmyPosture::Advance:
            return "Advance";
        case EArmyPosture::Engage:
            return "Engage";
        case EArmyPosture::Regroup:
            return "Regroup";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
