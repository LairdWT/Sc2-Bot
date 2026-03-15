#include "common/catalogs/FTerranGoalRuleLibrary.h"

#include <algorithm>

#include "common/armies/EArmyMissionType.h"
#include "common/catalogs/EGoalActivationRuleId.h"
#include "common/catalogs/EGoalTargetRuleId.h"
#include "common/catalogs/FTerranGoalDefinition.h"
#include "common/descriptors/FExecutionPressureDescriptor.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/descriptors/FMacroStateDescriptor.h"
#include "common/economy/EconomyForecastConstants.h"
#include "common/goals/FGoalDescriptor.h"
#include "common/telemetry/EExecutionConditionState.h"

namespace sc2
{
namespace
{

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

bool HasSustainedMineralFloat(const FExecutionPressureDescriptor& ExecutionPressureDescriptorValue)
{
    if (ExecutionPressureDescriptorValue.MineralBankState != EExecutionConditionState::Active)
    {
        return false;
    }

    return ExecutionPressureDescriptorValue.CurrentMineralBankAmount >= 800U ||
           (ExecutionPressureDescriptorValue.CurrentMineralBankAmount >= 500U &&
            ExecutionPressureDescriptorValue.MineralBankDurationGameLoops >=
                ForecastHorizonGameLoopsValue[ShortForecastHorizonIndexValue]);
}

bool HasIdleTownHallPressure(const FExecutionPressureDescriptor& ExecutionPressureDescriptorValue)
{
    return ExecutionPressureDescriptorValue.IdleTownHallCount > 0U;
}

bool HasIdleCombatProductionPressure(const FExecutionPressureDescriptor& ExecutionPressureDescriptorValue)
{
    return ExecutionPressureDescriptorValue.GetIdleCombatProductionStructureCount() > 0U ||
           ExecutionPressureDescriptorValue.RecentIdleProductionConflictCount > 0U;
}

}  // namespace

bool FTerranGoalRuleLibrary::IsGoalActive(const FGoalDescriptor& GoalDescriptorValue)
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

EGoalStatus FTerranGoalRuleLibrary::EvaluateGoalStatus(const FTerranGoalDefinition& TerranGoalDefinitionValue,
                                                       const FGameStateDescriptor& GameStateDescriptorValue)
{
    switch (TerranGoalDefinitionValue.ActivationRuleId)
    {
        case EGoalActivationRuleId::AlwaysActive:
            return EGoalStatus::Active;
        case EGoalActivationRuleId::ProjectedWorkersBelowTarget:
            return GetProjectedUnitCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_SCV) <
                           DetermineDesiredWorkerCount(GameStateDescriptorValue)
                       ? EGoalStatus::Active
                       : EGoalStatus::Satisfied;
        case EGoalActivationRuleId::SupplyPressureOrProjectedDepotsBelowTarget:
            return (GameStateDescriptorValue.EconomyState.GetProjectedDiscretionarySupplyAtHorizon(
                        GameStateDescriptorValue.CommitmentLedger, ShortForecastHorizonIndexValue) <= 4U ||
                    GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_SUPPLYDEPOT) <
                        DetermineDesiredSupplyDepotCount(GameStateDescriptorValue))
                       ? EGoalStatus::Active
                       : EGoalStatus::Satisfied;
        case EGoalActivationRuleId::ProjectedCommandCentersBelowTarget:
            return GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_COMMANDCENTER) <
                           DetermineDesiredBaseCount(GameStateDescriptorValue)
                       ? EGoalStatus::Active
                       : EGoalStatus::Satisfied;
        case EGoalActivationRuleId::ProjectedRefineriesBelowTarget:
            return GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_REFINERY) <
                           DetermineDesiredRefineryCount(GameStateDescriptorValue)
                       ? EGoalStatus::Active
                       : EGoalStatus::Satisfied;
        case EGoalActivationRuleId::ProjectedBarracksBelowTarget:
            return GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_BARRACKS) <
                           DetermineDesiredBarracksCount(GameStateDescriptorValue)
                       ? EGoalStatus::Active
                       : EGoalStatus::Satisfied;
        case EGoalActivationRuleId::ProjectedFactoryBelowTarget:
            return GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_FACTORY) <
                           DetermineDesiredFactoryCount(GameStateDescriptorValue)
                       ? EGoalStatus::Active
                       : EGoalStatus::Satisfied;
        case EGoalActivationRuleId::ProjectedStarportBelowTarget:
            return GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_STARPORT) <
                           DetermineDesiredStarportCount(GameStateDescriptorValue)
                       ? EGoalStatus::Active
                       : EGoalStatus::Satisfied;
        case EGoalActivationRuleId::MissingBarracksReactor:
            return (GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_BARRACKS) >= 1U &&
                    GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_BARRACKSREACTOR) < 1U)
                       ? EGoalStatus::Active
                       : EGoalStatus::Satisfied;
        case EGoalActivationRuleId::MissingFactoryTechLab:
            return (GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_FACTORY) >= 1U &&
                    GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_FACTORYTECHLAB) < 1U)
                       ? EGoalStatus::Active
                       : EGoalStatus::Satisfied;
        case EGoalActivationRuleId::MissingEngineeringBayForUpgrades:
            return (ShouldPrioritizeUpgrades(GameStateDescriptorValue) &&
                    GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_ENGINEERINGBAY) < 1U)
                       ? EGoalStatus::Active
                       : EGoalStatus::Satisfied;
        case EGoalActivationRuleId::MissingStarportReactor:
            return (GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_STARPORT) >= 1U &&
                    GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_STARPORTREACTOR) < 1U)
                       ? EGoalStatus::Active
                       : EGoalStatus::Satisfied;
        case EGoalActivationRuleId::MissingStimpack:
            return (GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_BARRACKS) >= 2U &&
                    GetProjectedUpgradeCount(GameStateDescriptorValue, UpgradeID(UPGRADE_ID::STIMPACK)) == 0U)
                       ? EGoalStatus::Active
                       : EGoalStatus::Satisfied;
        case EGoalActivationRuleId::MissingCombatShield:
            return (GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_BARRACKS) >= 2U &&
                    GetProjectedUpgradeCount(GameStateDescriptorValue, UpgradeID(UPGRADE_ID::SHIELDWALL)) == 0U)
                       ? EGoalStatus::Active
                       : EGoalStatus::Satisfied;
        case EGoalActivationRuleId::MissingInfantryWeaponsLevel1:
            return (ShouldPrioritizeUpgrades(GameStateDescriptorValue) &&
                    GetProjectedUpgradeCount(GameStateDescriptorValue,
                                             UpgradeID(UPGRADE_ID::TERRANINFANTRYWEAPONSLEVEL1)) == 0U)
                       ? EGoalStatus::Active
                       : EGoalStatus::Satisfied;
        case EGoalActivationRuleId::MissingConcussiveShells:
            return (GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_BARRACKS) >= 2U &&
                    GetProjectedUpgradeCount(GameStateDescriptorValue, UpgradeID(UPGRADE_ID::PUNISHERGRENADES)) == 0U)
                       ? EGoalStatus::Active
                       : EGoalStatus::Satisfied;
        case EGoalActivationRuleId::ProjectedMarinesBelowTarget:
            return GetProjectedUnitCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_MARINE) <
                           DetermineDesiredMarineCount(GameStateDescriptorValue)
                       ? EGoalStatus::Active
                       : EGoalStatus::Satisfied;
        case EGoalActivationRuleId::ProjectedMaraudersBelowTarget:
            return GetProjectedUnitCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_MARAUDER) <
                           DetermineDesiredMarauderCount(GameStateDescriptorValue)
                       ? EGoalStatus::Active
                       : EGoalStatus::Satisfied;
        case EGoalActivationRuleId::ProjectedCyclonesBelowTarget:
            return GetProjectedUnitCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_CYCLONE) <
                           DetermineDesiredCycloneCount(GameStateDescriptorValue)
                       ? EGoalStatus::Active
                       : EGoalStatus::Satisfied;
        case EGoalActivationRuleId::ProjectedSiegeTanksBelowTarget:
            return GetProjectedUnitCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_SIEGETANK) <
                           DetermineDesiredSiegeTankCount(GameStateDescriptorValue)
                       ? EGoalStatus::Active
                       : EGoalStatus::Satisfied;
        case EGoalActivationRuleId::ProjectedMedivacsBelowTarget:
            return GetProjectedUnitCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_MEDIVAC) <
                           DetermineDesiredMedivacCount(GameStateDescriptorValue)
                       ? EGoalStatus::Active
                       : EGoalStatus::Satisfied;
        case EGoalActivationRuleId::ProjectedLiberatorsBelowTarget:
            return GetProjectedUnitCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_LIBERATOR) <
                           DetermineDesiredLiberatorCount(GameStateDescriptorValue)
                       ? EGoalStatus::Active
                       : EGoalStatus::Satisfied;
        case EGoalActivationRuleId::Invalid:
        default:
            return EGoalStatus::Blocked;
    }
}

