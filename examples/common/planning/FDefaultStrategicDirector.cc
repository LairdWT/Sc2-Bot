#include "common/planning/FDefaultStrategicDirector.h"

#include <algorithm>

#include "common/armies/EArmyGoal.h"
#include "common/armies/EArmyMissionType.h"
#include "common/armies/FArmyDomainState.h"
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

constexpr uint32_t HoldOwnedBaseGoalIdValue = 100U;
constexpr uint32_t SaturateWorkersGoalIdValue = 110U;
constexpr uint32_t MaintainSupplyGoalIdValue = 120U;

constexpr uint32_t ExpandBaseCountGoalIdValue = 200U;
constexpr uint32_t RefineryGoalIdValue = 210U;
constexpr uint32_t BarracksCapacityGoalIdValue = 220U;
constexpr uint32_t FactoryCapacityGoalIdValue = 221U;
constexpr uint32_t StarportCapacityGoalIdValue = 222U;
constexpr uint32_t BarracksReactorGoalIdValue = 230U;
constexpr uint32_t FactoryTechLabGoalIdValue = 231U;
constexpr uint32_t EngineeringBayGoalIdValue = 232U;
constexpr uint32_t StarportReactorGoalIdValue = 233U;
constexpr uint32_t StimpackGoalIdValue = 240U;
constexpr uint32_t CombatShieldGoalIdValue = 241U;
constexpr uint32_t InfantryWeaponsGoalIdValue = 242U;
constexpr uint32_t ConcussiveShellsGoalIdValue = 243U;

constexpr uint32_t MarineProductionGoalIdValue = 300U;
constexpr uint32_t MarauderProductionGoalIdValue = 301U;
constexpr uint32_t CycloneProductionGoalIdValue = 302U;
constexpr uint32_t SiegeTankProductionGoalIdValue = 303U;
constexpr uint32_t MedivacProductionGoalIdValue = 304U;
constexpr uint32_t LiberatorProductionGoalIdValue = 305U;

constexpr uint32_t PressureEnemyGoalIdValue = 400U;
constexpr uint32_t ClearEnemyPresenceGoalIdValue = 410U;
constexpr uint32_t ScoutExpansionLocationsGoalIdValue = 420U;

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

bool IsGoalActive(const FGoalDescriptor& GoalDescriptorValue)
{
    switch (GoalDescriptorValue.GoalStatus)
    {
        case EGoalStatus::Active:
        case EGoalStatus::Proposed:
            return true;
        case EGoalStatus::Satisfied:
        case EGoalStatus::Blocked:
        case EGoalStatus::Abandoned:
        default:
            return false;
    }
}

uint32_t GetProjectedBuildingCount(const FGameStateDescriptor& GameStateDescriptorValue,
                                   const UNIT_TYPEID UnitTypeIdValue)
{
    return GameStateDescriptorValue.ProductionState.GetProjectedBuildingCount(UnitTypeIdValue);
}

uint32_t GetProjectedUnitCount(const FGameStateDescriptor& GameStateDescriptorValue,
                               const UNIT_TYPEID UnitTypeIdValue)
{
    return GameStateDescriptorValue.ProductionState.GetProjectedUnitCount(UnitTypeIdValue);
}

