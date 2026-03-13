#include "common/planning/FTerranTimingAttackBuildPlanner.h"

#include <algorithm>

#include "common/build_planning/FBuildPlanningState.h"
#include "common/descriptors/EGamePlan.h"
#include "common/descriptors/EMacroPhase.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/descriptors/FMacroStateDescriptor.h"

namespace sc2
{
namespace
{

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
    BuildPlanningStateValue.DesiredTownHallCount = 1U;
    BuildPlanningStateValue.DesiredOrbitalCommandCount = 0U;
    BuildPlanningStateValue.DesiredWorkerCount = 12U;
    BuildPlanningStateValue.DesiredRefineryCount = 0U;
    BuildPlanningStateValue.DesiredSupplyDepotCount = 1U;
    BuildPlanningStateValue.DesiredBarracksCount = 1U;
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

    BuildPlanningStateValue.ActivePackageCount = 1U;
    BuildPlanningStateValue.ActiveNeedCount = CountOutstandingNeeds(GameStateDescriptorValue, BuildPlanningStateValue);
}

void FTerranTimingAttackBuildPlanner::ProduceRecoveryBuildPlan(
    const FGameStateDescriptor& GameStateDescriptorValue, FBuildPlanningState& BuildPlanningStateValue) const
{
    const FMacroStateDescriptor& MacroStateDescriptorValue = GameStateDescriptorValue.MacroState;
    BuildPlanningStateValue.DesiredTownHallCount = std::max<uint32_t>(1U, MacroStateDescriptorValue.ActiveBaseCount);
    BuildPlanningStateValue.DesiredWorkerCount = std::max<uint32_t>(12U, MacroStateDescriptorValue.WorkerCount);
    BuildPlanningStateValue.DesiredSupplyDepotCount = 1U;
    BuildPlanningStateValue.DesiredBarracksCount = std::max<uint32_t>(1U, MacroStateDescriptorValue.BarracksCount);
    BuildPlanningStateValue.DesiredMarineCount = std::max<uint32_t>(4U, MacroStateDescriptorValue.ArmyUnitCount);
}

void FTerranTimingAttackBuildPlanner::ProduceTimingAttackBuildPlan(
    const FGameStateDescriptor& GameStateDescriptorValue, FBuildPlanningState& BuildPlanningStateValue) const
{
    const FMacroStateDescriptor& MacroStateDescriptorValue = GameStateDescriptorValue.MacroState;
    const uint64_t CurrentGameLoopValue = GameStateDescriptorValue.CurrentGameLoop;
    const uint64_t FrameOpeningEndGameLoopValue = ConvertClockTimeToGameLoops(4U, 20U);

    if (CurrentGameLoopValue <= FrameOpeningEndGameLoopValue ||
        MacroStateDescriptorValue.ActiveMacroPhase == EMacroPhase::Opening)
    {
        ProduceFrameOpeningBuildPlan(GameStateDescriptorValue, BuildPlanningStateValue);
        return;
    }

    switch (MacroStateDescriptorValue.ActiveMacroPhase)
    {
        case EMacroPhase::Opening:
            ProduceFrameOpeningBuildPlan(GameStateDescriptorValue, BuildPlanningStateValue);
            break;
        case EMacroPhase::EarlyGame:
            BuildPlanningStateValue.DesiredTownHallCount = 2U;
            BuildPlanningStateValue.DesiredOrbitalCommandCount = 2U;
            BuildPlanningStateValue.DesiredWorkerCount = 36U;
            BuildPlanningStateValue.DesiredRefineryCount = 2U;
            BuildPlanningStateValue.DesiredSupplyDepotCount = 5U;
            BuildPlanningStateValue.DesiredBarracksCount = 2U;
            BuildPlanningStateValue.DesiredBarracksReactorCount = 1U;
            BuildPlanningStateValue.DesiredFactoryCount = 1U;
            BuildPlanningStateValue.DesiredFactoryTechLabCount = 1U;
            BuildPlanningStateValue.DesiredStarportCount = 1U;
            BuildPlanningStateValue.DesiredMarineCount = 16U;
            BuildPlanningStateValue.DesiredMarauderCount = 2U;
            BuildPlanningStateValue.DesiredHellionCount = 1U;
            BuildPlanningStateValue.DesiredCycloneCount = 1U;
            BuildPlanningStateValue.DesiredMedivacCount = 1U;
            BuildPlanningStateValue.DesiredLiberatorCount = 1U;
            BuildPlanningStateValue.DesiredSiegeTankCount = 1U;
            break;
        case EMacroPhase::MidGame:
            BuildPlanningStateValue.DesiredTownHallCount = 3U;
            BuildPlanningStateValue.DesiredOrbitalCommandCount = 2U;
            BuildPlanningStateValue.DesiredWorkerCount = 44U;
            BuildPlanningStateValue.DesiredRefineryCount = 4U;
            BuildPlanningStateValue.DesiredSupplyDepotCount = 6U;
            BuildPlanningStateValue.DesiredBarracksCount = 3U;
            BuildPlanningStateValue.DesiredBarracksReactorCount = 1U;
            BuildPlanningStateValue.DesiredFactoryCount = 1U;
            BuildPlanningStateValue.DesiredFactoryTechLabCount = 1U;
            BuildPlanningStateValue.DesiredStarportCount = 1U;
            BuildPlanningStateValue.DesiredMarineCount = 32U;
            BuildPlanningStateValue.DesiredMarauderCount = 6U;
            BuildPlanningStateValue.DesiredHellionCount = 1U;
            BuildPlanningStateValue.DesiredCycloneCount = 1U;
            BuildPlanningStateValue.DesiredMedivacCount = 2U;
            BuildPlanningStateValue.DesiredLiberatorCount = 1U;
            BuildPlanningStateValue.DesiredSiegeTankCount = 1U;
            break;
        case EMacroPhase::LateGame:
            ProduceMacroBuildPlan(GameStateDescriptorValue, BuildPlanningStateValue);
            break;
        case EMacroPhase::Recovery:
            ProduceRecoveryBuildPlan(GameStateDescriptorValue, BuildPlanningStateValue);
            break;
        default:
            break;
    }
}

void FTerranTimingAttackBuildPlanner::ProduceFrameOpeningBuildPlan(
    const FGameStateDescriptor& GameStateDescriptorValue, FBuildPlanningState& BuildPlanningStateValue) const
{
    const uint64_t CurrentGameLoopValue = GameStateDescriptorValue.CurrentGameLoop;

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

    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(0U, 16U), 1U,
                                      BuildPlanningStateValue.DesiredSupplyDepotCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(0U, 40U), 1U,
                                      BuildPlanningStateValue.DesiredBarracksCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(0U, 42U), 1U,
                                      BuildPlanningStateValue.DesiredRefineryCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(1U, 26U), 1U,
                                      BuildPlanningStateValue.DesiredMarineCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(1U, 27U), 1U,
                                      BuildPlanningStateValue.DesiredOrbitalCommandCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(1U, 38U), 2U,
                                      BuildPlanningStateValue.DesiredTownHallCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(1U, 38U), 22U,
                                      BuildPlanningStateValue.DesiredWorkerCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(1U, 44U), 1U,
                                      BuildPlanningStateValue.DesiredBarracksReactorCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(1U, 50U), 2U,
                                      BuildPlanningStateValue.DesiredSupplyDepotCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(2U, 8U), 1U,
                                      BuildPlanningStateValue.DesiredFactoryCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(2U, 8U), 26U,
                                      BuildPlanningStateValue.DesiredWorkerCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(2U, 15U), 2U,
                                      BuildPlanningStateValue.DesiredRefineryCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(2U, 20U), 3U,
                                      BuildPlanningStateValue.DesiredMarineCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(2U, 38U), 5U,
                                      BuildPlanningStateValue.DesiredMarineCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(2U, 50U), 2U,
                                      BuildPlanningStateValue.DesiredOrbitalCommandCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(2U, 50U), 32U,
                                      BuildPlanningStateValue.DesiredWorkerCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(2U, 52U), 1U,
                                      BuildPlanningStateValue.DesiredStarportCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(2U, 54U), 1U,
                                      BuildPlanningStateValue.DesiredHellionCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(2U, 59U), 6U,
                                      BuildPlanningStateValue.DesiredMarineCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(3U, 2U), 7U,
                                      BuildPlanningStateValue.DesiredMarineCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(3U, 11U), 3U,
                                      BuildPlanningStateValue.DesiredSupplyDepotCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(3U, 16U), 1U,
                                      BuildPlanningStateValue.DesiredFactoryTechLabCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(3U, 21U), 8U,
                                      BuildPlanningStateValue.DesiredMarineCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(3U, 22U), 9U,
                                      BuildPlanningStateValue.DesiredMarineCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(3U, 28U), 1U,
                                      BuildPlanningStateValue.DesiredMedivacCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(3U, 34U), 1U,
                                      BuildPlanningStateValue.DesiredCycloneCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(3U, 41U), 11U,
                                      BuildPlanningStateValue.DesiredMarineCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(3U, 45U), 4U,
                                      BuildPlanningStateValue.DesiredSupplyDepotCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(3U, 55U), 2U,
                                      BuildPlanningStateValue.DesiredBarracksCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(4U, 1U), 1U,
                                      BuildPlanningStateValue.DesiredLiberatorCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(4U, 2U), 12U,
                                      BuildPlanningStateValue.DesiredMarineCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(4U, 9U), 1U,
                                      BuildPlanningStateValue.DesiredSiegeTankCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(4U, 9U), 36U,
                                      BuildPlanningStateValue.DesiredWorkerCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(4U, 15U), 5U,
                                      BuildPlanningStateValue.DesiredSupplyDepotCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(4U, 18U), 13U,
                                      BuildPlanningStateValue.DesiredMarineCount);
    ApplyBuildTargetAtOrAfterGameLoop(CurrentGameLoopValue, ConvertClockTimeToGameLoops(4U, 20U), 14U,
                                      BuildPlanningStateValue.DesiredMarineCount);
}

