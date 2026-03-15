#pragma once

#include <cstdint>

namespace sc2
{

enum class EGoalTargetRuleId : uint8_t
{
    Invalid,
    None,
    DefaultTargetCount,
    DesiredWorkerCount,
    DesiredSupplyDepotCount,
    DesiredBaseCount,
    DesiredRefineryCount,
    DesiredBarracksCount,
    DesiredFactoryCount,
    DesiredStarportCount,
    DesiredMarineCount,
    DesiredMarauderCount,
    DesiredCycloneCount,
    DesiredSiegeTankCount,
    DesiredMedivacCount,
    DesiredLiberatorCount
};

}  // namespace sc2

