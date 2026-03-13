#include "common/planning/FDefaultStrategicDirector.h"

#include <algorithm>

#include "common/armies/EArmyGoal.h"
#include "common/armies/FArmyDomainState.h"
#include "common/descriptors/EGamePlan.h"
#include "common/descriptors/EMacroPhase.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/descriptors/FMacroStateDescriptor.h"

namespace sc2
{

void FDefaultStrategicDirector::UpdateGameStateDescriptor(FGameStateDescriptor& GameStateDescriptorValue) const
{
    FMacroStateDescriptor& MacroStateDescriptorValue = GameStateDescriptorValue.MacroState;
    FArmyDomainState& ArmyDomainStateValue = GameStateDescriptorValue.ArmyState;

    MacroStateDescriptorValue.ActiveGamePlan = DetermineGamePlan(GameStateDescriptorValue);
    MacroStateDescriptorValue.DesiredBaseCount = DetermineDesiredBaseCount(GameStateDescriptorValue);
    MacroStateDescriptorValue.DesiredArmyCount = DetermineDesiredArmyCount(GameStateDescriptorValue);

    ArmyDomainStateValue.MinimumArmyCount = std::max<uint32_t>(1U, MacroStateDescriptorValue.DesiredArmyCount);
    ArmyDomainStateValue.EnsurePrimaryArmyExists();

    if (!ArmyDomainStateValue.ArmyGoals.empty())
    {
        ArmyDomainStateValue.ArmyGoals.front() = DeterminePrimaryArmyGoal(GameStateDescriptorValue);
    }

    if (ArmyDomainStateValue.ArmyGoals.size() > 1U)
    {
        ArmyDomainStateValue.ArmyGoals[1] = EArmyGoal::HoldBase;
    }
}

EGamePlan FDefaultStrategicDirector::DetermineGamePlan(const FGameStateDescriptor& GameStateDescriptorValue) const
{
    const FMacroStateDescriptor& MacroStateDescriptorValue = GameStateDescriptorValue.MacroState;

    if (MacroStateDescriptorValue.ActiveMacroPhase == EMacroPhase::Recovery)
    {
        return EGamePlan::Recovery;
    }

    if (MacroStateDescriptorValue.ActiveBaseCount >= 3U)
    {
        return EGamePlan::Macro;
    }

    return EGamePlan::TimingAttack;
}

uint32_t FDefaultStrategicDirector::DetermineDesiredArmyCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    const FMacroStateDescriptor& MacroStateDescriptorValue = GameStateDescriptorValue.MacroState;
    if (MacroStateDescriptorValue.ActiveBaseCount >= 3U || MacroStateDescriptorValue.ArmySupply >= 100U)
    {
        return 2U;
    }

    return 1U;
}

uint32_t FDefaultStrategicDirector::DetermineDesiredBaseCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    const FMacroStateDescriptor& MacroStateDescriptorValue = GameStateDescriptorValue.MacroState;

    switch (MacroStateDescriptorValue.ActiveMacroPhase)
    {
        case EMacroPhase::Opening:
        case EMacroPhase::EarlyGame:
            return 2U;
        case EMacroPhase::MidGame:
            if (MacroStateDescriptorValue.ActiveBaseCount >= 3U && MacroStateDescriptorValue.ArmySupply >= 80U)
            {
                return 4U;
            }
            return 3U;
        case EMacroPhase::LateGame:
            return 4U;
        case EMacroPhase::Recovery:
            return std::max<uint32_t>(1U, MacroStateDescriptorValue.ActiveBaseCount);
        default:
            return std::max<uint32_t>(1U, MacroStateDescriptorValue.ActiveBaseCount);
    }
}

EArmyGoal FDefaultStrategicDirector::DeterminePrimaryArmyGoal(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    const FMacroStateDescriptor& MacroStateDescriptorValue = GameStateDescriptorValue.MacroState;

    switch (MacroStateDescriptorValue.ActiveGamePlan)
    {
        case EGamePlan::Recovery:
            return EArmyGoal::HoldBase;
        case EGamePlan::Macro:
            return EArmyGoal::MapControl;
        case EGamePlan::TimingAttack:
            if (MacroStateDescriptorValue.ActiveBaseCount >= 2U && MacroStateDescriptorValue.StarportCount > 0U &&
                MacroStateDescriptorValue.ArmySupply >= 40U)
            {
                return EArmyGoal::TimingAttack;
            }
            return EArmyGoal::HoldBase;
        case EGamePlan::Aggressive:
            return EArmyGoal::FrontalAssault;
        case EGamePlan::AllIn:
            return EArmyGoal::FrontalAssault;
        case EGamePlan::Unknown:
        default:
            return EArmyGoal::Unknown;
    }
}

}  // namespace sc2
