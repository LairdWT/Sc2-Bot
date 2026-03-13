#pragma once

#include <vector>

#include "common/services/FBuildPlacementSlot.h"
#include "sc2api/sc2_common.h"

namespace sc2
{

struct FMainBaseLayoutDescriptor
{
public:
    FMainBaseLayoutDescriptor();

    void Reset();

public:
    bool bIsValid;
    Point2D LayoutAnchorPoint;
    std::vector<FBuildPlacementSlot> NaturalApproachDepotSlots;
    std::vector<FBuildPlacementSlot> SupportDepotSlots;
    std::vector<FBuildPlacementSlot> ProductionRailWithAddonSlots;
    std::vector<FBuildPlacementSlot> BarracksWithAddonSlots;
    std::vector<FBuildPlacementSlot> FactoryWithAddonSlots;
    std::vector<FBuildPlacementSlot> StarportWithAddonSlots;
};

}  // namespace sc2
