#pragma once

#include <vector>

#include "common/agent_framework.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/services/FBuildPlacementContext.h"
#include "common/services/FBuildPlacementSlot.h"

namespace sc2
{

class IBuildPlacementService
{
public:
    virtual ~IBuildPlacementService();

    virtual FRampWallDescriptor GetRampWallDescriptor(const FFrameContext& FrameValue,
                                                      const FBuildPlacementContext& BuildPlacementContextValue) const = 0;
    virtual FMainBaseLayoutDescriptor GetMainBaseLayoutDescriptor(
        const FFrameContext& FrameValue, const FBuildPlacementContext& BuildPlacementContextValue) const = 0;
    virtual Point2D GetPrimaryStructureAnchor(const FGameStateDescriptor& GameStateDescriptorValue,
                                              const FBuildPlacementContext& BuildPlacementContextValue) const = 0;
    virtual Point2D GetArmyAssemblyPoint(const FGameStateDescriptor& GameStateDescriptorValue,
                                         const FBuildPlacementContext& BuildPlacementContextValue) const = 0;
    virtual Point2D GetProductionRallyPoint(const FGameStateDescriptor& GameStateDescriptorValue,
                                            const FBuildPlacementContext& BuildPlacementContextValue) const = 0;
    virtual std::vector<FBuildPlacementSlot> GetStructurePlacementSlots(
        const FGameStateDescriptor& GameStateDescriptorValue, ABILITY_ID StructureAbilityId,
        const FBuildPlacementContext& BuildPlacementContextValue) const = 0;
};

}  // namespace sc2
