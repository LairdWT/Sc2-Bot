#pragma once

#include <vector>

#include "common/services/IBuildPlacementService.h"

namespace sc2
{

class FTerranBuildPlacementService : public IBuildPlacementService
{
public:
    FRampWallDescriptor GetRampWallDescriptor(const FFrameContext& FrameValue,
                                              const FBuildPlacementContext& BuildPlacementContextValue) const final;
    Point2D GetPrimaryStructureAnchor(const FGameStateDescriptor& GameStateDescriptorValue,
                                      const FBuildPlacementContext& BuildPlacementContextValue) const final;
    Point2D GetArmyAssemblyPoint(const FGameStateDescriptor& GameStateDescriptorValue,
                                 const FBuildPlacementContext& BuildPlacementContextValue) const final;
    std::vector<FBuildPlacementSlot> GetStructurePlacementSlots(
        const FGameStateDescriptor& GameStateDescriptorValue, ABILITY_ID StructureAbilityId,
        const FBuildPlacementContext& BuildPlacementContextValue) const final;
};

}  // namespace sc2
