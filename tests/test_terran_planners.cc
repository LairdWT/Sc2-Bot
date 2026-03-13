#include "test_terran_planners.h"

#include <iostream>
#include <string>

#include "common/armies/EArmyGoal.h"
#include "common/armies/EArmyPosture.h"
#include "common/armies/FArmyDomainState.h"
#include "common/build_planning/FBuildPlanningState.h"
#include "common/descriptors/EGamePlan.h"
#include "common/descriptors/EMacroPhase.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/planning/FTerranArmyPlanner.h"
#include "common/planning/FTerranTimingAttackBuildPlanner.h"

namespace sc2
{
namespace
{

uint64_t ConvertClockTimeToGameLoops(const uint32_t MinutesValue, const uint32_t SecondsValue)
{
    const uint64_t TotalSecondsValue = (static_cast<uint64_t>(MinutesValue) * 60U) + SecondsValue;
    return ((TotalSecondsValue * 112U) + 2U) / 5U;
}

bool Check(const bool ConditionValue, bool& SuccessValue, const std::string& MessageValue)
{
    if (!ConditionValue)
    {
        SuccessValue = false;
        std::cerr << "    " << MessageValue << std::endl;
    }

    return ConditionValue;
}

FGameStateDescriptor CreateOpeningDescriptor(const uint64_t CurrentGameLoopValue)
{
    FGameStateDescriptor GameStateDescriptorValue;
    GameStateDescriptorValue.CurrentGameLoop = CurrentGameLoopValue;
    GameStateDescriptorValue.MacroState.ActiveGamePlan = EGamePlan::TimingAttack;
    GameStateDescriptorValue.MacroState.ActiveMacroPhase = EMacroPhase::Opening;
    GameStateDescriptorValue.MacroState.CurrentGameLoop = CurrentGameLoopValue;
    GameStateDescriptorValue.MacroState.ActiveBaseCount = 1U;
    GameStateDescriptorValue.MacroState.WorkerCount = 16U;
    GameStateDescriptorValue.MacroState.ArmyUnitCount = 6U;
    GameStateDescriptorValue.MacroState.ArmySupply = 6U;
    GameStateDescriptorValue.MacroState.BarracksCount = 1U;
    return GameStateDescriptorValue;
}

FGameStateDescriptor CreateMidGameTimingDescriptor()
{
    FGameStateDescriptor GameStateDescriptorValue;
    GameStateDescriptorValue.CurrentGameLoop = ConvertClockTimeToGameLoops(5U, 10U);
    GameStateDescriptorValue.MacroState.ActiveGamePlan = EGamePlan::TimingAttack;
    GameStateDescriptorValue.MacroState.ActiveMacroPhase = EMacroPhase::MidGame;
    GameStateDescriptorValue.MacroState.CurrentGameLoop = GameStateDescriptorValue.CurrentGameLoop;
    GameStateDescriptorValue.MacroState.ActiveBaseCount = 2U;
    GameStateDescriptorValue.MacroState.WorkerCount = 38U;
    GameStateDescriptorValue.MacroState.ArmyUnitCount = 28U;
    GameStateDescriptorValue.MacroState.ArmySupply = 52U;
    GameStateDescriptorValue.MacroState.BarracksCount = 2U;
    GameStateDescriptorValue.MacroState.FactoryCount = 1U;
    GameStateDescriptorValue.MacroState.StarportCount = 1U;
    GameStateDescriptorValue.ArmyState.ArmyGoals.front() = EArmyGoal::TimingAttack;
    return GameStateDescriptorValue;
}

FGameStateDescriptor CreateMacroDescriptor()
{
    FGameStateDescriptor GameStateDescriptorValue;
    GameStateDescriptorValue.MacroState.ActiveGamePlan = EGamePlan::Macro;
    GameStateDescriptorValue.MacroState.ActiveMacroPhase = EMacroPhase::LateGame;
    GameStateDescriptorValue.MacroState.ActiveBaseCount = 3U;
    GameStateDescriptorValue.MacroState.WorkerCount = 64U;
    GameStateDescriptorValue.MacroState.ArmyUnitCount = 70U;
    GameStateDescriptorValue.MacroState.ArmySupply = 122U;
    GameStateDescriptorValue.MacroState.BarracksCount = 4U;
    GameStateDescriptorValue.MacroState.FactoryCount = 2U;
    GameStateDescriptorValue.MacroState.StarportCount = 1U;
    GameStateDescriptorValue.ArmyState.MinimumArmyCount = 2U;
    GameStateDescriptorValue.ArmyState.EnsurePrimaryArmyExists();
    GameStateDescriptorValue.ArmyState.ArmyGoals.front() = EArmyGoal::MapControl;
    return GameStateDescriptorValue;
}

}  // namespace

bool TestTerranPlanners(int ArgC, char** ArgV)
{
    (void)ArgC;
    (void)ArgV;

    bool SuccessValue = true;

    FTerranTimingAttackBuildPlanner BuildPlannerValue;
    FTerranArmyPlanner ArmyPlannerValue;

    {
        FGameStateDescriptor GameStateDescriptorValue =
            CreateOpeningDescriptor(ConvertClockTimeToGameLoops(0U, 42U));
        FBuildPlanningState BuildPlanningStateValue;
        BuildPlannerValue.ProduceBuildPlan(GameStateDescriptorValue, BuildPlanningStateValue);

        Check(BuildPlanningStateValue.DesiredSupplyDepotCount == 1U, SuccessValue,
              "Early opener should target the first supply depot.");
        Check(BuildPlanningStateValue.DesiredBarracksCount == 1U, SuccessValue,
              "Early opener should target the first barracks.");
        Check(BuildPlanningStateValue.DesiredRefineryCount == 1U, SuccessValue,
              "Early opener should target the first refinery.");
        Check(BuildPlanningStateValue.DesiredOrbitalCommandCount == 0U, SuccessValue,
              "Orbital command should not be targeted before its opener frame.");

        ArmyPlannerValue.ProduceArmyPlan(GameStateDescriptorValue, GameStateDescriptorValue.ArmyState);
        Check(!GameStateDescriptorValue.ArmyState.ArmyPostures.empty() &&
                  GameStateDescriptorValue.ArmyState.ArmyPostures.front() == EArmyPosture::Assemble,
              SuccessValue, "Opening army posture should assemble instead of engaging.");
    }

    {
        FGameStateDescriptor GameStateDescriptorValue =
            CreateOpeningDescriptor(ConvertClockTimeToGameLoops(3U, 28U));
        FBuildPlanningState BuildPlanningStateValue;
        BuildPlannerValue.ProduceBuildPlan(GameStateDescriptorValue, BuildPlanningStateValue);

        Check(BuildPlanningStateValue.DesiredTownHallCount == 2U, SuccessValue,
              "Frame opener should target the natural expansion by the medivac timing.");
        Check(BuildPlanningStateValue.DesiredOrbitalCommandCount == 2U, SuccessValue,
              "Frame opener should target both orbital commands by the medivac timing.");
        Check(BuildPlanningStateValue.DesiredBarracksReactorCount == 1U, SuccessValue,
              "Frame opener should target one barracks reactor.");
        Check(BuildPlanningStateValue.DesiredFactoryCount == 1U, SuccessValue,
              "Frame opener should target one factory.");
        Check(BuildPlanningStateValue.DesiredFactoryTechLabCount == 1U, SuccessValue,
              "Frame opener should target one factory tech lab.");
        Check(BuildPlanningStateValue.DesiredStarportCount == 1U, SuccessValue,
              "Frame opener should target one starport.");
        Check(BuildPlanningStateValue.DesiredHellionCount == 1U, SuccessValue,
              "Frame opener should target the first hellion.");
        Check(BuildPlanningStateValue.DesiredMedivacCount == 1U, SuccessValue,
              "Frame opener should target the first medivac.");
        Check(BuildPlanningStateValue.DesiredCycloneCount == 0U, SuccessValue,
              "Cyclone should not be targeted before its opener frame.");
    }

    {
        FGameStateDescriptor GameStateDescriptorValue = CreateMidGameTimingDescriptor();
        FBuildPlanningState BuildPlanningStateValue;
        BuildPlannerValue.ProduceBuildPlan(GameStateDescriptorValue, BuildPlanningStateValue);

        Check(BuildPlanningStateValue.DesiredTownHallCount == 3U, SuccessValue,
              "Mid-game timing plan should target a third town hall.");
        Check(BuildPlanningStateValue.DesiredOrbitalCommandCount == 2U, SuccessValue,
              "Mid-game timing plan should preserve two orbital commands.");
        Check(BuildPlanningStateValue.DesiredBarracksCount == 3U, SuccessValue,
              "Mid-game timing plan should target three barracks.");
        Check(BuildPlanningStateValue.DesiredFactoryCount == 1U, SuccessValue,
              "Mid-game timing plan should preserve one factory.");
        Check(BuildPlanningStateValue.DesiredStarportCount == 1U, SuccessValue,
              "Mid-game timing plan should preserve one starport.");
        Check(BuildPlanningStateValue.DesiredMedivacCount == 2U, SuccessValue,
              "Mid-game timing plan should target two medivacs.");
        Check(BuildPlanningStateValue.DesiredSiegeTankCount == 1U, SuccessValue,
              "Mid-game timing plan should preserve one siege tank.");

        ArmyPlannerValue.ProduceArmyPlan(GameStateDescriptorValue, GameStateDescriptorValue.ArmyState);
        Check(!GameStateDescriptorValue.ArmyState.ArmyPostures.empty() &&
                  GameStateDescriptorValue.ArmyState.ArmyPostures.front() == EArmyPosture::Engage,
              SuccessValue, "Ready timing armies should move to the Engage posture.");
    }

    {
        FGameStateDescriptor GameStateDescriptorValue = CreateMacroDescriptor();
        FBuildPlanningState BuildPlanningStateValue;
        BuildPlannerValue.ProduceBuildPlan(GameStateDescriptorValue, BuildPlanningStateValue);

        Check(BuildPlanningStateValue.DesiredTownHallCount == 4U, SuccessValue,
              "Macro plan should target a fourth town hall.");
        Check(BuildPlanningStateValue.DesiredWorkerCount == 66U, SuccessValue,
              "Macro plan should target sixty-six workers.");
        Check(BuildPlanningStateValue.DesiredBarracksCount == 5U, SuccessValue,
              "Macro plan should target five barracks.");
        Check(BuildPlanningStateValue.DesiredStarportCount == 2U, SuccessValue,
              "Macro plan should target two starports.");

        ArmyPlannerValue.ProduceArmyPlan(GameStateDescriptorValue, GameStateDescriptorValue.ArmyState);
        Check(GameStateDescriptorValue.ArmyState.ArmyPostures.size() >= 2U, SuccessValue,
              "Macro army plan should preserve posture storage for multiple armies.");
        Check(GameStateDescriptorValue.ArmyState.ArmyPostures.front() == EArmyPosture::Advance, SuccessValue,
              "Macro primary army should advance for map control once the army is large enough.");
        Check(GameStateDescriptorValue.ArmyState.ArmyPostures[1] == EArmyPosture::Hold, SuccessValue,
              "Secondary macro army posture should default to Hold.");
    }

    return SuccessValue;
}

}  // namespace sc2
