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
        case EBuildPlacementSlotType::NaturalEntranceDepotLeft:
            return "NaturalEntranceDepotLeft";
        case EBuildPlacementSlotType::NaturalEntranceDepotRight:
            return "NaturalEntranceDepotRight";
        case EBuildPlacementSlotType::NaturalEntranceDepotCenter:
            return "NaturalEntranceDepotCenter";
        case EBuildPlacementSlotType::NaturalEntranceBunker:
            return "NaturalEntranceBunker";
        case EBuildPlacementSlotType::NaturalApproachDepot:
            return "NaturalApproachDepot";
        case EBuildPlacementSlotType::MainSupportDepot:
            return "MainSupportDepot";
        case EBuildPlacementSlotType::MainPeripheralDepot:
            return "MainPeripheralDepot";
        case EBuildPlacementSlotType::MainBarracksWithAddon:
            return "MainBarracksWithAddon";
        case EBuildPlacementSlotType::MainFactoryWithAddon:
            return "MainFactoryWithAddon";
        case EBuildPlacementSlotType::MainStarportWithAddon:
            return "MainStarportWithAddon";
        case EBuildPlacementSlotType::MainProductionWithAddon:
            return "MainProductionWithAddon";
        case EBuildPlacementSlotType::MainSupportStructure:
            return "MainSupportStructure";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