uint32_t GetProjectedUpgradeCount(const FGameStateDescriptor& GameStateDescriptorValue, const UpgradeID UpgradeIdValue)
{
    const size_t UpgradeTypeIndexValue = GetTerranUpgradeTypeIndex(UpgradeIdValue);
    return IsTerranUpgradeTypeIndexValid(UpgradeTypeIndexValue)
               ? static_cast<uint32_t>(GameStateDescriptorValue.BuildPlanning
                                           .ObservedCompletedUpgradeCounts[UpgradeTypeIndexValue]) +
                     GameStateDescriptorValue.SchedulerOutlook.GetScheduledUpgradeCount(UpgradeIdValue)
               : 0U;
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
    const FMacroStateDescriptor& MacroStateDescriptorValue = GameStateDescriptorValue.MacroState;
    const uint32_t DesiredWorkerCountValue = DetermineDesiredWorkerCount(GameStateDescriptorValue);
    const uint32_t DesiredSupplyDepotCountValue = DetermineDesiredSupplyDepotCount(GameStateDescriptorValue);
    const uint32_t ProjectedWorkerCountValue =
        GetProjectedUnitCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_SCV);
    const bool HasSupplyPressureValue =
        GameStateDescriptorValue.EconomyState.ProjectedAvailableSupplyByHorizon[ShortForecastHorizonIndexValue] <= 4U ||
        GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_SUPPLYDEPOT) <
            DesiredSupplyDepotCountValue;

    GoalDescriptorsValue.push_back(CreateGoalDescriptor(HoldOwnedBaseGoalIdValue, EGoalDomain::Defense,
                                                        EGoalHorizon::Immediate, EGoalType::HoldOwnedBase,
                                                        EGoalStatus::Active, 300));
    GoalDescriptorsValue.push_back(CreateGoalDescriptor(
        SaturateWorkersGoalIdValue, EGoalDomain::Economy, EGoalHorizon::Immediate, EGoalType::SaturateWorkers,
        ProjectedWorkerCountValue < DesiredWorkerCountValue ? EGoalStatus::Active : EGoalStatus::Satisfied,
        250, DesiredWorkerCountValue, UNIT_TYPEID::TERRAN_SCV));
    GoalDescriptorsValue.push_back(CreateGoalDescriptor(
        MaintainSupplyGoalIdValue, EGoalDomain::Economy, EGoalHorizon::Immediate, EGoalType::MaintainSupply,
        HasSupplyPressureValue ? EGoalStatus::Active : EGoalStatus::Satisfied, 240, DesiredSupplyDepotCountValue,
        UNIT_TYPEID::TERRAN_SUPPLYDEPOT));
}

