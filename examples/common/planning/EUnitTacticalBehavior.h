#pragma once

#include <cstdint>

namespace sc2
{

enum class EUnitTacticalBehavior : uint8_t
{
    AdvanceToMissionAnchor,
    AttackLocalThreat,
    RegroupToSquadAnchor,
    RetreatToSafeAnchor,
    HoldPosition,
    ClearBlockingStructure,
};

const char* ToString(EUnitTacticalBehavior UnitTacticalBehaviorValue);

}  // namespace sc2
