#include "common/planning/ECommandTaskType.h"

namespace sc2
{

const char* ToString(const ECommandTaskType CommandTaskTypeValue)
{
    switch (CommandTaskTypeValue)
    {
        case ECommandTaskType::Recovery:
            return "Recovery";
        case ECommandTaskType::WorkerProduction:
            return "WorkerProduction";
        case ECommandTaskType::Supply:
            return "Supply";
        case ECommandTaskType::Expansion:
            return "Expansion";
        case ECommandTaskType::Refinery:
            return "Refinery";
        case ECommandTaskType::ProductionStructure:
            return "ProductionStructure";
        case ECommandTaskType::TechStructure:
            return "TechStructure";
        case ECommandTaskType::AddOn:
            return "AddOn";
        case ECommandTaskType::UnitProduction:
            return "UnitProduction";
        case ECommandTaskType::UpgradeResearch:
            return "UpgradeResearch";
        case ECommandTaskType::StaticDefense:
            return "StaticDefense";
        case ECommandTaskType::ArmyMission:
            return "ArmyMission";
        case ECommandTaskType::Unknown:
        default:
            return "Unknown";
    }
}

}  // namespace sc2
