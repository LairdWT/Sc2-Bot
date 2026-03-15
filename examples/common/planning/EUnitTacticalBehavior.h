#pragma once

#include <cstdint>

namespace sc2
{

enum class EUnitTacticalBehavior : uint8_t
{
    AdvanceToMissionAnchor,
    AttackLocalThreat,
    SupportBioAnchor,
    HealBioSupportTarget,
    BurrowDownForAmbush,
    BurrowUpForAdvance,
    SiegeForAnchor,
    UnsiegeForAdvance,
    RegroupToSquadAnchor,
    RetreatToSafeAnchor,
    HoldPosition,
    ClearBlockingStructure,
};

const char* ToString(EUnitTacticalBehavior UnitTacticalBehaviorValue);

}  // namespace sc2
