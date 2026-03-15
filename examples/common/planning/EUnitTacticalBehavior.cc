#include "common/planning/EUnitTacticalBehavior.h"

namespace sc2
{

const char* ToString(const EUnitTacticalBehavior UnitTacticalBehaviorValue)
{
    switch (UnitTacticalBehaviorValue)
    {
        case EUnitTacticalBehavior::AdvanceToMissionAnchor:
            return "AdvanceToMissionAnchor";
        case EUnitTacticalBehavior::AttackLocalThreat:
            return "AttackLocalThreat";
        case EUnitTacticalBehavior::SupportBioAnchor:
            return "SupportBioAnchor";
        case EUnitTacticalBehavior::HealBioSupportTarget:
            return "HealBioSupportTarget";
        case EUnitTacticalBehavior::BurrowDownForAmbush:
            return "BurrowDownForAmbush";
        case EUnitTacticalBehavior::BurrowUpForAdvance:
            return "BurrowUpForAdvance";
        case EUnitTacticalBehavior::SiegeForAnchor:
            return "SiegeForAnchor";
        case EUnitTacticalBehavior::UnsiegeForAdvance:
            return "UnsiegeForAdvance";
        case EUnitTacticalBehavior::RegroupToSquadAnchor:
            return "RegroupToSquadAnchor";
        case EUnitTacticalBehavior::RetreatToSafeAnchor:
            return "RetreatToSafeAnchor";
        case EUnitTacticalBehavior::HoldPosition:
            return "HoldPosition";
        case EUnitTacticalBehavior::ClearBlockingStructure:
            return "ClearBlockingStructure";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
