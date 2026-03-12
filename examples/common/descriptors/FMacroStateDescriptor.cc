#include "common/descriptors/FMacroStateDescriptor.h"

namespace sc2
{

FMacroStateDescriptor::FMacroStateDescriptor()
{
    Reset();
}

void FMacroStateDescriptor::Reset()
{
    ActiveGamePlan = EGamePlan::Unknown;
    ActiveMacroPhase = EMacroPhase::Opening;
    DesiredBaseCount = 1;
    ActiveBaseCount = 0;
    DesiredArmyCount = 1;
}

}  // namespace sc2
