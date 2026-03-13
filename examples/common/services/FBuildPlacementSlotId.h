#pragma once

#include <cstdint>

#include "common/services/EBuildPlacementSlotType.h"

namespace sc2
{

struct FBuildPlacementSlotId
{
public:
    FBuildPlacementSlotId();

    void Reset();
    bool IsValid() const;

public:
    EBuildPlacementSlotType SlotType;
    uint8_t Ordinal;
};

bool operator==(const FBuildPlacementSlotId& LeftValue, const FBuildPlacementSlotId& RightValue);
bool operator!=(const FBuildPlacementSlotId& LeftValue, const FBuildPlacementSlotId& RightValue);

}  // namespace sc2
