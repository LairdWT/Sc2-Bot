#include "test_terran_planners.h"

#include <iostream>
#include <string>

#include "common/armies/EArmyGoal.h"
#include "common/armies/EArmyMissionType.h"
#include "common/armies/EArmyPosture.h"
#include "common/armies/FArmyDomainState.h"
#include "common/build_planning/FBuildPlanningState.h"
#include "common/descriptors/EGamePlan.h"
#include "common/descriptors/EMacroPhase.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/goals/EGoalDomain.h"
#include "common/goals/EGoalHorizon.h"
#include "common/goals/EGoalStatus.h"
#include "common/goals/EGoalType.h"
#include "common/goals/FGoalDescriptor.h"
#include "common/planning/FTerranArmyPlanner.h"
#include "common/planning/FTerranTimingAttackBuildPlanner.h"

namespace sc2
{
namespace
{

bool Check(const bool ConditionValue, bool& SuccessValue, const std::string& MessageValue)
{
    if (!ConditionValue)
    {
        SuccessValue = false;
        std::cerr << "    " << MessageValue << std::endl;
    }

    return ConditionValue;
}

FGoalDescriptor CreateGoalDescriptor(const uint32_t GoalIdValue, const EGoalDomain GoalDomainValue,
                                     const EGoalHorizon GoalHorizonValue, const EGoalType GoalTypeValue,
                                     const EGoalStatus GoalStatusValue, const int BasePriorityValue,
                                     const uint32_t TargetCountValue = 0U,
                                     const UNIT_TYPEID TargetUnitTypeIdValue = UNIT_TYPEID::INVALID,
                                     const UpgradeID TargetUpgradeIdValue = UpgradeID(UPGRADE_ID::INVALID))
{
    FGoalDescriptor GoalDescriptorValue;
    GoalDescriptorValue.GoalId = GoalIdValue;
    GoalDescriptorValue.GoalDomain = GoalDomainValue;
    GoalDescriptorValue.GoalHorizon = GoalHorizonValue;
    GoalDescriptorValue.GoalType = GoalTypeValue;
    GoalDescriptorValue.GoalStatus = GoalStatusValue;
    GoalDescriptorValue.BasePriorityValue = BasePriorityValue;
    GoalDescriptorValue.TargetCount = TargetCountValue;
    GoalDescriptorValue.TargetUnitTypeId = TargetUnitTypeIdValue;
    GoalDescriptorValue.TargetUpgradeId = TargetUpgradeIdValue;
    return GoalDescriptorValue;
}

FGameStateDescriptor CreateGoalDrivenOpeningDescriptor()
{
    FGameStateDescriptor GameStateDescriptorValue;
    GameStateDescriptorValue.MacroState.ActiveGamePlan = EGamePlan::TimingAttack;
    GameStateDescriptorValue.MacroState.ActiveMacroPhase = EMacroPhase::Opening;
    GameStateDescriptorValue.MacroState.ActiveBaseCount = 1U;
    GameStateDescriptorValue.MacroState.WorkerCount = 16U;
    GameStateDescriptorValue.MacroState.ArmyUnitCount = 6U;
    GameStateDescriptorValue.MacroState.ArmySupply = 6U;
    GameStateDescriptorValue.MacroState.BarracksCount = 1U;
    GameStateDescriptorValue.GoalSet.ImmediateGoals.push_back(CreateGoalDescriptor(
        100U, EGoalDomain::Economy, EGoalHorizon::Immediate, EGoalType::MaintainSupply, EGoalStatus::Active, 200,
        1U, UNIT_TYPEID::TERRAN_SUPPLYDEPOT));
    GameStateDescriptorValue.GoalSet.NearTermGoals.push_back(CreateGoalDescriptor(
        110U, EGoalDomain::Economy, EGoalHorizon::NearTerm, EGoalType::BuildProductionCapacity,
        EGoalStatus::Active, 180, 1U, UNIT_TYPEID::TERRAN_REFINERY));
    GameStateDescriptorValue.GoalSet.NearTermGoals.push_back(CreateGoalDescriptor(
        120U, EGoalDomain::Production, EGoalHorizon::NearTerm, EGoalType::BuildProductionCapacity,
        EGoalStatus::Active, 190, 1U, UNIT_TYPEID::TERRAN_BARRACKS));
    GameStateDescriptorValue.ArmyState.EnsurePrimaryArmyExists();
    GameStateDescriptorValue.ArmyState.ArmyMissions.front().MissionType = EArmyMissionType::AssembleAtRally;
    return GameStateDescriptorValue;
}

FGameStateDescriptor CreateGoalDrivenTimingDescriptor()
{
    FGameStateDescriptor GameStateDescriptorValue;
    GameStateDescriptorValue.MacroState.ActiveGamePlan = EGamePlan::TimingAttack;
    GameStateDescriptorValue.MacroState.ActiveMacroPhase = EMacroPhase::MidGame;
    GameStateDescriptorValue.MacroState.ActiveBaseCount = 2U;
    GameStateDescriptorValue.MacroState.WorkerCount = 38U;
    GameStateDescriptorValue.MacroState.ArmyUnitCount = 28U;
    GameStateDescriptorValue.MacroState.ArmySupply = 52U;
    GameStateDescriptorValue.MacroState.BarracksCount = 2U;
    GameStateDescriptorValue.MacroState.FactoryCount = 1U;
    GameStateDescriptorValue.MacroState.StarportCount = 1U;
    GameStateDescriptorValue.GoalSet.NearTermGoals.push_back(CreateGoalDescriptor(
        200U, EGoalDomain::Economy, EGoalHorizon::NearTerm, EGoalType::ExpandBaseCount, EGoalStatus::Active, 180,
        2U));
    GameStateDescriptorValue.GoalSet.NearTermGoals.push_back(CreateGoalDescriptor(
        210U, EGoalDomain::Technology, EGoalHorizon::NearTerm, EGoalType::UnlockTechnology, EGoalStatus::Active, 170,
        1U, UNIT_TYPEID::TERRAN_BARRACKSREACTOR));
    GameStateDescriptorValue.GoalSet.NearTermGoals.push_back(CreateGoalDescriptor(
        211U, EGoalDomain::Technology, EGoalHorizon::NearTerm, EGoalType::UnlockTechnology, EGoalStatus::Active, 165,
        1U, UNIT_TYPEID::TERRAN_FACTORYTECHLAB));
    GameStateDescriptorValue.GoalSet.NearTermGoals.push_back(CreateGoalDescriptor(
        212U, EGoalDomain::Technology, EGoalHorizon::NearTerm, EGoalType::UnlockTechnology, EGoalStatus::Active, 160,
        1U, UNIT_TYPEID::TERRAN_STARPORTREACTOR));
    GameStateDescriptorValue.GoalSet.NearTermGoals.push_back(CreateGoalDescriptor(
        213U, EGoalDomain::Technology, EGoalHorizon::NearTerm, EGoalType::UnlockTechnology, EGoalStatus::Active, 158,
        1U, UNIT_TYPEID::TERRAN_ENGINEERINGBAY));
    GameStateDescriptorValue.GoalSet.NearTermGoals.push_back(CreateGoalDescriptor(
        214U, EGoalDomain::Technology, EGoalHorizon::NearTerm, EGoalType::ResearchUpgrade, EGoalStatus::Active, 156,
        1U, UNIT_TYPEID::INVALID, UpgradeID(UPGRADE_ID::STIMPACK)));
    GameStateDescriptorValue.GoalSet.NearTermGoals.push_back(CreateGoalDescriptor(
        220U, EGoalDomain::Production, EGoalHorizon::NearTerm, EGoalType::BuildProductionCapacity,
        EGoalStatus::Active, 175, 1U, UNIT_TYPEID::TERRAN_FACTORY));
    GameStateDescriptorValue.GoalSet.NearTermGoals.push_back(CreateGoalDescriptor(
        221U, EGoalDomain::Production, EGoalHorizon::NearTerm, EGoalType::BuildProductionCapacity,
        EGoalStatus::Active, 170, 1U, UNIT_TYPEID::TERRAN_STARPORT));
    GameStateDescriptorValue.GoalSet.StrategicGoals.push_back(CreateGoalDescriptor(
        300U, EGoalDomain::Army, EGoalHorizon::Strategic, EGoalType::ProduceArmy, EGoalStatus::Active, 160, 1U,
        UNIT_TYPEID::TERRAN_HELLION));
    GameStateDescriptorValue.GoalSet.StrategicGoals.push_back(CreateGoalDescriptor(
        301U, EGoalDomain::Army, EGoalHorizon::Strategic, EGoalType::ProduceArmy, EGoalStatus::Active, 155, 1U,
        UNIT_TYPEID::TERRAN_MEDIVAC));
    GameStateDescriptorValue.GoalSet.StrategicGoals.push_back(CreateGoalDescriptor(
        302U, EGoalDomain::Army, EGoalHorizon::Strategic, EGoalType::ProduceArmy, EGoalStatus::Active, 150, 1U,
        UNIT_TYPEID::TERRAN_SIEGETANK));
    GameStateDescriptorValue.ArmyState.EnsurePrimaryArmyExists();
    GameStateDescriptorValue.ArmyState.ArmyGoals.front() = EArmyGoal::TimingAttack;
    GameStateDescriptorValue.ArmyState.ArmyMissions.front().MissionType = EArmyMissionType::PressureKnownEnemyBase;
    return GameStateDescriptorValue;
}

FGameStateDescriptor CreateGoalDrivenMacroDescriptor()
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
    GameStateDescriptorValue.GoalSet.NearTermGoals.push_back(CreateGoalDescriptor(
        400U, EGoalDomain::Economy, EGoalHorizon::NearTerm, EGoalType::ExpandBaseCount, EGoalStatus::Active, 180,
        4U));
    GameStateDescriptorValue.GoalSet.NearTermGoals.push_back(CreateGoalDescriptor(
        410U, EGoalDomain::Economy, EGoalHorizon::NearTerm, EGoalType::SaturateWorkers, EGoalStatus::Active, 170,
        66U));
    GameStateDescriptorValue.GoalSet.NearTermGoals.push_back(CreateGoalDescriptor(
        420U, EGoalDomain::Production, EGoalHorizon::NearTerm, EGoalType::BuildProductionCapacity,
        EGoalStatus::Active, 160, 5U, UNIT_TYPEID::TERRAN_BARRACKS));
    GameStateDescriptorValue.GoalSet.NearTermGoals.push_back(CreateGoalDescriptor(
        421U, EGoalDomain::Production, EGoalHorizon::NearTerm, EGoalType::BuildProductionCapacity,
        EGoalStatus::Active, 150, 2U, UNIT_TYPEID::TERRAN_STARPORT));
    GameStateDescriptorValue.ArmyState.MinimumArmyCount = 2U;
    GameStateDescriptorValue.ArmyState.EnsurePrimaryArmyExists();
    GameStateDescriptorValue.ArmyState.ArmyGoals.front() = EArmyGoal::MapControl;
    GameStateDescriptorValue.ArmyState.ArmyMissions.front().MissionType = EArmyMissionType::SweepExpansionLocations;
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
        FGameStateDescriptor GameStateDescriptorValue = CreateGoalDrivenOpeningDescriptor();
        FBuildPlanningState BuildPlanningStateValue;
        BuildPlannerValue.ProduceBuildPlan(GameStateDescriptorValue, BuildPlanningStateValue);

