#include "common/planning/FDefaultStrategicDirector.h"

#include <algorithm>

#include "common/armies/EArmyGoal.h"
#include "common/catalogs/FTerranGoalDefinition.h"
#include "common/catalogs/FTerranGoalDictionary.h"
#include "common/catalogs/FTerranGoalRuleLibrary.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/goals/FAgentGoalSetDescriptor.h"

namespace sc2
{
namespace
{

FGoalDescriptor CreateGoalDescriptorFromDefinition(const FTerranGoalDefinition& TerranGoalDefinitionValue,
                                                   const FGameStateDescriptor& GameStateDescriptorValue)
{
    FGoalDescriptor GoalDescriptorValue;
    GoalDescriptorValue.GoalId = TerranGoalDefinitionValue.GoalId;
    GoalDescriptorValue.GoalDomain = TerranGoalDefinitionValue.GoalDomain;
    GoalDescriptorValue.GoalHorizon = TerranGoalDefinitionValue.GoalHorizon;
    GoalDescriptorValue.GoalType = TerranGoalDefinitionValue.GoalType;
    GoalDescriptorValue.GoalStatus =
        FTerranGoalRuleLibrary::EvaluateGoalStatus(TerranGoalDefinitionValue, GameStateDescriptorValue);
    GoalDescriptorValue.BasePriorityValue = TerranGoalDefinitionValue.BasePriorityValue;
    GoalDescriptorValue.TargetCount =
        FTerranGoalRuleLibrary::EvaluateGoalTargetCount(TerranGoalDefinitionValue, GameStateDescriptorValue);
    GoalDescriptorValue.TargetUnitTypeId = TerranGoalDefinitionValue.DefaultTargetUnitTypeId;
    GoalDescriptorValue.TargetUpgradeId = TerranGoalDefinitionValue.DefaultTargetUpgradeId;
    return GoalDescriptorValue;
}

void AppendGoalsForHorizon(const FGameStateDescriptor& GameStateDescriptorValue, const EGoalHorizon GoalHorizonValue,
                           std::vector<FGoalDescriptor>& GoalDescriptorsValue)
{
    const size_t DefinitionCountValue = FTerranGoalDictionary::GetDefinitionCount();
    for (size_t DefinitionIndexValue = 0U; DefinitionIndexValue < DefinitionCountValue; ++DefinitionIndexValue)
    {
        const FTerranGoalDefinition& TerranGoalDefinitionValue =
            FTerranGoalDictionary::GetDefinitionByIndex(DefinitionIndexValue);
        if (TerranGoalDefinitionValue.GoalHorizon != GoalHorizonValue)
        {
            continue;
        }

        GoalDescriptorsValue.push_back(
            CreateGoalDescriptorFromDefinition(TerranGoalDefinitionValue, GameStateDescriptorValue));
    }
}

}  // namespace

void FDefaultStrategicDirector::UpdateGameStateDescriptor(FGameStateDescriptor& GameStateDescriptorValue) const
{
    FMacroStateDescriptor& MacroStateDescriptorValue = GameStateDescriptorValue.MacroState;
    FArmyDomainState& ArmyDomainStateValue = GameStateDescriptorValue.ArmyState;

    MacroStateDescriptorValue.ActiveGamePlan = DetermineGamePlan(GameStateDescriptorValue);
    MacroStateDescriptorValue.DesiredBaseCount = DetermineDesiredBaseCount(GameStateDescriptorValue);
    MacroStateDescriptorValue.DesiredArmyCount = DetermineDesiredArmyCount(GameStateDescriptorValue);
    MacroStateDescriptorValue.PrimaryProductionFocus = DeterminePrimaryProductionFocus(GameStateDescriptorValue);

    RebuildGoalSet(GameStateDescriptorValue);

    ArmyDomainStateValue.MinimumArmyCount = std::max<uint32_t>(1U, MacroStateDescriptorValue.DesiredArmyCount);
    ArmyDomainStateValue.EnsurePrimaryArmyExists();
    RebuildArmyGoals(GameStateDescriptorValue);
}

void FDefaultStrategicDirector::RebuildGoalSet(FGameStateDescriptor& GameStateDescriptorValue) const
{
    FAgentGoalSetDescriptor& AgentGoalSetDescriptorValue = GameStateDescriptorValue.GoalSet;
    AgentGoalSetDescriptorValue.Reset();
    AppendImmediateGoals(GameStateDescriptorValue, AgentGoalSetDescriptorValue.ImmediateGoals);
    AppendNearTermGoals(GameStateDescriptorValue, AgentGoalSetDescriptorValue.NearTermGoals);
    AppendStrategicGoals(GameStateDescriptorValue, AgentGoalSetDescriptorValue.StrategicGoals);
}

void FDefaultStrategicDirector::RebuildArmyGoals(FGameStateDescriptor& GameStateDescriptorValue) const
{
    FArmyDomainState& ArmyDomainStateValue = GameStateDescriptorValue.ArmyState;
    if (!ArmyDomainStateValue.ArmyGoals.empty())
    {
        ArmyDomainStateValue.ArmyGoals.front() = DeterminePrimaryArmyGoal(GameStateDescriptorValue);
    }

    if (ArmyDomainStateValue.ArmyGoals.size() > 1U)
    {
        ArmyDomainStateValue.ArmyGoals[1] = EArmyGoal::HoldBase;
    }
}

void FDefaultStrategicDirector::AppendImmediateGoals(
    const FGameStateDescriptor& GameStateDescriptorValue, std::vector<FGoalDescriptor>& GoalDescriptorsValue) const
{
    AppendGoalsForHorizon(GameStateDescriptorValue, EGoalHorizon::Immediate, GoalDescriptorsValue);
}

void FDefaultStrategicDirector::AppendNearTermGoals(
    const FGameStateDescriptor& GameStateDescriptorValue, std::vector<FGoalDescriptor>& GoalDescriptorsValue) const
{
    AppendGoalsForHorizon(GameStateDescriptorValue, EGoalHorizon::NearTerm, GoalDescriptorsValue);
}

void FDefaultStrategicDirector::AppendStrategicGoals(
    const FGameStateDescriptor& GameStateDescriptorValue, std::vector<FGoalDescriptor>& GoalDescriptorsValue) const
{
    AppendGoalsForHorizon(GameStateDescriptorValue, EGoalHorizon::Strategic, GoalDescriptorsValue);
}

EProductionFocus FDefaultStrategicDirector::DeterminePrimaryProductionFocus(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    return FTerranGoalRuleLibrary::DeterminePrimaryProductionFocus(GameStateDescriptorValue);
}

EGamePlan FDefaultStrategicDirector::DetermineGamePlan(const FGameStateDescriptor& GameStateDescriptorValue) const
{
    return FTerranGoalRuleLibrary::DetermineGamePlan(GameStateDescriptorValue);
}

uint32_t FDefaultStrategicDirector::DetermineDesiredArmyCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    return FTerranGoalRuleLibrary::DetermineDesiredArmyCount(GameStateDescriptorValue);
}

