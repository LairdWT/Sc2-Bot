#include "common/descriptors/EProductionFocus.h"

namespace sc2
{

const char* ToString(const EProductionFocus ProductionFocusValue)
{
    switch (ProductionFocusValue)
    {
        case EProductionFocus::Recovery:
            return "Recovery";
        case EProductionFocus::Supply:
            return "Supply";
        case EProductionFocus::Workers:
            return "Workers";
        case EProductionFocus::Expansion:
            return "Expansion";
        case EProductionFocus::Production:
            return "Production";
        case EProductionFocus::Army:
            return "Army";
        case EProductionFocus::Upgrades:
            return "Upgrades";
        case EProductionFocus::Defense:
            return "Defense";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