void FDefaultStrategicDirector::AppendNearTermGoals(
    const FGameStateDescriptor& GameStateDescriptorValue, std::vector<FGoalDescriptor>& GoalDescriptorsValue) const
{
    const FMacroStateDescriptor& MacroStateDescriptorValue = GameStateDescriptorValue.MacroState;
    const uint32_t DesiredBaseCountValue = MacroStateDescriptorValue.DesiredBaseCount;
    const uint32_t DesiredRefineryCountValue = DetermineDesiredRefineryCount(GameStateDescriptorValue);
    const uint32_t DesiredBarracksCountValue = DetermineDesiredBarracksCount(GameStateDescriptorValue);
    const uint32_t DesiredFactoryCountValue = DetermineDesiredFactoryCount(GameStateDescriptorValue);
    const uint32_t DesiredStarportCountValue = DetermineDesiredStarportCount(GameStateDescriptorValue);

    GoalDescriptorsValue.push_back(CreateGoalDescriptor(
        ExpandBaseCountGoalIdValue, EGoalDomain::Economy, EGoalHorizon::NearTerm, EGoalType::ExpandBaseCount,
        GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_COMMANDCENTER) < DesiredBaseCountValue
            ? EGoalStatus::Active
            : EGoalStatus::Satisfied,
        230, DesiredBaseCountValue, UNIT_TYPEID::TERRAN_COMMANDCENTER));
    GoalDescriptorsValue.push_back(CreateGoalDescriptor(
        RefineryGoalIdValue, EGoalDomain::Economy, EGoalHorizon::NearTerm, EGoalType::BuildProductionCapacity,
        GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_REFINERY) < DesiredRefineryCountValue
            ? EGoalStatus::Active
            : EGoalStatus::Satisfied,
        190, DesiredRefineryCountValue, UNIT_TYPEID::TERRAN_REFINERY));
    GoalDescriptorsValue.push_back(CreateGoalDescriptor(
        BarracksCapacityGoalIdValue, EGoalDomain::Production, EGoalHorizon::NearTerm,
        EGoalType::BuildProductionCapacity,
        GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_BARRACKS) <
                DesiredBarracksCountValue
            ? EGoalStatus::Active
            : EGoalStatus::Satisfied,
        210, DesiredBarracksCountValue, UNIT_TYPEID::TERRAN_BARRACKS));
    GoalDescriptorsValue.push_back(CreateGoalDescriptor(
        FactoryCapacityGoalIdValue, EGoalDomain::Production, EGoalHorizon::NearTerm,
        EGoalType::BuildProductionCapacity,
        GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_FACTORY) < DesiredFactoryCountValue
            ? EGoalStatus::Active
            : EGoalStatus::Satisfied,
        205, DesiredFactoryCountValue, UNIT_TYPEID::TERRAN_FACTORY));
    GoalDescriptorsValue.push_back(CreateGoalDescriptor(
        StarportCapacityGoalIdValue, EGoalDomain::Production, EGoalHorizon::NearTerm,
        EGoalType::BuildProductionCapacity,
        GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_STARPORT) <
                DesiredStarportCountValue
            ? EGoalStatus::Active
            : EGoalStatus::Satisfied,
        205, DesiredStarportCountValue, UNIT_TYPEID::TERRAN_STARPORT));

    GoalDescriptorsValue.push_back(CreateGoalDescriptor(
        BarracksReactorGoalIdValue, EGoalDomain::Technology, EGoalHorizon::NearTerm, EGoalType::UnlockTechnology,
        GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_BARRACKS) >= 1U &&
                GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_BARRACKSREACTOR) < 1U
            ? EGoalStatus::Active
            : EGoalStatus::Satisfied,
        170, 1U, UNIT_TYPEID::TERRAN_BARRACKSREACTOR));
    GoalDescriptorsValue.push_back(CreateGoalDescriptor(
        FactoryTechLabGoalIdValue, EGoalDomain::Technology, EGoalHorizon::NearTerm, EGoalType::UnlockTechnology,
        GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_FACTORY) >= 1U &&
                GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_FACTORYTECHLAB) < 1U
            ? EGoalStatus::Active
            : EGoalStatus::Satisfied,
        165, 1U, UNIT_TYPEID::TERRAN_FACTORYTECHLAB));
    GoalDescriptorsValue.push_back(CreateGoalDescriptor(
        EngineeringBayGoalIdValue, EGoalDomain::Technology, EGoalHorizon::NearTerm, EGoalType::UnlockTechnology,
        ShouldPrioritizeUpgrades(GameStateDescriptorValue) &&
                GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_ENGINEERINGBAY) < 1U
            ? EGoalStatus::Active
            : EGoalStatus::Satisfied,
        150, 1U, UNIT_TYPEID::TERRAN_ENGINEERINGBAY));
    GoalDescriptorsValue.push_back(CreateGoalDescriptor(
        StarportReactorGoalIdValue, EGoalDomain::Technology, EGoalHorizon::NearTerm, EGoalType::UnlockTechnology,
        GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_STARPORT) >= 1U &&
                GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_STARPORTREACTOR) < 1U
            ? EGoalStatus::Active
            : EGoalStatus::Satisfied,
        145, 1U, UNIT_TYPEID::TERRAN_STARPORTREACTOR));

    GoalDescriptorsValue.push_back(CreateGoalDescriptor(
        StimpackGoalIdValue, EGoalDomain::Technology, EGoalHorizon::NearTerm, EGoalType::ResearchUpgrade,
        GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_BARRACKS) >= 2U &&
                GetProjectedUpgradeCount(GameStateDescriptorValue, UpgradeID(UPGRADE_ID::STIMPACK)) == 0U
            ? EGoalStatus::Active
            : EGoalStatus::Satisfied,
        160, 1U, UNIT_TYPEID::INVALID, UpgradeID(UPGRADE_ID::STIMPACK)));
    GoalDescriptorsValue.push_back(CreateGoalDescriptor(
        CombatShieldGoalIdValue, EGoalDomain::Technology, EGoalHorizon::NearTerm, EGoalType::ResearchUpgrade,
        GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_BARRACKS) >= 2U &&
                GetProjectedUpgradeCount(GameStateDescriptorValue, UpgradeID(UPGRADE_ID::SHIELDWALL)) == 0U
            ? EGoalStatus::Active
            : EGoalStatus::Satisfied,
        140, 1U, UNIT_TYPEID::INVALID, UpgradeID(UPGRADE_ID::SHIELDWALL)));
    GoalDescriptorsValue.push_back(CreateGoalDescriptor(
        InfantryWeaponsGoalIdValue, EGoalDomain::Technology, EGoalHorizon::NearTerm, EGoalType::ResearchUpgrade,
        ShouldPrioritizeUpgrades(GameStateDescriptorValue) &&
                GetProjectedUpgradeCount(GameStateDescriptorValue,
                                        UpgradeID(UPGRADE_ID::TERRANINFANTRYWEAPONSLEVEL1)) == 0U
            ? EGoalStatus::Active
            : EGoalStatus::Satisfied,
        135, 1U, UNIT_TYPEID::INVALID, UpgradeID(UPGRADE_ID::TERRANINFANTRYWEAPONSLEVEL1)));
    GoalDescriptorsValue.push_back(CreateGoalDescriptor(
        ConcussiveShellsGoalIdValue, EGoalDomain::Technology, EGoalHorizon::NearTerm, EGoalType::ResearchUpgrade,
        GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_BARRACKS) >= 2U &&
                GetProjectedUpgradeCount(GameStateDescriptorValue, UpgradeID(UPGRADE_ID::PUNISHERGRENADES)) == 0U
            ? EGoalStatus::Active
            : EGoalStatus::Satisfied,
        120, 1U, UNIT_TYPEID::INVALID, UpgradeID(UPGRADE_ID::PUNISHERGRENADES)));
}

