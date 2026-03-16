#pragma once

#include <vector>

#include "common/descriptors/EObservedWallSlotState.h"
#include "common/services/FBuildPlacementSlotId.h"

namespace sc2
{

struct FObservedPlacementSlotState
{
public:
    FObservedPlacementSlotState();

    void Reset();
    EObservedWallSlotState GetObservedPlacementSlotState(
        const FBuildPlacementSlotId& BuildPlacementSlotIdValue) const;
    void SetObservedPlacementSlotState(const FBuildPlacementSlotId& BuildPlacementSlotIdValue,
                                       EObservedWallSlotState ObservedWallSlotStateValue);
    const std::vector<FBuildPlacementSlotId>& GetObservedPlacementSlotIds() const;
    const std::vector<EObservedWallSlotState>& GetObservedPlacementSlotStates() const;

public:
    std::vector<FBuildPlacementSlotId> ObservedPlacementSlotIds;
    std::vector<EObservedWallSlotState> ObservedPlacementSlotStates;
};

}  // namespace sc2
