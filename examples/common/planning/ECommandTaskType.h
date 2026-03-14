#pragma once

#include <cstdint>

namespace sc2
{

enum class ECommandTaskType : uint8_t
{
    Recovery,
    WorkerProduction,
    Supply,
    Expansion,
    Refinery,
    ProductionStructure,
    TechStructure,
    AddOn,
    UnitProduction,
    UpgradeResearch,
    StaticDefense,
    ArmyMission,
    Unknown,
};

const char* ToString(ECommandTaskType CommandTaskTypeValue);

}  // namespace sc2