uint32_t FDefaultStrategicDirector::DetermineDesiredBaseCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    return FTerranGoalRuleLibrary::DetermineDesiredBaseCount(GameStateDescriptorValue);
}

uint32_t FDefaultStrategicDirector::DetermineDesiredWorkerCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    return FTerranGoalRuleLibrary::DetermineDesiredWorkerCount(GameStateDescriptorValue);
}

uint32_t FDefaultStrategicDirector::DetermineDesiredRefineryCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    return FTerranGoalRuleLibrary::DetermineDesiredRefineryCount(GameStateDescriptorValue);
}

uint32_t FDefaultStrategicDirector::DetermineDesiredSupplyDepotCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    return FTerranGoalRuleLibrary::DetermineDesiredSupplyDepotCount(GameStateDescriptorValue);
}

uint32_t FDefaultStrategicDirector::DetermineDesiredBarracksCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    return FTerranGoalRuleLibrary::DetermineDesiredBarracksCount(GameStateDescriptorValue);
}

uint32_t FDefaultStrategicDirector::DetermineDesiredFactoryCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    return FTerranGoalRuleLibrary::DetermineDesiredFactoryCount(GameStateDescriptorValue);
}

uint32_t FDefaultStrategicDirector::DetermineDesiredStarportCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    return FTerranGoalRuleLibrary::DetermineDesiredStarportCount(GameStateDescriptorValue);
}

uint32_t FDefaultStrategicDirector::DetermineDesiredMarineCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    return FTerranGoalRuleLibrary::DetermineDesiredMarineCount(GameStateDescriptorValue);
}

uint32_t FDefaultStrategicDirector::DetermineDesiredMarauderCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    return FTerranGoalRuleLibrary::DetermineDesiredMarauderCount(GameStateDescriptorValue);
}

uint32_t FDefaultStrategicDirector::DetermineDesiredCycloneCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    return FTerranGoalRuleLibrary::DetermineDesiredCycloneCount(GameStateDescriptorValue);
}

uint32_t FDefaultStrategicDirector::DetermineDesiredSiegeTankCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    return FTerranGoalRuleLibrary::DetermineDesiredSiegeTankCount(GameStateDescriptorValue);
}

uint32_t FDefaultStrategicDirector::DetermineDesiredMedivacCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    return FTerranGoalRuleLibrary::DetermineDesiredMedivacCount(GameStateDescriptorValue);
}

uint32_t FDefaultStrategicDirector::DetermineDesiredLiberatorCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    return FTerranGoalRuleLibrary::DetermineDesiredLiberatorCount(GameStateDescriptorValue);
}

bool FDefaultStrategicDirector::ShouldPrioritizeUpgrades(const FGameStateDescriptor& GameStateDescriptorValue) const
{
    return FTerranGoalRuleLibrary::ShouldPrioritizeUpgrades(GameStateDescriptorValue);
}

EArmyGoal FDefaultStrategicDirector::DeterminePrimaryArmyGoal(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    const FAgentGoalSetDescriptor& AgentGoalSetDescriptorValue = GameStateDescriptorValue.GoalSet;
    for (const FGoalDescriptor& GoalDescriptorValue : AgentGoalSetDescriptorValue.ImmediateGoals)
    {
        if (!FTerranGoalRuleLibrary::IsGoalActive(GoalDescriptorValue))
        {
            continue;
        }

        if (GoalDescriptorValue.GoalType == EGoalType::HoldOwnedBase)
        {
            return EArmyGoal::HoldBase;
        }
    }

    for (const FGoalDescriptor& GoalDescriptorValue : AgentGoalSetDescriptorValue.StrategicGoals)
    {
        if (!FTerranGoalRuleLibrary::IsGoalActive(GoalDescriptorValue))
        {
            continue;
        }

        switch (GoalDescriptorValue.GoalType)
        {
            case EGoalType::PressureEnemy:
                return EArmyGoal::TimingAttack;
            case EGoalType::ClearEnemyPresence:
                return EArmyGoal::FrontalAssault;
            case EGoalType::ScoutExpansionLocations:
                return EArmyGoal::MapControl;
            default:
                break;
        }
    }

    return EArmyGoal::HoldBase;
}

}  // namespace sc2
