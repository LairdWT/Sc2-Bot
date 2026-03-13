#pragma once

#include <cstdint>

namespace sc2
{

enum class EBuildPlacementFootprintPolicy : uint8_t
{
    StructureOnly,
    RequiresAddonClearance,
};

const char* ToString(EBuildPlacementFootprintPolicy BuildPlacementFootprintPolicyValue);

}  // namespace sc2
