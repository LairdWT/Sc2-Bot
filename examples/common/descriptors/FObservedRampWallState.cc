#include "common/descriptors/FObservedRampWallState.h"

namespace sc2
{

FObservedRampWallState::FObservedRampWallState()
{
    Reset();
}

void FObservedRampWallState::Reset()
{
    LeftDepotState = EObservedWallSlotState::Unknown;
    BarracksState = EObservedWallSlotState::Unknown;
    RightDepotState = EObservedWallSlotState::Unknown;
}

EObservedWallSlotState FObservedRampWallState::GetObservedWallSlotState(
    const EBuildPlacementSlotType BuildPlacementSlotTypeValue) const
{
    switch (BuildPlacementSlotTypeValue)
    {
        case EBuildPlacementSlotType::MainRampDepotLeft:
            return LeftDepotState;
        case EBuildPlacementSlotType::MainRampBarracksWithAddon:
            return BarracksState;
        case EBuildPlacementSlotType::MainRampDepotRight:
            return RightDepotState;
        default:
            return EObservedWallSlotState::Unknown;
    }
}

void FObservedRampWallState::SetObservedWallSlotState(const EBuildPlacementSlotType BuildPlacementSlotTypeValue,
                                                      const EObservedWallSlotState ObservedWallSlotStateValue)
{
    switch (BuildPlacementSlotTypeValue)
    {
        case EBuildPlacementSlotType::MainRampDepotLeft:
            LeftDepotState = ObservedWallSlotStateValue;
            return;
        case EBuildPlacementSlotType::MainRampBarracksWithAddon:
            BarracksState = ObservedWallSlotStateValue;
            return;
        case EBuildPlacementSlotType::MainRampDepotRight:
            RightDepotState = ObservedWallSlotStateValue;
            return;
        default:
            return;
    }
}

}  // namespace sc2
