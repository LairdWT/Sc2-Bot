#pragma once

#include <cstdint>

namespace sc2
{

enum class EIntentDomain : uint8_t
{
    Recovery,
    StructureBuild,
    StructureControl,
    UnitProduction,
    ArmyCombat,
};

int GetIntentDomainOrder(EIntentDomain IntentDomainValue);
const char* ToString(EIntentDomain IntentDomainValue);

}  // namespace sc2
