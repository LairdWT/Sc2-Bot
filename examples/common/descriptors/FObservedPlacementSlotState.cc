#include "common/descriptors/FObservedPlacementSlotState.h"

namespace sc2
{

namespace
{

int FindObservedPlacementSlotIndex(const std::vector<FBuildPlacementSlotId>& BuildPlacementSlotIdsValue,
                                   const FBuildPlacementSlotId& BuildPlacementSlotIdValue)
{
    for (size_t PlacementSlotIndexValue = 0U; PlacementSlotIndexValue < BuildPlacementSlotIdsValue.size();
         ++PlacementSlotIndexValue)
    {
        if (BuildPlacementSlotIdsValue[PlacementSlotIndexValue] == BuildPlacementSlotIdValue)
        {
            return static_cast<int>(PlacementSlotIndexValue);
        }
    }

    return -1;
}

}  // namespace

FObservedPlacementSlotState::FObservedPlacementSlotState()
{
    Reset();
}

void FObservedPlacementSlotState::Reset()
{
    ObservedPlacementSlotIds.clear();
    ObservedPlacementSlotStates.clear();
}

EObservedWallSlotState FObservedPlacementSlotState::GetObservedPlacementSlotState(
    const FBuildPlacementSlotId& BuildPlacementSlotIdValue) const
{
    const int PlacementSlotIndexValue =
        FindObservedPlacementSlotIndex(ObservedPlacementSlotIds, BuildPlacementSlotIdValue);
    if (PlacementSlotIndexValue < 0)
    {
        return EObservedWallSlotState::Unknown;
    }

    return ObservedPlacementSlotStates[static_cast<size_t>(PlacementSlotIndexValue)];
}

void FObservedPlacementSlotState::SetObservedPlacementSlotState(
    const FBuildPlacementSlotId& BuildPlacementSlotIdValue,
    const EObservedWallSlotState ObservedWallSlotStateValue)
{
    const int PlacementSlotIndexValue =
        FindObservedPlacementSlotIndex(ObservedPlacementSlotIds, BuildPlacementSlotIdValue);
    if (PlacementSlotIndexValue >= 0)
    {
        ObservedPlacementSlotStates[static_cast<size_t>(PlacementSlotIndexValue)] = ObservedWallSlotStateValue;
        return;
    }

    ObservedPlacementSlotIds.push_back(BuildPlacementSlotIdValue);
    ObservedPlacementSlotStates.push_back(ObservedWallSlotStateValue);
}

const std::vector<FBuildPlacementSlotId>& FObservedPlacementSlotState::GetObservedPlacementSlotIds() const
{
    return ObservedPlacementSlotIds;
}

const std::vector<EObservedWallSlotState>& FObservedPlacementSlotState::GetObservedPlacementSlotStates() const
{
    return ObservedPlacementSlotStates;
}

}  // namespace sc2
