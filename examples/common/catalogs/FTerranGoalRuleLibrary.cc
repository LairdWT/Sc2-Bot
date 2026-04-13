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

bool HasNearTermSupplyPressure(const FGameStateDescriptor& GameStateDescriptorValue)
{
    return GameStateDescriptorValue.ExecutionPressure.SupplyBlockState == EExecutionConditionState::Active ||
           GameStateDescriptorValue.EconomyState.ProjectedAvailableSupplyByHorizon[ShortForecastHorizonIndexValue] <=
               2U ||
           GameStateDescriptorValue.EconomyState.GetProjectedDiscretionarySupplyAtHorizon(
               GameStateDescriptorValue.CommitmentLedger, ShortForecastHorizonIndexValue) <= 2U;
}

uint32_t GetShortHorizonDiscretionaryMinerals(const FGameStateDescriptor& GameStateDescriptorValue)
{
    return GameStateDescriptorValue.EconomyState.GetProjectedDiscretionaryMineralsAtHorizon(
        GameStateDescriptorValue.CommitmentLedger, ShortForecastHorizonIndexValue);
}

uint32_t GetCappedScheduledSupplyUsageDelta(const FGameStateDescriptor& GameStateDescriptorValue)
{
    uint32_t MaximumScheduledSupplyUsageDeltaValue = 4U;

    switch (GameStateDescriptorValue.MacroState.ActiveMacroPhase)
    {
        case EMacroPhase::Opening:
        case EMacroPhase::Recovery:
            MaximumScheduledSupplyUsageDeltaValue = 1U;
            break;
        case EMacroPhase::EarlyGame:
            MaximumScheduledSupplyUsageDeltaValue = 2U;
            break;
        case EMacroPhase::MidGame:
            MaximumScheduledSupplyUsageDeltaValue = 8U;
            break;
        case EMacroPhase::LateGame:
            MaximumScheduledSupplyUsageDeltaValue = 12U;
            break;
        default:
            break;
    }

    return std::min<uint32_t>(GameStateDescriptorValue.SchedulerOutlook.ExpectedSupplyUsedDelta,
                              MaximumScheduledSupplyUsageDeltaValue);
}

uint32_t GetSupplyBufferForMacroPhase(const EMacroPhase MacroPhaseValue)
{
    switch (MacroPhaseValue)
    {
        case EMacroPhase::Opening:
        case EMacroPhase::Recovery:
            return 2U;
        case EMacroPhase::EarlyGame:
            return 4U;
        case EMacroPhase::MidGame:
            return 16U;
        case EMacroPhase::LateGame:
            return 24U;
        default:
            return 4U;
    }
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
            return (HasNearTermSupplyPressure(GameStateDescriptorValue) &&
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
        case EGoalActivationRuleId::MissingBarracksTechLab:
            return (GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_BARRACKS) >= 2U &&
                    GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_BARRACKSTECHLAB) < 1U)
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
        case EGoalActivationRuleId::MissingSecondEngineeringBayForUpgrades:
            return (ShouldPrioritizeUpgrades(GameStateDescriptorValue) &&
                    (GameStateDescriptorValue.MacroState.ActiveBaseCount >= 3U ||
                     GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_BARRACKS) >= 3U ||
                     HasSustainedMineralFloat(GameStateDescriptorValue.ExecutionPressure) ||
                     (GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_BARRACKS) >= 2U &&
                      GetShortHorizonDiscretionaryMinerals(GameStateDescriptorValue) >= 250U)) &&
                    GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_ENGINEERINGBAY) < 2U)
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
        case EGoalActivationRuleId::MissingInfantryArmorLevel1:
            return (ShouldPrioritizeUpgrades(GameStateDescriptorValue) &&
                    GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_ENGINEERINGBAY) >= 2U &&
                    GetProjectedUpgradeCount(GameStateDescriptorValue,
                                             UpgradeID(UPGRADE_ID::TERRANINFANTRYARMORSLEVEL1)) == 0U)
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
        case EGoalActivationRuleId::ProjectedOrbitalsBelowDesiredCount:
            return GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_ORBITALCOMMAND) <
                           DetermineDesiredOrbitalCount(GameStateDescriptorValue)
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
        case EGoalTargetRuleId::DesiredOrbitalCount:
            return DetermineDesiredOrbitalCount(GameStateDescriptorValue);
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
    const uint32_t ShortHorizonDiscretionaryMineralsValue =
        GetShortHorizonDiscretionaryMinerals(GameStateDescriptorValue);

    switch (GameStateDescriptorValue.MacroState.ActiveMacroPhase)
    {
        case EMacroPhase::Opening:
            return 2U;
        case EMacroPhase::EarlyGame:
            if (GameStateDescriptorValue.MacroState.ActiveBaseCount >= 2U &&
                GameStateDescriptorValue.MacroState.WorkerCount >= 24U &&
                (ShortHorizonDiscretionaryMineralsValue >= 350U ||
                 HasSustainedMineralFloat(GameStateDescriptorValue.ExecutionPressure)))
            {
                return 3U;
            }
            return 2U;
        case EMacroPhase::MidGame:
            if (GameStateDescriptorValue.MacroState.ActiveBaseCount >= 3U &&
                (GameStateDescriptorValue.MacroState.ArmySupply >= 60U ||
                 ShortHorizonDiscretionaryMineralsValue >= 550U ||
                 HasSustainedMineralFloat(GameStateDescriptorValue.ExecutionPressure)))
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
    constexpr uint32_t StartingSupplyCapValue = 15U;
    constexpr uint32_t MaximumSupplyCapValue = 200U;

    const uint32_t SupplyBufferValue =
        GetSupplyBufferForMacroPhase(GameStateDescriptorValue.MacroState.ActiveMacroPhase);
    const uint32_t ProjectedSupplyUsedValue =
        std::min<uint32_t>(MaximumSupplyCapValue,
                           GameStateDescriptorValue.MacroState.SupplyUsed +
                               GetCappedScheduledSupplyUsageDelta(GameStateDescriptorValue));
    const uint32_t BufferedSupplyCapTargetValue =
        std::min<uint32_t>(MaximumSupplyCapValue, ProjectedSupplyUsedValue + SupplyBufferValue);
    const uint32_t RequiredSupplyDepotCountValue =
        BufferedSupplyCapTargetValue > StartingSupplyCapValue
            ? ((BufferedSupplyCapTargetValue - StartingSupplyCapValue) + 7U) / 8U
            : 0U;
    return std::max<uint32_t>(2U, RequiredSupplyDepotCountValue);
}

