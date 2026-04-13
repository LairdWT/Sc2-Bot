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
    MissingBarracksTechLab,
    MissingFactoryTechLab,
    MissingEngineeringBayForUpgrades,
    MissingSecondEngineeringBayForUpgrades,
    MissingStarportReactor,
    MissingStimpack,
    MissingCombatShield,
    MissingInfantryWeaponsLevel1,
    MissingInfantryArmorLevel1,
    MissingConcussiveShells,
    ProjectedMarinesBelowTarget,
    ProjectedMaraudersBelowTarget,
    ProjectedCyclonesBelowTarget,
    ProjectedSiegeTanksBelowTarget,
    ProjectedMedivacsBelowTarget,
    ProjectedLiberatorsBelowTarget,
    ProjectedOrbitalsBelowDesiredCount
};

}  // namespace sc2
