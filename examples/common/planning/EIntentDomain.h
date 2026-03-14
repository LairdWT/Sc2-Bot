#pragma once

#include <cstddef>
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

constexpr size_t IntentDomainCountValue = 5U;

size_t GetIntentDomainIndex(EIntentDomain IntentDomainValue);
int GetIntentDomainOrder(EIntentDomain IntentDomainValue);
const char* ToString(EIntentDomain IntentDomainValue);

}  // namespace sc2
