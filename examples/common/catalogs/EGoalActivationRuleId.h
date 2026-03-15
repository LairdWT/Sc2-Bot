#pragma once

#include <cstdint>

namespace sc2
{

enum class EGoalActivationRuleId : uint8_t
{
    Invalid,
    AlwaysActive,
    ProjectedWorkersBelowTarget,
    SupplyPressureOrProjectedDepotsBelowTarget,
    ProjectedCommandCentersBelowTarget,
    ProjectedRefineriesBelowTarget,
    ProjectedBarracksBelowTarget,
    ProjectedFactoryBelowTarget,
    ProjectedStarportBelowTarget,
    MissingBarracksReactor,
    MissingFactoryTechLab,
    MissingEngineeringBayForUpgrades,
    MissingStarportReactor,
    MissingStimpack,
    MissingCombatShield,
    MissingInfantryWeaponsLevel1,
    MissingConcussiveShells,
    ProjectedMarinesBelowTarget,
    ProjectedMaraudersBelowTarget,
    ProjectedCyclonesBelowTarget,
    ProjectedSiegeTanksBelowTarget,
    ProjectedMedivacsBelowTarget,
    ProjectedLiberatorsBelowTarget
};

}  // namespace sc2

