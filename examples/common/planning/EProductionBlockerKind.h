#pragma once

#include <cstdint>

namespace sc2
{

enum class EProductionBlockerKind : uint8_t
{
    None,
    FriendlyMovableUnit,
    FriendlyStructure,
    EnemyUnit,
    EnemyStructure,
    Terrain,
    ResourceNode,
};

const char* ToString(EProductionBlockerKind ProductionBlockerKindValue);

}  // namespace sc2
