#pragma once

#include <cstdint>

namespace sc2
{

enum class EBuildPlacementSlotType : uint8_t
{
    Unknown,
    MainRampDepotLeft,
    MainRampBarracksWithAddon,
    MainRampDepotRight,
    NaturalApproachDepot,
    MainProductionWithAddon,
    MainSupportStructure,
};

const char* ToString(EBuildPlacementSlotType BuildPlacementSlotTypeValue);

}  // namespace sc2