        Check(BuildPlanningStateValue.DesiredSupplyDepotCount == 1U, SuccessValue,
              "Goal-driven opening should project the requested first supply depot.");
        Check(BuildPlanningStateValue.DesiredBarracksCount == 1U, SuccessValue,
              "Goal-driven opening should project the requested first barracks.");
        Check(BuildPlanningStateValue.DesiredRefineryCount == 1U, SuccessValue,
              "Goal-driven opening should project the requested first refinery.");
        Check(BuildPlanningStateValue.DesiredOrbitalCommandCount == 1U, SuccessValue,
              "Goal-driven opening should preserve orbital intent for the active main-base town hall.");

        ArmyPlannerValue.ProduceArmyPlan(GameStateDescriptorValue, GameStateDescriptorValue.ArmyState);
        Check(!GameStateDescriptorValue.ArmyState.ArmyPostures.empty() &&
                  GameStateDescriptorValue.ArmyState.ArmyPostures.front() == EArmyPosture::Assemble,
              SuccessValue, "AssembleAtRally missions should produce the Assemble army posture.");
    }

    {
        FGameStateDescriptor GameStateDescriptorValue = CreateGoalDrivenTimingDescriptor();
        FBuildPlanningState BuildPlanningStateValue;
        BuildPlannerValue.ProduceBuildPlan(GameStateDescriptorValue, BuildPlanningStateValue);

        Check(BuildPlanningStateValue.DesiredTownHallCount == 2U, SuccessValue,
              "Goal-driven timing plan should preserve the requested natural expansion count.");
        Check(BuildPlanningStateValue.DesiredOrbitalCommandCount == 2U, SuccessValue,
              "Goal-driven timing plan should preserve orbital intent for both active town halls.");
        Check(BuildPlanningStateValue.DesiredBarracksReactorCount == 1U, SuccessValue,
              "Goal-driven timing plan should project the requested barracks reactor.");
        Check(BuildPlanningStateValue.DesiredFactoryCount == 1U, SuccessValue,
              "Goal-driven timing plan should project the requested factory.");
        Check(BuildPlanningStateValue.DesiredFactoryTechLabCount == 1U, SuccessValue,
              "Goal-driven timing plan should project the requested factory tech lab.");
        Check(BuildPlanningStateValue.DesiredStarportCount == 1U, SuccessValue,
              "Goal-driven timing plan should project the requested starport.");
        Check(BuildPlanningStateValue.DesiredStarportReactorCount == 1U, SuccessValue,
              "Goal-driven timing plan should project the requested starport reactor.");
        Check(BuildPlanningStateValue.DesiredEngineeringBayCount == 1U, SuccessValue,
              "Goal-driven timing plan should project the requested engineering bay.");
        Check(BuildPlanningStateValue.DesiredHellionCount == 1U, SuccessValue,
              "Goal-driven timing plan should project the requested first hellion.");
        Check(BuildPlanningStateValue.DesiredMedivacCount == 1U, SuccessValue,
              "Goal-driven timing plan should project the requested first medivac.");
        Check(BuildPlanningStateValue.DesiredSiegeTankCount == 1U, SuccessValue,
              "Goal-driven timing plan should project the requested first siege tank.");
        Check(BuildPlanningStateValue.DesiredCompletedUpgradeCounts[GetTerranUpgradeTypeIndex(
                  UpgradeID(UPGRADE_ID::STIMPACK))] == 1U,
              SuccessValue, "Goal-driven timing plan should project the requested stimpack upgrade.");
        Check(BuildPlanningStateValue.ActiveNeedCount >= 4U, SuccessValue,
              "Outstanding need counting should include new add-on, tech-structure, and upgrade gaps.");

        ArmyPlannerValue.ProduceArmyPlan(GameStateDescriptorValue, GameStateDescriptorValue.ArmyState);
        Check(!GameStateDescriptorValue.ArmyState.ArmyPostures.empty() &&
                  GameStateDescriptorValue.ArmyState.ArmyPostures.front() == EArmyPosture::Engage,
              SuccessValue, "PressureKnownEnemyBase missions should produce the Engage army posture.");
    }