void FDefaultStrategicDirector::AppendStrategicGoals(
    const FGameStateDescriptor& GameStateDescriptorValue, std::vector<FGoalDescriptor>& GoalDescriptorsValue) const
{
    GoalDescriptorsValue.push_back(CreateGoalDescriptor(
        MarineProductionGoalIdValue, EGoalDomain::Army, EGoalHorizon::Strategic, EGoalType::ProduceArmy,
        GetProjectedUnitCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_MARINE) <
                DetermineDesiredMarineCount(GameStateDescriptorValue)
            ? EGoalStatus::Active
            : EGoalStatus::Satisfied,
        180, DetermineDesiredMarineCount(GameStateDescriptorValue), UNIT_TYPEID::TERRAN_MARINE));
    GoalDescriptorsValue.push_back(CreateGoalDescriptor(
        MarauderProductionGoalIdValue, EGoalDomain::Army, EGoalHorizon::Strategic, EGoalType::ProduceArmy,
        GetProjectedUnitCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_MARAUDER) <
                DetermineDesiredMarauderCount(GameStateDescriptorValue)
            ? EGoalStatus::Active
            : EGoalStatus::Satisfied,
        165, DetermineDesiredMarauderCount(GameStateDescriptorValue), UNIT_TYPEID::TERRAN_MARAUDER));
    GoalDescriptorsValue.push_back(CreateGoalDescriptor(
        CycloneProductionGoalIdValue, EGoalDomain::Army, EGoalHorizon::Strategic, EGoalType::ProduceArmy,
        GetProjectedUnitCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_CYCLONE) <
                DetermineDesiredCycloneCount(GameStateDescriptorValue)
            ? EGoalStatus::Active
            : EGoalStatus::Satisfied,
        150, DetermineDesiredCycloneCount(GameStateDescriptorValue), UNIT_TYPEID::TERRAN_CYCLONE));
    GoalDescriptorsValue.push_back(CreateGoalDescriptor(
        SiegeTankProductionGoalIdValue, EGoalDomain::Army, EGoalHorizon::Strategic, EGoalType::ProduceArmy,
        GetProjectedUnitCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_SIEGETANK) <
                DetermineDesiredSiegeTankCount(GameStateDescriptorValue)
            ? EGoalStatus::Active
            : EGoalStatus::Satisfied,
        160, DetermineDesiredSiegeTankCount(GameStateDescriptorValue), UNIT_TYPEID::TERRAN_SIEGETANK));
    GoalDescriptorsValue.push_back(CreateGoalDescriptor(
        MedivacProductionGoalIdValue, EGoalDomain::Army, EGoalHorizon::Strategic, EGoalType::ProduceArmy,
        GetProjectedUnitCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_MEDIVAC) <
                DetermineDesiredMedivacCount(GameStateDescriptorValue)
            ? EGoalStatus::Active
            : EGoalStatus::Satisfied,
        170, DetermineDesiredMedivacCount(GameStateDescriptorValue), UNIT_TYPEID::TERRAN_MEDIVAC));
    GoalDescriptorsValue.push_back(CreateGoalDescriptor(
        LiberatorProductionGoalIdValue, EGoalDomain::Army, EGoalHorizon::Strategic, EGoalType::ProduceArmy,
        GetProjectedUnitCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_LIBERATOR) <
                DetermineDesiredLiberatorCount(GameStateDescriptorValue)
            ? EGoalStatus::Active
            : EGoalStatus::Satisfied,
        130, DetermineDesiredLiberatorCount(GameStateDescriptorValue), UNIT_TYPEID::TERRAN_LIBERATOR));

    GoalDescriptorsValue.push_back(CreateGoalDescriptor(PressureEnemyGoalIdValue, EGoalDomain::Army,
                                                        EGoalHorizon::Strategic, EGoalType::PressureEnemy,
                                                        EGoalStatus::Active, 125));
    GoalDescriptorsValue.push_back(CreateGoalDescriptor(ClearEnemyPresenceGoalIdValue, EGoalDomain::Army,
                                                        EGoalHorizon::Strategic, EGoalType::ClearEnemyPresence,
                                                        EGoalStatus::Active, 120));
    GoalDescriptorsValue.push_back(CreateGoalDescriptor(ScoutExpansionLocationsGoalIdValue, EGoalDomain::Scouting,
                                                        EGoalHorizon::Strategic,
                                                        EGoalType::ScoutExpansionLocations,
                                                        EGoalStatus::Active, 110));
}

