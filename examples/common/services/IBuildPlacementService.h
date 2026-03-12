#pragma once

#include <vector>

#include "common/descriptors/FGameStateDescriptor.h"
#include "sc2api/sc2_common.h"
#include "sc2api/sc2_typeenums.h"

namespace sc2
{

class IBuildPlacementService
{
public:
    virtual ~IBuildPlacementService();

    virtual Point2D GetPrimaryStructureAnchor(const FGameStateDescriptor& GameStateDescriptorValue,
                                              const Point2D& BaseLocationValue) const = 0;
    virtual std::vector<Point2D> GetStructurePlacementCandidates(const FGameStateDescriptor& GameStateDescriptorValue,
                                                                 ABILITY_ID StructureAbilityId,
                                                                 const Point2D& BaseLocationValue) const = 0;
};

}  // namespace sc2