uint32_t FTerranGoalRuleLibrary::EvaluateGoalTargetCount(const FTerranGoalDefinition& TerranGoalDefinitionValue,
                                                         const FGameStateDescriptor& GameStateDescriptorValue)
{
    switch (TerranGoalDefinitionValue.TargetRuleId)
    {
        case EGoalTargetRuleId::None:
            return 0U;
        case EGoalTargetRuleId::DefaultTargetCount:
            return TerranGoalDefinitionValue.DefaultTargetCount;
        case EGoalTargetRuleId::DesiredWorkerCount:
            return DetermineDesiredWorkerCount(GameStateDescriptorValue);
        case EGoalTargetRuleId::DesiredSupplyDepotCount:
            return DetermineDesiredSupplyDepotCount(GameStateDescriptorValue);
        case EGoalTargetRuleId::DesiredBaseCount:
            return DetermineDesiredBaseCount(GameStateDescriptorValue);
        case EGoalTargetRuleId::DesiredRefineryCount:
            return DetermineDesiredRefineryCount(GameStateDescriptorValue);
        case EGoalTargetRuleId::DesiredBarracksCount:
            return DetermineDesiredBarracksCount(GameStateDescriptorValue);
        case EGoalTargetRuleId::DesiredFactoryCount:
            return DetermineDesiredFactoryCount(GameStateDescriptorValue);
        case EGoalTargetRuleId::DesiredStarportCount:
            return DetermineDesiredStarportCount(GameStateDescriptorValue);
        case EGoalTargetRuleId::DesiredMarineCount:
            return DetermineDesiredMarineCount(GameStateDescriptorValue);
        case EGoalTargetRuleId::DesiredMarauderCount:
            return DetermineDesiredMarauderCount(GameStateDescriptorValue);
        case EGoalTargetRuleId::DesiredCycloneCount:
            return DetermineDesiredCycloneCount(GameStateDescriptorValue);
        case EGoalTargetRuleId::DesiredSiegeTankCount:
            return DetermineDesiredSiegeTankCount(GameStateDescriptorValue);
        case EGoalTargetRuleId::DesiredMedivacCount:
            return DetermineDesiredMedivacCount(GameStateDescriptorValue);
        case EGoalTargetRuleId::DesiredLiberatorCount:
            return DetermineDesiredLiberatorCount(GameStateDescriptorValue);
        case EGoalTargetRuleId::Invalid:
        default:
            return TerranGoalDefinitionValue.DefaultTargetCount;
    }
}