    {
        FGameStateDescriptor GameStateDescriptorValue = CreateGoalDrivenMacroDescriptor();
        FBuildPlanningState BuildPlanningStateValue;
        BuildPlannerValue.ProduceBuildPlan(GameStateDescriptorValue, BuildPlanningStateValue);

        Check(BuildPlanningStateValue.DesiredTownHallCount == 4U, SuccessValue,
              "Goal-driven macro plan should project a fourth town hall.");
        Check(BuildPlanningStateValue.DesiredWorkerCount == 66U, SuccessValue,
              "Goal-driven macro plan should project sixty-six workers.");
        Check(BuildPlanningStateValue.DesiredBarracksCount == 5U, SuccessValue,
              "Goal-driven macro plan should project five barracks.");
        Check(BuildPlanningStateValue.DesiredStarportCount == 2U, SuccessValue,
              "Goal-driven macro plan should project two starports.");

        ArmyPlannerValue.ProduceArmyPlan(GameStateDescriptorValue, GameStateDescriptorValue.ArmyState);
        Check(GameStateDescriptorValue.ArmyState.ArmyPostures.size() >= 2U, SuccessValue,
              "Army planner should preserve posture storage for multiple armies.");
        Check(GameStateDescriptorValue.ArmyState.ArmyPostures.front() == EArmyPosture::Advance, SuccessValue,
              "SweepExpansionLocations missions should produce the Advance army posture.");
        Check(GameStateDescriptorValue.ArmyState.ArmyPostures[1] == EArmyPosture::Hold, SuccessValue,
              "Secondary army posture should still default to Hold.");
    }

    return SuccessValue;
}

}  // namespace sc2
