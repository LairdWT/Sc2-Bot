#pragma once

#include "common/descriptors/EObservedWallSlotState.h"
#include "common/services/EBuildPlacementSlotType.h"

namespace sc2
{

struct FObservedRampWallState
{
public:
    FObservedRampWallState();

    void Reset();
    EObservedWallSlotState GetObservedWallSlotState(EBuildPlacementSlotType BuildPlacementSlotTypeValue) const;
    void SetObservedWallSlotState(EBuildPlacementSlotType BuildPlacementSlotTypeValue,
                                  EObservedWallSlotState ObservedWallSlotStateValue);

public:
    EObservedWallSlotState LeftDepotState;
    EObservedWallSlotState BarracksState;
    EObservedWallSlotState RightDepotState;
};

}  // namespace sc2
