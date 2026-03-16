#pragma once

#include <cstdint>

namespace sc2
{

enum class ETerranTaskTemplateId : uint16_t
{
    Invalid = 0,
    BuildSupplyDepot,
    BuildBarracks,
    BuildRefinery,
    TrainMarine,
    MorphOrbitalCommand,
    BuildCommandCenter,
    BuildBarracksReactor,
    BuildFactory,
    BuildStarport,
    TrainHellion,
    BuildFactoryTechLab,
    TrainMedivac,
    TrainCyclone,
    TrainLiberator,
    TrainSiegeTank,
    BuildEngineeringBay,
    ResearchStimpack,
    BuildStarportReactor,
    TrainMarauder,
    BuildFactoryReactor,
    ResearchCombatShield,
    ResearchTerranInfantryWeaponsLevel1,
    ResearchTerranInfantryArmorLevel1,
    TrainWidowMine,
    ResearchConcussiveShells,
    BuildBunker,
    BuildBarracksTechLab,
    TrainScv,
    TrainVikingFighter,
    Count
};

}  // namespace sc2