uint32_t FTerranGoalRuleLibrary::DetermineDesiredArmyCount(const FGameStateDescriptor& GameStateDescriptorValue)
{
    if (GameStateDescriptorValue.MacroState.ActiveBaseCount >= 3U ||
        GameStateDescriptorValue.MacroState.ArmySupply >= 100U)
    {
        return 2U;
    }

    return 1U;
}

uint32_t FTerranGoalRuleLibrary::DetermineDesiredBaseCount(const FGameStateDescriptor& GameStateDescriptorValue)
{
    switch (GameStateDescriptorValue.MacroState.ActiveMacroPhase)
    {
        case EMacroPhase::Opening:
        case EMacroPhase::EarlyGame:
            return 2U;
        case EMacroPhase::MidGame:
            if (GameStateDescriptorValue.MacroState.ActiveBaseCount >= 3U &&
                GameStateDescriptorValue.MacroState.ArmySupply >= 80U)
            {
                return 4U;
            }
            return 3U;
        case EMacroPhase::LateGame:
            return 4U;
        case EMacroPhase::Recovery:
            return std::max<uint32_t>(1U, GameStateDescriptorValue.MacroState.ActiveBaseCount);
        default:
            return std::max<uint32_t>(1U, GameStateDescriptorValue.MacroState.ActiveBaseCount);
    }
}

