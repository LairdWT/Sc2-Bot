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
    CurrentGameLoop = 0;
    DesiredBaseCount = 1;
    ActiveBaseCount = 0;
    DesiredArmyCount = 1;
    PrimaryProductionFocus = EProductionFocus::Recovery;
    WorkerCount = 0;
    ArmyUnitCount = 0;
    ArmySupply = 0;
    BarracksCount = 0;
    FactoryCount = 0;
    StarportCount = 0;
    SupplyUsed = 0;
    SupplyCap = 0;
}

}  // namespace sc2