EProductionFocus FDefaultStrategicDirector::DeterminePrimaryProductionFocus(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    const FMacroStateDescriptor& MacroStateDescriptorValue = GameStateDescriptorValue.MacroState;
    const FEconomyStateDescriptor& EconomyStateDescriptorValue = GameStateDescriptorValue.EconomyState;
    const FProductionStateDescriptor& ProductionStateDescriptorValue = GameStateDescriptorValue.ProductionState;
    const uint32_t ProjectedWorkerCountValue =
        ProductionStateDescriptorValue.GetProjectedUnitCount(UNIT_TYPEID::TERRAN_SCV);
    const uint32_t ProjectedBaseCountValue =
        ProductionStateDescriptorValue.GetProjectedBuildingCount(UNIT_TYPEID::TERRAN_COMMANDCENTER);

    if (MacroStateDescriptorValue.ActiveMacroPhase == EMacroPhase::Recovery)
    {
        return EProductionFocus::Recovery;
    }

    if (!GameStateDescriptorValue.ArmyState.ArmyMissions.empty())
    {
        switch (GameStateDescriptorValue.ArmyState.ArmyMissions.front().MissionType)
        {
            case EArmyMissionType::DefendOwnedBase:
            case EArmyMissionType::Regroup:
                return EProductionFocus::Defense;
            default:
                break;
        }
    }

    if (EconomyStateDescriptorValue.ProjectedAvailableSupplyByHorizon[ShortForecastHorizonIndexValue] <= 1U)
    {
        return EProductionFocus::Supply;
    }

    const uint32_t DesiredBaseCountValue = DetermineDesiredBaseCount(GameStateDescriptorValue);
    if (ProjectedBaseCountValue < DesiredBaseCountValue &&
        ProjectedWorkerCountValue >= (std::max<uint32_t>(1U, ProjectedBaseCountValue) * 16U))
    {
        const uint32_t ExpansionArmyThresholdValue = DesiredBaseCountValue <= 2U ? 12U : 30U;
        if (MacroStateDescriptorValue.ArmySupply >= ExpansionArmyThresholdValue)
        {
            return EProductionFocus::Expansion;
        }
    }

    if (ProjectedWorkerCountValue < DetermineDesiredWorkerCount(GameStateDescriptorValue))
    {
        return EProductionFocus::Workers;
    }

    if (EconomyStateDescriptorValue.ProjectedAvailableMineralsByHorizon[ShortForecastHorizonIndexValue] >= 500U)
    {
        return EProductionFocus::Production;
    }

    if (ShouldPrioritizeUpgrades(GameStateDescriptorValue))
    {
        return EProductionFocus::Upgrades;
    }

    return EProductionFocus::Army;
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

uint32_t FDefaultStrategicDirector::DetermineDesiredWorkerCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    const uint32_t DesiredRefineryCountValue = DetermineDesiredRefineryCount(GameStateDescriptorValue);
    const uint32_t DesiredWorkerCountValue =
        (DetermineDesiredBaseCount(GameStateDescriptorValue) * 16U) + (DesiredRefineryCountValue * 3U);
    return std::min<uint32_t>(72U, std::max<uint32_t>(12U, DesiredWorkerCountValue));
}

