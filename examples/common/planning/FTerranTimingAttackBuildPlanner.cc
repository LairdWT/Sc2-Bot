#include "common/planning/FTerranTimingAttackBuildPlanner.h"

#include <algorithm>

#include "common/build_planning/FBuildPlanningState.h"
#include "common/descriptors/EGamePlan.h"
#include "common/descriptors/EMacroPhase.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/descriptors/FMacroStateDescriptor.h"
#include "common/economy/EconomyForecastConstants.h"
#include "common/goals/FAgentGoalSetDescriptor.h"

namespace sc2
{
namespace
{

void ResetDesiredCounts(FBuildPlanningState& BuildPlanningStateValue)
{
    BuildPlanningStateValue.DesiredTownHallCount = 1U;
    BuildPlanningStateValue.DesiredOrbitalCommandCount = 0U;
    BuildPlanningStateValue.DesiredWorkerCount = 12U;
    BuildPlanningStateValue.DesiredRefineryCount = 0U;
    BuildPlanningStateValue.DesiredSupplyDepotCount = 0U;
    BuildPlanningStateValue.DesiredBarracksCount = 0U;
    BuildPlanningStateValue.DesiredBarracksReactorCount = 0U;
    BuildPlanningStateValue.DesiredFactoryCount = 0U;
    BuildPlanningStateValue.DesiredFactoryTechLabCount = 0U;
    BuildPlanningStateValue.DesiredStarportCount = 0U;
    BuildPlanningStateValue.DesiredMarineCount = 0U;
    BuildPlanningStateValue.DesiredMarauderCount = 0U;
    BuildPlanningStateValue.DesiredHellionCount = 0U;
    BuildPlanningStateValue.DesiredCycloneCount = 0U;
    BuildPlanningStateValue.DesiredMedivacCount = 0U;
    BuildPlanningStateValue.DesiredLiberatorCount = 0U;
    BuildPlanningStateValue.DesiredSiegeTankCount = 0U;
}

void ApplyGoalDescriptorToBuildPlan(const FGoalDescriptor& GoalDescriptorValue,
                                    FBuildPlanningState& BuildPlanningStateValue)
{
    switch (GoalDescriptorValue.GoalType)
    {
        case EGoalType::SaturateWorkers:
            BuildPlanningStateValue.DesiredWorkerCount =
                std::max(BuildPlanningStateValue.DesiredWorkerCount, GoalDescriptorValue.TargetCount);
            break;
        case EGoalType::MaintainSupply:
            BuildPlanningStateValue.DesiredSupplyDepotCount =
                std::max(BuildPlanningStateValue.DesiredSupplyDepotCount, GoalDescriptorValue.TargetCount);
            break;
        case EGoalType::ExpandBaseCount:
            BuildPlanningStateValue.DesiredTownHallCount =
                std::max(BuildPlanningStateValue.DesiredTownHallCount, GoalDescriptorValue.TargetCount);
            break;
        case EGoalType::BuildProductionCapacity:
            switch (GoalDescriptorValue.TargetUnitTypeId)
            {
                case UNIT_TYPEID::TERRAN_REFINERY:
                    BuildPlanningStateValue.DesiredRefineryCount =
                        std::max(BuildPlanningStateValue.DesiredRefineryCount, GoalDescriptorValue.TargetCount);
                    break;
                case UNIT_TYPEID::TERRAN_SUPPLYDEPOT:
                    BuildPlanningStateValue.DesiredSupplyDepotCount =
                        std::max(BuildPlanningStateValue.DesiredSupplyDepotCount, GoalDescriptorValue.TargetCount);
                    break;
                case UNIT_TYPEID::TERRAN_BARRACKS:
                    BuildPlanningStateValue.DesiredBarracksCount =
                        std::max(BuildPlanningStateValue.DesiredBarracksCount, GoalDescriptorValue.TargetCount);
                    break;
                case UNIT_TYPEID::TERRAN_FACTORY:
                    BuildPlanningStateValue.DesiredFactoryCount =
                        std::max(BuildPlanningStateValue.DesiredFactoryCount, GoalDescriptorValue.TargetCount);
                    break;
                case UNIT_TYPEID::TERRAN_STARPORT:
                    BuildPlanningStateValue.DesiredStarportCount =
                        std::max(BuildPlanningStateValue.DesiredStarportCount, GoalDescriptorValue.TargetCount);
                    break;
                default:
                    break;
            }
            break;
        case EGoalType::UnlockTechnology:
            switch (GoalDescriptorValue.TargetUnitTypeId)
            {
                case UNIT_TYPEID::TERRAN_BARRACKSREACTOR:
                    BuildPlanningStateValue.DesiredBarracksReactorCount =
                        std::max(BuildPlanningStateValue.DesiredBarracksReactorCount, GoalDescriptorValue.TargetCount);
                    break;
                case UNIT_TYPEID::TERRAN_FACTORYTECHLAB:
                    BuildPlanningStateValue.DesiredFactoryTechLabCount =
                        std::max(BuildPlanningStateValue.DesiredFactoryTechLabCount, GoalDescriptorValue.TargetCount);
                    break;
                default:
                    break;
            }
            break;
        case EGoalType::ProduceArmy:
            switch (GoalDescriptorValue.TargetUnitTypeId)
            {
                case UNIT_TYPEID::TERRAN_MARINE:
                    BuildPlanningStateValue.DesiredMarineCount =
                        std::max(BuildPlanningStateValue.DesiredMarineCount, GoalDescriptorValue.TargetCount);
                    break;
                case UNIT_TYPEID::TERRAN_MARAUDER:
                    BuildPlanningStateValue.DesiredMarauderCount =
                        std::max(BuildPlanningStateValue.DesiredMarauderCount, GoalDescriptorValue.TargetCount);
                    break;
                case UNIT_TYPEID::TERRAN_HELLION:
                    BuildPlanningStateValue.DesiredHellionCount =
                        std::max(BuildPlanningStateValue.DesiredHellionCount, GoalDescriptorValue.TargetCount);
                    break;
                case UNIT_TYPEID::TERRAN_CYCLONE:
                    BuildPlanningStateValue.DesiredCycloneCount =
                        std::max(BuildPlanningStateValue.DesiredCycloneCount, GoalDescriptorValue.TargetCount);
                    break;
                case UNIT_TYPEID::TERRAN_MEDIVAC:
                    BuildPlanningStateValue.DesiredMedivacCount =
                        std::max(BuildPlanningStateValue.DesiredMedivacCount, GoalDescriptorValue.TargetCount);
                    break;
                case UNIT_TYPEID::TERRAN_LIBERATOR:
                    BuildPlanningStateValue.DesiredLiberatorCount =
                        std::max(BuildPlanningStateValue.DesiredLiberatorCount, GoalDescriptorValue.TargetCount);
                    break;
                case UNIT_TYPEID::TERRAN_SIEGETANK:
                    BuildPlanningStateValue.DesiredSiegeTankCount =
                        std::max(BuildPlanningStateValue.DesiredSiegeTankCount, GoalDescriptorValue.TargetCount);
                    break;
                default:
                    break;
            }
            break;
        case EGoalType::HoldOwnedBase:
        case EGoalType::ResearchUpgrade:
        case EGoalType::PressureEnemy:
        case EGoalType::ClearEnemyPresence:
        case EGoalType::ScoutExpansionLocations:
        default:
            break;
    }
}

void ApplyGoalListToBuildPlan(const std::vector<FGoalDescriptor>& GoalDescriptorsValue,
                              FBuildPlanningState& BuildPlanningStateValue)
{
    for (const FGoalDescriptor& GoalDescriptorValue : GoalDescriptorsValue)
    {
        switch (GoalDescriptorValue.GoalStatus)
        {
            case EGoalStatus::Active:
            case EGoalStatus::Satisfied:
                ApplyGoalDescriptorToBuildPlan(GoalDescriptorValue, BuildPlanningStateValue);
                break;
            case EGoalStatus::Proposed:
            case EGoalStatus::Blocked:
            case EGoalStatus::Abandoned:
            default:
                break;
        }
    }
}

void ApplyGoalDrivenBuildPlan(const FGameStateDescriptor& GameStateDescriptorValue,
                              FBuildPlanningState& BuildPlanningStateValue)
{
    const FAgentGoalSetDescriptor& AgentGoalSetDescriptorValue = GameStateDescriptorValue.GoalSet;
    ApplyGoalListToBuildPlan(AgentGoalSetDescriptorValue.ImmediateGoals, BuildPlanningStateValue);
    ApplyGoalListToBuildPlan(AgentGoalSetDescriptorValue.NearTermGoals, BuildPlanningStateValue);
    ApplyGoalListToBuildPlan(AgentGoalSetDescriptorValue.StrategicGoals, BuildPlanningStateValue);

    const FMacroStateDescriptor& MacroStateDescriptorValue = GameStateDescriptorValue.MacroState;
    BuildPlanningStateValue.DesiredTownHallCount =
        std::max(BuildPlanningStateValue.DesiredTownHallCount, MacroStateDescriptorValue.ActiveBaseCount);
    BuildPlanningStateValue.DesiredOrbitalCommandCount =
        std::min(BuildPlanningStateValue.DesiredTownHallCount, std::max<uint32_t>(1U, MacroStateDescriptorValue.ActiveBaseCount));
    BuildPlanningStateValue.DesiredWorkerCount =
        std::max(BuildPlanningStateValue.DesiredWorkerCount, MacroStateDescriptorValue.WorkerCount);
    BuildPlanningStateValue.DesiredBarracksCount =
        std::max(BuildPlanningStateValue.DesiredBarracksCount, MacroStateDescriptorValue.BarracksCount);
    BuildPlanningStateValue.DesiredFactoryCount =
        std::max(BuildPlanningStateValue.DesiredFactoryCount, MacroStateDescriptorValue.FactoryCount);
    BuildPlanningStateValue.DesiredStarportCount =
        std::max(BuildPlanningStateValue.DesiredStarportCount, MacroStateDescriptorValue.StarportCount);
}

uint32_t GetObservedSupplyDepotCount(const FGameStateDescriptor& GameStateDescriptorValue)
{
    return GameStateDescriptorValue.ProductionState.GetProjectedBuildingCount(UNIT_TYPEID::TERRAN_SUPPLYDEPOT);
}

uint32_t GetObservedRefineryCount(const FGameStateDescriptor& GameStateDescriptorValue)
{
    return GameStateDescriptorValue.ProductionState.GetProjectedBuildingCount(UNIT_TYPEID::TERRAN_REFINERY);
}

void ApplyBuildTargetAtOrAfterGameLoop(const uint64_t CurrentGameLoopValue, const uint64_t TargetGameLoopValue,
                                       const uint32_t DesiredCountValue, uint32_t& TargetCountValue)
{
    if (CurrentGameLoopValue >= TargetGameLoopValue)
    {
        TargetCountValue = std::max(TargetCountValue, DesiredCountValue);
    }
}

}  // namespace

void FTerranTimingAttackBuildPlanner::ProduceBuildPlan(const FGameStateDescriptor& GameStateDescriptorValue,
                                                       FBuildPlanningState& BuildPlanningStateValue) const
{
    ResetDesiredCounts(BuildPlanningStateValue);

    switch (GameStateDescriptorValue.MacroState.ActiveGamePlan)
    {
        case EGamePlan::Recovery:
            ProduceRecoveryBuildPlan(GameStateDescriptorValue, BuildPlanningStateValue);
            break;
        case EGamePlan::Macro:
            ProduceMacroBuildPlan(GameStateDescriptorValue, BuildPlanningStateValue);
            break;
        case EGamePlan::TimingAttack:
        case EGamePlan::Aggressive:
        case EGamePlan::AllIn:
        case EGamePlan::Unknown:
        default:
            ProduceTimingAttackBuildPlan(GameStateDescriptorValue, BuildPlanningStateValue);
            break;
    }

    BuildPlanningStateValue.ActivePackageCount = static_cast<uint32_t>(
        GameStateDescriptorValue.GoalSet.ImmediateGoals.size() + GameStateDescriptorValue.GoalSet.NearTermGoals.size() +
        GameStateDescriptorValue.GoalSet.StrategicGoals.size());
    BuildPlanningStateValue.ActiveNeedCount = CountOutstandingNeeds(GameStateDescriptorValue, BuildPlanningStateValue);
}

void FTerranTimingAttackBuildPlanner::ProduceRecoveryBuildPlan(
    const FGameStateDescriptor& GameStateDescriptorValue, FBuildPlanningState& BuildPlanningStateValue) const
{
    const FMacroStateDescriptor& MacroStateDescriptorValue = GameStateDescriptorValue.MacroState;
    ApplyGoalDrivenBuildPlan(GameStateDescriptorValue, BuildPlanningStateValue);
    BuildPlanningStateValue.DesiredTownHallCount = std::max<uint32_t>(1U, BuildPlanningStateValue.DesiredTownHallCount);
    BuildPlanningStateValue.DesiredWorkerCount = std::max<uint32_t>(12U, BuildPlanningStateValue.DesiredWorkerCount);
    BuildPlanningStateValue.DesiredSupplyDepotCount = std::max<uint32_t>(1U, BuildPlanningStateValue.DesiredSupplyDepotCount);
    BuildPlanningStateValue.DesiredBarracksCount =
        std::max<uint32_t>(1U, std::max(BuildPlanningStateValue.DesiredBarracksCount, MacroStateDescriptorValue.BarracksCount));
    BuildPlanningStateValue.DesiredMarineCount = std::max<uint32_t>(4U, BuildPlanningStateValue.DesiredMarineCount);
}

void FTerranTimingAttackBuildPlanner::ProduceTimingAttackBuildPlan(
    const FGameStateDescriptor& GameStateDescriptorValue, FBuildPlanningState& BuildPlanningStateValue) const
{
    ApplyGoalDrivenBuildPlan(GameStateDescriptorValue, BuildPlanningStateValue);
}

void FTerranTimingAttackBuildPlanner::ProduceFrameOpeningBuildPlan(
    const FGameStateDescriptor& GameStateDescriptorValue, FBuildPlanningState& BuildPlanningStateValue) const
{
    ApplyGoalDrivenBuildPlan(GameStateDescriptorValue, BuildPlanningStateValue);
}

void FTerranTimingAttackBuildPlanner::ProduceMacroBuildPlan(const FGameStateDescriptor& GameStateDescriptorValue,
                                                            FBuildPlanningState& BuildPlanningStateValue) const
{
    ApplyGoalDrivenBuildPlan(GameStateDescriptorValue, BuildPlanningStateValue);
}

uint32_t FTerranTimingAttackBuildPlanner::CountOutstandingNeeds(
    const FGameStateDescriptor& GameStateDescriptorValue, const FBuildPlanningState& BuildPlanningStateValue) const
{
    const FProductionStateDescriptor& ProductionStateDescriptorValue = GameStateDescriptorValue.ProductionState;
    const FEconomyStateDescriptor& EconomyStateDescriptorValue = GameStateDescriptorValue.EconomyState;
    uint32_t OutstandingNeedCountValue = 0U;

    if (ProductionStateDescriptorValue.GetProjectedBuildingCount(UNIT_TYPEID::TERRAN_COMMANDCENTER) <
        BuildPlanningStateValue.DesiredTownHallCount)
    {
        ++OutstandingNeedCountValue;
    }
    if (EconomyStateDescriptorValue.ProjectedAvailableSupplyByHorizon[ShortForecastHorizonIndexValue] <= 2U)
    {
        ++OutstandingNeedCountValue;
    }
    if (ProductionStateDescriptorValue.GetProjectedUnitCount(UNIT_TYPEID::TERRAN_SCV) <
        BuildPlanningStateValue.DesiredWorkerCount)
    {
        ++OutstandingNeedCountValue;
    }
    if (GetObservedSupplyDepotCount(GameStateDescriptorValue) < BuildPlanningStateValue.DesiredSupplyDepotCount)
    {
        ++OutstandingNeedCountValue;
    }
    if (GetObservedRefineryCount(GameStateDescriptorValue) < BuildPlanningStateValue.DesiredRefineryCount)
    {
        ++OutstandingNeedCountValue;
    }
    if (ProductionStateDescriptorValue.GetProjectedBuildingCount(UNIT_TYPEID::TERRAN_BARRACKS) <
        BuildPlanningStateValue.DesiredBarracksCount)
    {
        ++OutstandingNeedCountValue;
    }
    if (ProductionStateDescriptorValue.GetProjectedBuildingCount(UNIT_TYPEID::TERRAN_FACTORY) <
        BuildPlanningStateValue.DesiredFactoryCount)
    {
        ++OutstandingNeedCountValue;
    }
    if (ProductionStateDescriptorValue.GetProjectedBuildingCount(UNIT_TYPEID::TERRAN_STARPORT) <
        BuildPlanningStateValue.DesiredStarportCount)
    {
        ++OutstandingNeedCountValue;
    }
    if (ProductionStateDescriptorValue.GetProjectedUnitCount(UNIT_TYPEID::TERRAN_MARINE) <
        BuildPlanningStateValue.DesiredMarineCount)
    {
        ++OutstandingNeedCountValue;
    }

    return OutstandingNeedCountValue;
}

uint64_t FTerranTimingAttackBuildPlanner::ConvertClockTimeToGameLoops(const uint32_t MinutesValue,
                                                                      const uint32_t SecondsValue)
{
    const uint64_t TotalSecondsValue = (static_cast<uint64_t>(MinutesValue) * 60U) + SecondsValue;
    return ((TotalSecondsValue * 112U) + 2U) / 5U;
}

}  // namespace sc2
