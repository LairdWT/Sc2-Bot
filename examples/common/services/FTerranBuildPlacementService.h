#pragma once

#include <vector>

#include "common/services/IBuildPlacementService.h"

namespace sc2
{

class FTerranBuildPlacementService : public IBuildPlacementService
{
public:
    Point2D GetPrimaryStructureAnchor(const FGameStateDescriptor& GameStateDescriptorValue,
                                      const Point2D& BaseLocationValue) const final;
    std::vector<Point2D> GetStructurePlacementCandidates(const FGameStateDescriptor& GameStateDescriptorValue,
                                                         ABILITY_ID StructureAbilityId,
                                                         const Point2D& BaseLocationValue) const final;
};

}  // namespace sc2