uint32_t FDefaultStrategicDirector::DetermineDesiredRefineryCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    switch (GameStateDescriptorValue.MacroState.ActiveMacroPhase)
    {
        case EMacroPhase::Opening:
            return 1U;
        case EMacroPhase::EarlyGame:
            return 2U;
        case EMacroPhase::MidGame:
            return 4U;
        case EMacroPhase::LateGame:
            return 8U;
        case EMacroPhase::Recovery:
            return std::min<uint32_t>(1U, GetProjectedBuildingCount(GameStateDescriptorValue,
                                                                    UNIT_TYPEID::TERRAN_REFINERY));
        default:
            return 2U;
    }
}

uint32_t FDefaultStrategicDirector::DetermineDesiredSupplyDepotCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    const uint32_t DesiredWorkerCountValue = DetermineDesiredWorkerCount(GameStateDescriptorValue);
    const uint32_t DesiredSupplyUsedValue =
        DesiredWorkerCountValue + DetermineDesiredMarineCount(GameStateDescriptorValue) +
        DetermineDesiredMarauderCount(GameStateDescriptorValue) +
        (DetermineDesiredCycloneCount(GameStateDescriptorValue) * 3U) +
        (DetermineDesiredSiegeTankCount(GameStateDescriptorValue) * 3U) +
        (DetermineDesiredMedivacCount(GameStateDescriptorValue) * 2U) +
        (DetermineDesiredLiberatorCount(GameStateDescriptorValue) * 3U);
    return std::max<uint32_t>(2U, (DesiredSupplyUsedValue + 7U) / 8U);
}

uint32_t FDefaultStrategicDirector::DetermineDesiredBarracksCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    switch (GameStateDescriptorValue.MacroState.ActiveMacroPhase)
    {
        case EMacroPhase::Opening:
            return 1U;
        case EMacroPhase::EarlyGame:
            return 2U;
        case EMacroPhase::MidGame:
            return 3U;
        case EMacroPhase::LateGame:
            return 5U;
        case EMacroPhase::Recovery:
            return std::max<uint32_t>(1U, GetProjectedBuildingCount(GameStateDescriptorValue,
                                                                    UNIT_TYPEID::TERRAN_BARRACKS));
        default:
            return 2U;
    }
}

uint32_t FDefaultStrategicDirector::DetermineDesiredFactoryCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    switch (GameStateDescriptorValue.MacroState.ActiveMacroPhase)
    {
        case EMacroPhase::Opening:
            return 0U;
        case EMacroPhase::EarlyGame:
        case EMacroPhase::MidGame:
            return 1U;
        case EMacroPhase::LateGame:
            return 2U;
        case EMacroPhase::Recovery:
            return std::min<uint32_t>(1U, GetProjectedBuildingCount(GameStateDescriptorValue,
                                                                    UNIT_TYPEID::TERRAN_FACTORY));
        default:
            return 1U;
    }
}

uint32_t FDefaultStrategicDirector::DetermineDesiredStarportCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    switch (GameStateDescriptorValue.MacroState.ActiveMacroPhase)
    {
        case EMacroPhase::Opening:
            return 0U;
        case EMacroPhase::EarlyGame:
        case EMacroPhase::MidGame:
            return 1U;
        case EMacroPhase::LateGame:
            return 2U;
        case EMacroPhase::Recovery:
            return std::min<uint32_t>(1U, GetProjectedBuildingCount(GameStateDescriptorValue,
                                                                    UNIT_TYPEID::TERRAN_STARPORT));
        default:
            return 1U;
    }
}

