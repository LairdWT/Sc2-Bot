#include "common/planning/ECommandAuthorityLayer.h"

namespace sc2
{

const char* ToString(const ECommandAuthorityLayer CommandAuthorityLayerValue)
{
    switch (CommandAuthorityLayerValue)
    {
        case ECommandAuthorityLayer::Agent:
            return "Agent";
        case ECommandAuthorityLayer::StrategicDirector:
            return "StrategicDirector";
        case ECommandAuthorityLayer::EconomyAndProduction:
            return "EconomyAndProduction";
        case ECommandAuthorityLayer::Army:
            return "Army";
        case ECommandAuthorityLayer::Squad:
            return "Squad";
        case ECommandAuthorityLayer::UnitExecution:
            return "UnitExecution";
        default:
            return "Agent";
    }
}

}  // namespace sc2
