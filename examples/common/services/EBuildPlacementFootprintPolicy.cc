#include "common/services/EBuildPlacementFootprintPolicy.h"

namespace sc2
{

const char* ToString(const EBuildPlacementFootprintPolicy BuildPlacementFootprintPolicyValue)
{
    switch (BuildPlacementFootprintPolicyValue)
    {
        case EBuildPlacementFootprintPolicy::StructureOnly:
            return "StructureOnly";
        case EBuildPlacementFootprintPolicy::RequiresAddonClearance:
            return "RequiresAddonClearance";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
