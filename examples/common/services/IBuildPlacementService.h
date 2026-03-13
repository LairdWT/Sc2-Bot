#pragma once

#include <vector>

#include "common/descriptors/FGameStateDescriptor.h"
#include "common/services/FBuildPlacementContext.h"
#include "common/services/FBuildPlacementSlot.h"
#include "sc2api/sc2_common.h"
#include "sc2api/sc2_typeenums.h"

namespace sc2
{

class IBuildPlacementService
{
public:
    virtual ~IBuildPlacementService();

    virtual Point2D GetPrimaryStructureAnchor(const FGameStateDescriptor& GameStateDescriptorValue,
                                              const FBuildPlacementContext& BuildPlacementContextValue) const = 0;
    virtual Point2D GetArmyAssemblyPoint(const FGameStateDescriptor& GameStateDescriptorValue,
                                         const FBuildPlacementContext& BuildPlacementContextValue) const = 0;
    virtual std::vector<FBuildPlacementSlot> GetStructurePlacementSlots(
        const FGameStateDescriptor& GameStateDescriptorValue, ABILITY_ID StructureAbilityId,
        const FBuildPlacementContext& BuildPlacementContextValue) const = 0;
};

}  // namespace sc2
