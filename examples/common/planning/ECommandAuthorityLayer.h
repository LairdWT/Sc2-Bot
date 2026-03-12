#pragma once

#include <cstdint>

namespace sc2
{

enum class ECommandAuthorityLayer : uint8_t
{
    Agent,
    StrategicDirector,
    EconomyAndProduction,
    Army,
    Squad,
    UnitExecution,
};

const char* ToString(const ECommandAuthorityLayer CommandAuthorityLayerValue);

}  // namespace sc2