uint32_t FTerranGoalRuleLibrary::DetermineDesiredWorkerCount(const FGameStateDescriptor& GameStateDescriptorValue)
{
    const uint32_t DesiredRefineryCountValue = DetermineDesiredRefineryCount(GameStateDescriptorValue);
    const uint32_t DesiredWorkerCountValue =
        (DetermineDesiredBaseCount(GameStateDescriptorValue) * 16U) + (DesiredRefineryCountValue * 3U);
    return std::min<uint32_t>(72U, std::max<uint32_t>(12U, DesiredWorkerCountValue));
}

uint32_t FTerranGoalRuleLibrary::DetermineDesiredRefineryCount(const FGameStateDescriptor& GameStateDescriptorValue)
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

uint32_t FTerranGoalRuleLibrary::DetermineDesiredSupplyDepotCount(
    const FGameStateDescriptor& GameStateDescriptorValue)
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

uint32_t FTerranGoalRuleLibrary::DetermineDesiredBarracksCount(const FGameStateDescriptor& GameStateDescriptorValue)
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

uint32_t FTerranGoalRuleLibrary::DetermineDesiredFactoryCount(const FGameStateDescriptor& GameStateDescriptorValue)
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

uint32_t FTerranGoalRuleLibrary::DetermineDesiredStarportCount(const FGameStateDescriptor& GameStateDescriptorValue)
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

uint32_t FTerranGoalRuleLibrary::DetermineDesiredMarineCount(const FGameStateDescriptor& GameStateDescriptorValue)
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

uint32_t FTerranGoalRuleLibrary::DetermineDesiredMarauderCount(const FGameStateDescriptor& GameStateDescriptorValue)
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

uint32_t FTerranGoalRuleLibrary::DetermineDesiredCycloneCount(const FGameStateDescriptor& GameStateDescriptorValue)
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

uint32_t FTerranGoalRuleLibrary::DetermineDesiredSiegeTankCount(const FGameStateDescriptor& GameStateDescriptorValue)
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

uint32_t FTerranGoalRuleLibrary::DetermineDesiredMedivacCount(const FGameStateDescriptor& GameStateDescriptorValue)
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

uint32_t FTerranGoalRuleLibrary::DetermineDesiredLiberatorCount(const FGameStateDescriptor& GameStateDescriptorValue)
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

bool FTerranGoalRuleLibrary::ShouldPrioritizeUpgrades(const FGameStateDescriptor& GameStateDescriptorValue)
{
    return GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_COMMANDCENTER) >= 2U &&
           GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_BARRACKS) >= 2U &&
           GameStateDescriptorValue.EconomyState.GetProjectedDiscretionaryMineralsAtHorizon(
               GameStateDescriptorValue.CommitmentLedger, ShortForecastHorizonIndexValue) >= 100U;
}