void FTerranTimingAttackBuildPlanner::ProduceMacroBuildPlan(const FGameStateDescriptor& GameStateDescriptorValue,
                                                            FBuildPlanningState& BuildPlanningStateValue) const
{
    (void)GameStateDescriptorValue;

    BuildPlanningStateValue.DesiredTownHallCount = 4U;
    BuildPlanningStateValue.DesiredOrbitalCommandCount = 4U;
    BuildPlanningStateValue.DesiredWorkerCount = 66U;
    BuildPlanningStateValue.DesiredRefineryCount = 8U;
    BuildPlanningStateValue.DesiredSupplyDepotCount = 10U;
    BuildPlanningStateValue.DesiredBarracksCount = 5U;
    BuildPlanningStateValue.DesiredBarracksReactorCount = 2U;
    BuildPlanningStateValue.DesiredFactoryCount = 2U;
    BuildPlanningStateValue.DesiredFactoryTechLabCount = 1U;
    BuildPlanningStateValue.DesiredStarportCount = 2U;
    BuildPlanningStateValue.DesiredMarineCount = 60U;
    BuildPlanningStateValue.DesiredMarauderCount = 12U;
    BuildPlanningStateValue.DesiredHellionCount = 2U;
    BuildPlanningStateValue.DesiredCycloneCount = 2U;
    BuildPlanningStateValue.DesiredMedivacCount = 8U;
    BuildPlanningStateValue.DesiredLiberatorCount = 4U;
    BuildPlanningStateValue.DesiredSiegeTankCount = 4U;
}