uint32_t FTerranGoalRuleLibrary::DetermineDesiredBarracksCount(const FGameStateDescriptor& GameStateDescriptorValue)
{
    const uint32_t ShortHorizonDiscretionaryMineralsValue =
        GetShortHorizonDiscretionaryMinerals(GameStateDescriptorValue);
    const bool HasSustainedMineralFloatValue = HasSustainedMineralFloat(GameStateDescriptorValue.ExecutionPressure);

    switch (GameStateDescriptorValue.MacroState.ActiveMacroPhase)
    {
        case EMacroPhase::Opening:
            return 1U;
        case EMacroPhase::EarlyGame:
            if (GameStateDescriptorValue.MacroState.ActiveBaseCount >= 2U &&
                (ShortHorizonDiscretionaryMineralsValue >= 300U || HasSustainedMineralFloatValue))
            {
                return 3U;
            }
            return 2U;
        case EMacroPhase::MidGame:
            if (GameStateDescriptorValue.MacroState.ActiveBaseCount >= 3U ||
                ShortHorizonDiscretionaryMineralsValue >= 500U ||
                HasSustainedMineralFloatValue)
            {
                return 5U;
            }
            return 3U;
        case EMacroPhase::LateGame:
            return 8U;
        case EMacroPhase::Recovery:
            return std::max<uint32_t>(1U, GetProjectedBuildingCount(GameStateDescriptorValue,
                                                                    UNIT_TYPEID::TERRAN_BARRACKS));
        default:
            return 3U;
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
    const uint32_t DesiredBarracksCountValue = DetermineDesiredBarracksCount(GameStateDescriptorValue);

    switch (GameStateDescriptorValue.MacroState.ActiveMacroPhase)
    {
        case EMacroPhase::Opening:
            return 8U;
        case EMacroPhase::EarlyGame:
            return std::max<uint32_t>(20U, DesiredBarracksCountValue * 10U);
        case EMacroPhase::MidGame:
            return std::max<uint32_t>(36U, DesiredBarracksCountValue * 12U);
        case EMacroPhase::LateGame:
            return std::max<uint32_t>(72U, DesiredBarracksCountValue * 14U);
        case EMacroPhase::Recovery:
            return std::max<uint32_t>(4U, GameStateDescriptorValue.MacroState.ArmyUnitCount);
        default:
            return 20U;
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
            return 2U;
        case EMacroPhase::MidGame:
            return 4U;
        case EMacroPhase::LateGame:
            return 8U;
        case EMacroPhase::Recovery:
            return 0U;
        default:
            return 2U;
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

uint32_t FTerranGoalRuleLibrary::DetermineDesiredOrbitalCount(const FGameStateDescriptor& GameStateDescriptorValue)
{
    // Every completed CC should morph to orbital. The goal evaluates projected
    // orbital count against projected CC count (including in-progress CCs).
    const uint32_t ProjectedBaseCountValue =
        GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_COMMANDCENTER);
    return ProjectedBaseCountValue;
}

bool FTerranGoalRuleLibrary::ShouldPrioritizeUpgrades(const FGameStateDescriptor& GameStateDescriptorValue)
{
    return GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_COMMANDCENTER) >= 2U &&
           GetProjectedBuildingCount(GameStateDescriptorValue, UNIT_TYPEID::TERRAN_BARRACKS) >= 2U &&
           (GameStateDescriptorValue.EconomyState.GetProjectedDiscretionaryMineralsAtHorizon(
                GameStateDescriptorValue.CommitmentLedger, ShortForecastHorizonIndexValue) >= 75U ||
            HasSustainedMineralFloat(GameStateDescriptorValue.ExecutionPressure) ||
            HasIdleCombatProductionPressure(GameStateDescriptorValue.ExecutionPressure));
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
        if (ShouldPrioritizeUpgrades(GameStateDescriptorValue))
        {
            return EProductionFocus::Upgrades;
        }

        return EProductionFocus::Army;
    }

    if (HasSustainedMineralFloatValue)
    {
        if (ExecutionPressureDescriptorValue.CurrentMineralBankAmount >= 800U &&
            ExecutionPressureDescriptorValue.MineralBankDurationGameLoops >=
                ForecastHorizonGameLoopsValue[MediumForecastHorizonIndexValue])
        {
            return EProductionFocus::Army;
        }

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
