#include "common/planning/EIntentDomain.h"

#include <limits>

namespace sc2
{

int GetIntentDomainOrder(const EIntentDomain IntentDomainValue)
{
    switch (IntentDomainValue)
    {
        case EIntentDomain::Recovery:
            return 0;
        case EIntentDomain::StructureBuild:
            return 1;
        case EIntentDomain::StructureControl:
            return 2;
        case EIntentDomain::UnitProduction:
            return 3;
        case EIntentDomain::ArmyCombat:
            return 4;
        default:
            return std::numeric_limits<int>::max();
    }
}

const char* ToString(const EIntentDomain IntentDomainValue)
{
    switch (IntentDomainValue)
    {
        case EIntentDomain::Recovery:
            return "Recovery";
        case EIntentDomain::StructureBuild:
            return "StructureBuild";
        case EIntentDomain::StructureControl:
            return "StructureControl";
        case EIntentDomain::UnitProduction:
            return "UnitProduction";
        case EIntentDomain::ArmyCombat:
            return "ArmyCombat";
        default:
            return "Unknown";
    }
}

}  // namespace sc2