uint32_t FTerranTimingAttackBuildPlanner::CountOutstandingNeeds(
    const FGameStateDescriptor& GameStateDescriptorValue, const FBuildPlanningState& BuildPlanningStateValue) const
{
    const FMacroStateDescriptor& MacroStateDescriptorValue = GameStateDescriptorValue.MacroState;
    uint32_t OutstandingNeedCountValue = 0U;

    if (MacroStateDescriptorValue.ActiveBaseCount < BuildPlanningStateValue.DesiredTownHallCount)
    {
        ++OutstandingNeedCountValue;
    }
    if (MacroStateDescriptorValue.WorkerCount < BuildPlanningStateValue.DesiredWorkerCount)
    {
        ++OutstandingNeedCountValue;
    }
    if (MacroStateDescriptorValue.BarracksCount < BuildPlanningStateValue.DesiredBarracksCount)
    {
        ++OutstandingNeedCountValue;
    }
    if (MacroStateDescriptorValue.FactoryCount < BuildPlanningStateValue.DesiredFactoryCount)
    {
        ++OutstandingNeedCountValue;
    }
    if (MacroStateDescriptorValue.StarportCount < BuildPlanningStateValue.DesiredStarportCount)
    {
        ++OutstandingNeedCountValue;
    }
    if (MacroStateDescriptorValue.ArmyUnitCount < BuildPlanningStateValue.DesiredMarineCount)
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