uint32_t FDefaultStrategicDirector::DetermineDesiredMarineCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    switch (GameStateDescriptorValue.MacroState.ActiveMacroPhase)
    {
        case EMacroPhase::Opening:
            return 6U;
        case EMacroPhase::EarlyGame:
            return 16U;
        case EMacroPhase::MidGame:
            return 32U;
        case EMacroPhase::LateGame:
            return 60U;
        case EMacroPhase::Recovery:
            return std::max<uint32_t>(4U, GameStateDescriptorValue.MacroState.ArmyUnitCount);
        default:
            return 16U;
    }
}

uint32_t FDefaultStrategicDirector::DetermineDesiredMarauderCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    switch (GameStateDescriptorValue.MacroState.ActiveMacroPhase)
    {
        case EMacroPhase::Opening:
            return 0U;
        case EMacroPhase::EarlyGame:
            return 2U;
        case EMacroPhase::MidGame:
            return 6U;
        case EMacroPhase::LateGame:
            return 12U;
        case EMacroPhase::Recovery:
            return 0U;
        default:
            return 2U;
    }
}

uint32_t FDefaultStrategicDirector::DetermineDesiredCycloneCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    switch (GameStateDescriptorValue.MacroState.ActiveMacroPhase)
    {
        case EMacroPhase::Opening:
        case EMacroPhase::Recovery:
            return 0U;
        case EMacroPhase::EarlyGame:
        case EMacroPhase::MidGame:
            return 1U;
        case EMacroPhase::LateGame:
            return 2U;
        default:
            return 1U;
    }
}

uint32_t FDefaultStrategicDirector::DetermineDesiredSiegeTankCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    switch (GameStateDescriptorValue.MacroState.ActiveMacroPhase)
    {
        case EMacroPhase::Opening:
            return 0U;
        case EMacroPhase::EarlyGame:
        case EMacroPhase::MidGame:
            return 1U;
        case EMacroPhase::LateGame:
            return 4U;
        case EMacroPhase::Recovery:
            return 0U;
        default:
            return 1U;
    }
}

uint32_t FDefaultStrategicDirector::DetermineDesiredMedivacCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    switch (GameStateDescriptorValue.MacroState.ActiveMacroPhase)
    {
        case EMacroPhase::Opening:
            return 0U;
        case EMacroPhase::EarlyGame:
            return 1U;
        case EMacroPhase::MidGame:
            return 2U;
        case EMacroPhase::LateGame:
            return 8U;
        case EMacroPhase::Recovery:
            return 0U;
        default:
            return 1U;
    }
}

uint32_t FDefaultStrategicDirector::DetermineDesiredLiberatorCount(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    switch (GameStateDescriptorValue.MacroState.ActiveMacroPhase)
    {
        case EMacroPhase::Opening:
        case EMacroPhase::Recovery:
            return 0U;
        case EMacroPhase::EarlyGame:
        case EMacroPhase::MidGame:
            return 1U;
        case EMacroPhase::LateGame:
            return 4U;
        default:
            return 1U;
    }
}

bool FDefaultStrategicDirector::ShouldPrioritizeUpgrades(const FGameStateDescriptor& GameStateDescriptorValue) const
{
    return GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_COMMANDCENTER) >= 2U &&
           GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_BARRACKS) >= 2U;
}

EArmyGoal FDefaultStrategicDirector::DeterminePrimaryArmyGoal(
    const FGameStateDescriptor& GameStateDescriptorValue) const
{
    const FAgentGoalSetDescriptor& AgentGoalSetDescriptorValue = GameStateDescriptorValue.GoalSet;
    for (const FGoalDescriptor& GoalDescriptorValue : AgentGoalSetDescriptorValue.ImmediateGoals)
    {
        if (!IsGoalActive(GoalDescriptorValue))
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
        if (!IsGoalActive(GoalDescriptorValue))
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
