#include "common/services/EBuildPlacementSlotType.h"

namespace sc2
{

const char* ToString(const EBuildPlacementSlotType BuildPlacementSlotTypeValue)
{
    switch (BuildPlacementSlotTypeValue)
    {
        case EBuildPlacementSlotType::Unknown:
            return "Unknown";
        case EBuildPlacementSlotType::MainRampDepotLeft:
            return "MainRampDepotLeft";
        case EBuildPlacementSlotType::MainRampBarracksWithAddon:
            return "MainRampBarracksWithAddon";
        case EBuildPlacementSlotType::MainRampDepotRight:
            return "MainRampDepotRight";
        case EBuildPlacementSlotType::NaturalApproachDepot:
            return "NaturalApproachDepot";
        case EBuildPlacementSlotType::MainProductionWithAddon:
            return "MainProductionWithAddon";
        case EBuildPlacementSlotType::MainSupportStructure:
            return "MainSupportStructure";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
