#pragma once

#include <cstdint>

#include "common/armies/EArmyMissionType.h"
#include "sc2api/sc2_common.h"

namespace sc2
{

struct FArmyMissionDescriptor
{
public:
    FArmyMissionDescriptor();

    void Reset();

public:
    EArmyMissionType MissionType;
    uint32_t SourceGoalId;
    Point2D ObjectivePoint;
    float ObjectiveRadius;
    uint32_t SearchExpansionOrdinal;
};

}  // namespace sc2
