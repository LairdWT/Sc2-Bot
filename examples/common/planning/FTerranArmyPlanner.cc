#include "common/planning/FTerranArmyPlanner.h"

#include "common/armies/EArmyGoal.h"
#include "common/armies/EArmyPosture.h"
#include "common/armies/FArmyDomainState.h"
#include "common/descriptors/EGamePlan.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/descriptors/FMacroStateDescriptor.h"

namespace sc2
{

void FTerranArmyPlanner::ProduceArmyPlan(const FGameStateDescriptor& GameStateDescriptorValue,
                                         FArmyDomainState& ArmyDomainStateValue) const
{
    ArmyDomainStateValue.PrimaryArmyAttackSupplyThreshold = 40U;
    ArmyDomainStateValue.PrimaryArmyDisengageSupplyThreshold = 20U;
    ArmyDomainStateValue.EnsurePrimaryArmyExists();

    if (!ArmyDomainStateValue.ArmyPostures.empty())
    {
        ArmyDomainStateValue.ArmyPostures.front() =
            DeterminePrimaryArmyPosture(GameStateDescriptorValue, ArmyDomainStateValue);
    }

    for (size_t ArmyIndexValue = 1U; ArmyIndexValue < ArmyDomainStateValue.ArmyPostures.size(); ++ArmyIndexValue)
    {
        ArmyDomainStateValue.ArmyPostures[ArmyIndexValue] = EArmyPosture::Hold;
    }
}

EArmyPosture FTerranArmyPlanner::DeterminePrimaryArmyPosture(
    const FGameStateDescriptor& GameStateDescriptorValue, const FArmyDomainState& ArmyDomainStateValue) const
{
    const FMacroStateDescriptor& MacroStateDescriptorValue = GameStateDescriptorValue.MacroState;

    switch (MacroStateDescriptorValue.ActiveGamePlan)
    {
        case EGamePlan::Recovery:
            return EArmyPosture::Hold;
        case EGamePlan::Macro:
            if (MacroStateDescriptorValue.ArmySupply >= 80U)
            {
                return EArmyPosture::Advance;
            }
            return EArmyPosture::Hold;
        case EGamePlan::TimingAttack:
            if (MacroStateDescriptorValue.ArmySupply >= ArmyDomainStateValue.PrimaryArmyAttackSupplyThreshold &&
                MacroStateDescriptorValue.ActiveBaseCount >= 2U)
            {
                return EArmyPosture::Engage;
            }
            return EArmyPosture::Assemble;
        case EGamePlan::Aggressive:
        case EGamePlan::AllIn:
            return EArmyPosture::Engage;
        case EGamePlan::Unknown:
        default:
            return EArmyPosture::Assemble;
    }
}

}  // namespace sc2
