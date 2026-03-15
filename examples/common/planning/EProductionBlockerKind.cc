#include "common/planning/EProductionBlockerKind.h"

namespace sc2
{

const char* ToString(const EProductionBlockerKind ProductionBlockerKindValue)
{
    switch (ProductionBlockerKindValue)
    {
        case EProductionBlockerKind::None:
            return "None";
        case EProductionBlockerKind::FriendlyMovableUnit:
            return "FriendlyMovableUnit";
        case EProductionBlockerKind::FriendlyStructure:
            return "FriendlyStructure";
        case EProductionBlockerKind::EnemyUnit:
            return "EnemyUnit";
        case EProductionBlockerKind::EnemyStructure:
            return "EnemyStructure";
        case EProductionBlockerKind::Terrain:
            return "Terrain";
        case EProductionBlockerKind::ResourceNode:
            return "ResourceNode";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