EProductionFocus FTerranGoalRuleLibrary::DeterminePrimaryProductionFocus(
    const FGameStateDescriptor& GameStateDescriptorValue)
{
    const FMacroStateDescriptor& MacroStateDescriptorValue = GameStateDescriptorValue.MacroState;
    const FEconomyStateDescriptor& EconomyStateDescriptorValue = GameStateDescriptorValue.EconomyState;
    const FExecutionPressureDescriptor& ExecutionPressureDescriptorValue = GameStateDescriptorValue.ExecutionPressure;
    const uint32_t ProjectedWorkerCountValue =
        GameStateDescriptorValue.ProductionState.GetProjectedUnitCount(UNIT_TYPEID::TERRAN_SCV);
    const uint32_t ProjectedBaseCountValue =
        GameStateDescriptorValue.ProductionState.GetProjectedBuildingCount(UNIT_TYPEID::TERRAN_COMMANDCENTER);
    const bool HasSustainedMineralFloatValue = HasSustainedMineralFloat(ExecutionPressureDescriptorValue);
    const bool HasIdleTownHallPressureValue = HasIdleTownHallPressure(ExecutionPressureDescriptorValue);
    const bool HasIdleCombatProductionPressureValue =
        HasIdleCombatProductionPressure(ExecutionPressureDescriptorValue);

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

    if (EconomyStateDescriptorValue.GetProjectedDiscretionarySupplyAtHorizon(
            GameStateDescriptorValue.CommitmentLedger, ShortForecastHorizonIndexValue) <= 1U)
    {
        return EProductionFocus::Supply;
    }

    const uint32_t DesiredBaseCountValue = DetermineDesiredBaseCount(GameStateDescriptorValue);
    if (ProjectedBaseCountValue < DesiredBaseCountValue &&
        ProjectedWorkerCountValue >= (std::max<uint32_t>(1U, ProjectedBaseCountValue) * 16U) &&
        (HasSustainedMineralFloatValue || HasIdleTownHallPressureValue))
    {
        const uint32_t ExpansionArmyThresholdValue = DesiredBaseCountValue <= 2U ? 12U : 30U;
        if (MacroStateDescriptorValue.ArmySupply >= ExpansionArmyThresholdValue)
        {
            return EProductionFocus::Expansion;
        }
    }

    if (ProjectedWorkerCountValue < DetermineDesiredWorkerCount(GameStateDescriptorValue) &&
        (HasIdleTownHallPressureValue || HasSustainedMineralFloatValue ||
         ProjectedWorkerCountValue + 4U < DetermineDesiredWorkerCount(GameStateDescriptorValue)))
    {
        return EProductionFocus::Workers;
    }

    if (HasIdleCombatProductionPressureValue)
    {
        if (ShouldPrioritizeUpgrades(GameStateDescriptorValue) &&
            GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_ENGINEERINGBAY) > 0U)
        {
            return EProductionFocus::Upgrades;
        }

        return EProductionFocus::Army;
    }

    if (HasSustainedMineralFloatValue)
    {
        if (ShouldPrioritizeUpgrades(GameStateDescriptorValue))
        {
            return EProductionFocus::Upgrades;
        }

        return EProductionFocus::Production;
    }

    if (EconomyStateDescriptorValue.GetProjectedDiscretionaryMineralsAtHorizon(
            GameStateDescriptorValue.CommitmentLedger, ShortForecastHorizonIndexValue) >= 500U)
    {
        return EProductionFocus::Production;
    }

    if (ShouldPrioritizeUpgrades(GameStateDescriptorValue))
    {
        return EProductionFocus::Upgrades;
    }

    return EProductionFocus::Army;
}

EGamePlan FTerranGoalRuleLibrary::DetermineGamePlan(const FGameStateDescriptor& GameStateDescriptorValue)
{
    if (GameStateDescriptorValue.MacroState.ActiveMacroPhase == EMacroPhase::Recovery)
    {
        return EGamePlan::Recovery;
    }

    if (GameStateDescriptorValue.MacroState.ActiveBaseCount >= 3U)
    {
        return EGamePlan::Macro;
    }

    return EGamePlan::TimingAttack;
}

}  // namespace sc2
