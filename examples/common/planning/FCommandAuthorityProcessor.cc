#include "common/planning/FCommandAuthorityProcessor.h"

#include <algorithm>

#include "common/build_orders/FOpeningPlanRegistry.h"
#include "common/catalogs/FTerranGoalDefinition.h"
#include "common/catalogs/FTerranGoalDictionary.h"
#include "common/catalogs/FTerranGoalRuleLibrary.h"
#include "common/catalogs/FTerranTaskTemplateDefinition.h"
#include "common/catalogs/FTerranTaskTemplateDictionary.h"
#include "common/planning/FTerranCommandTaskAdmissionService.h"
#include "common/planning/ICommandTaskAdmissionService.h"
#include "common/terran_models.h"

namespace sc2
{
namespace
{

EIntentDomain DetermineIntentDomain(const AbilityID AbilityIdValue, const ECommandTaskType CommandTaskTypeValue)
{
    if (CommandTaskTypeValue == ECommandTaskType::ArmyMission)
    {
        return EIntentDomain::ArmyCombat;
    }

    switch (AbilityIdValue.ToType())
    {
        case ABILITY_ID::TRAIN_SCV:
        case ABILITY_ID::TRAIN_MARINE:
        case ABILITY_ID::TRAIN_MARAUDER:
        case ABILITY_ID::TRAIN_HELLION:
        case ABILITY_ID::TRAIN_CYCLONE:
        case ABILITY_ID::TRAIN_MEDIVAC:
        case ABILITY_ID::TRAIN_LIBERATOR:
        case ABILITY_ID::TRAIN_SIEGETANK:
        case ABILITY_ID::TRAIN_WIDOWMINE:
        case ABILITY_ID::TRAIN_VIKINGFIGHTER:
        case ABILITY_ID::RESEARCH_STIMPACK:
        case ABILITY_ID::RESEARCH_COMBATSHIELD:
        case ABILITY_ID::RESEARCH_CONCUSSIVESHELLS:
        case ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONSLEVEL1:
            return EIntentDomain::UnitProduction;
        default:
            return EIntentDomain::StructureBuild;
    }
}

EIntentDomain DetermineIntentDomain(const FCommandOrderRecord& CommandOrderRecordValue)
{
    return DetermineIntentDomain(CommandOrderRecordValue.AbilityId, CommandOrderRecordValue.TaskType);
}

uint32_t GetBuildingCount(const std::array<uint16_t, NUM_TERRAN_BUILDINGS>& BuildingCountsValue,
                          const UNIT_TYPEID BuildingTypeIdValue)
{
    switch (BuildingTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
            return 0U;
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOT:
            return static_cast<uint32_t>(BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_SUPPLYDEPOT)]) +
                   static_cast<uint32_t>(BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED)]);
        case UNIT_TYPEID::TERRAN_BARRACKS:
            return static_cast<uint32_t>(BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_BARRACKS)]) +
                   static_cast<uint32_t>(BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_BARRACKSFLYING)]);
        case UNIT_TYPEID::TERRAN_FACTORY:
            return static_cast<uint32_t>(BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_FACTORY)]) +
                   static_cast<uint32_t>(BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_FACTORYFLYING)]);
        case UNIT_TYPEID::TERRAN_STARPORT:
            return static_cast<uint32_t>(BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_STARPORT)]) +
                   static_cast<uint32_t>(BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_STARPORTFLYING)]);
        case UNIT_TYPEID::TERRAN_REFINERY:
            return static_cast<uint32_t>(BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_REFINERY)]) +
                   static_cast<uint32_t>(BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_REFINERYRICH)]);
        default:
        {
            const size_t BuildingTypeIndexValue = GetTerranBuildingTypeIndex(BuildingTypeIdValue);
            return IsTerranBuildingTypeIndexValid(BuildingTypeIndexValue)
                       ? static_cast<uint32_t>(BuildingCountsValue[BuildingTypeIndexValue])
                       : 0U;
        }
    }
}

uint32_t GetUnitCount(const std::array<uint16_t, NUM_TERRAN_UNITS>& UnitCountsValue, const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_HELLION:
            return static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_HELLION)]) +
                   static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_HELLIONTANK)]);
        case UNIT_TYPEID::TERRAN_LIBERATOR:
            return static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_LIBERATOR)]) +
                   static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_LIBERATORAG)]);
        case UNIT_TYPEID::TERRAN_SIEGETANK:
            return static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_SIEGETANK)]) +
                   static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_SIEGETANKSIEGED)]);
        default:
        {
            const size_t UnitTypeIndexValue = GetTerranUnitTypeIndex(UnitTypeIdValue);
            return IsTerranUnitTypeIndexValid(UnitTypeIndexValue) ? static_cast<uint32_t>(UnitCountsValue[UnitTypeIndexValue])
                                                                  : 0U;
        }
    }
}

uint32_t GetUpgradeCount(const std::array<uint8_t, NUM_TERRAN_UPGRADES>& UpgradeCountsValue,
                         const UpgradeID UpgradeIdValue)
{
    const size_t UpgradeTypeIndexValue = GetTerranUpgradeTypeIndex(UpgradeIdValue);
    return IsTerranUpgradeTypeIndexValid(UpgradeTypeIndexValue)
               ? static_cast<uint32_t>(UpgradeCountsValue[UpgradeTypeIndexValue])
               : 0U;
}

bool IsRampWallSlotType(const EBuildPlacementSlotType PreferredPlacementSlotTypeValue)
{
    switch (PreferredPlacementSlotTypeValue)
    {
        case EBuildPlacementSlotType::MainRampDepotLeft:
        case EBuildPlacementSlotType::MainRampBarracksWithAddon:
        case EBuildPlacementSlotType::MainRampDepotRight:
            return true;
        default:
            return false;
    }
}

bool IsRampWallDepotSlotType(const EBuildPlacementSlotType PreferredPlacementSlotTypeValue)
{
    switch (PreferredPlacementSlotTypeValue)
    {
        case EBuildPlacementSlotType::MainRampDepotLeft:
        case EBuildPlacementSlotType::MainRampDepotRight:
            return true;
        default:
            return false;
    }
}

bool DoesTaskSignatureMatch(const uint32_t LeftTaskIdValue, const uint32_t LeftSourceGoalIdValue,
                            const AbilityID LeftAbilityIdValue, const UNIT_TYPEID LeftResultUnitTypeIdValue,
                            const UpgradeID LeftUpgradeIdValue,
                            const FBuildPlacementSlotId& LeftPreferredPlacementSlotIdValue,
                            const uint32_t RightTaskIdValue, const uint32_t RightSourceGoalIdValue,
                            const AbilityID RightAbilityIdValue, const UNIT_TYPEID RightResultUnitTypeIdValue,
                            const UpgradeID RightUpgradeIdValue,
                            const FBuildPlacementSlotId& RightPreferredPlacementSlotIdValue)
{
    if (LeftTaskIdValue != 0U || RightTaskIdValue != 0U)
    {
        return LeftTaskIdValue == RightTaskIdValue;
    }

    return LeftSourceGoalIdValue == RightSourceGoalIdValue && LeftAbilityIdValue == RightAbilityIdValue &&
           LeftResultUnitTypeIdValue == RightResultUnitTypeIdValue && LeftUpgradeIdValue == RightUpgradeIdValue &&
           LeftPreferredPlacementSlotIdValue == RightPreferredPlacementSlotIdValue;
}

bool TryGetObservedExactPlacementSlotId(const FCommandOrderRecord& CommandOrderRecordValue,
                                        FBuildPlacementSlotId& OutBuildPlacementSlotIdValue)
{
    if (CommandOrderRecordValue.ReservedPlacementSlotId.IsValid())
    {
        OutBuildPlacementSlotIdValue = CommandOrderRecordValue.ReservedPlacementSlotId;
        return true;
    }

    if (CommandOrderRecordValue.PreferredPlacementSlotId.IsValid())
    {
        OutBuildPlacementSlotIdValue = CommandOrderRecordValue.PreferredPlacementSlotId;
        return true;
    }

    return false;
}

bool ShouldUseExactPlacementSlotObservedMatch(const FCommandOrderRecord& CommandOrderRecordValue)
{
    FBuildPlacementSlotId BuildPlacementSlotIdValue;
    return TryGetObservedExactPlacementSlotId(CommandOrderRecordValue, BuildPlacementSlotIdValue);
}

bool DoesExactPlacementSlotContainExpectedStructure(const FGameStateDescriptor& GameStateDescriptorValue,
                                                    const FCommandOrderRecord& CommandOrderRecordValue)
{
    FBuildPlacementSlotId BuildPlacementSlotIdValue;
    if (!TryGetObservedExactPlacementSlotId(CommandOrderRecordValue, BuildPlacementSlotIdValue))
    {
        return false;
    }

    return GameStateDescriptorValue.ObservedPlacementSlotState.GetObservedPlacementSlotState(
               BuildPlacementSlotIdValue) == EObservedWallSlotState::Occupied;
}

bool IsMandatoryOpeningDepotTask(const FCommandTaskDescriptor& CommandTaskDescriptorValue)
{
    return CommandTaskDescriptorValue.CommitmentClass == ECommandCommitmentClass::MandatoryOpening &&
           CommandTaskDescriptorValue.ExecutionGuarantee == ECommandTaskExecutionGuarantee::MustExecute &&
           CommandTaskDescriptorValue.ActionAbilityId == ABILITY_ID::BUILD_SUPPLYDEPOT &&
           IsRampWallDepotSlotType(CommandTaskDescriptorValue.ActionPreferredPlacementSlotType);
}

FCommandTaskDescriptor ResolveEffectiveOpeningTaskDescriptor(
    const FOpeningPlanExecutionState& OpeningPlanExecutionStateValue,
    const FCommandTaskDescriptor& CommandTaskDescriptorValue)
{
    FCommandTaskDescriptor EffectiveTaskDescriptorValue = CommandTaskDescriptorValue;
    FBuildPlacementSlotId RemappedPlacementSlotIdValue;
    if (!OpeningPlanExecutionStateValue.TryGetRemappedPlacementSlotId(CommandTaskDescriptorValue.TaskId,
                                                                      RemappedPlacementSlotIdValue))
    {
        return EffectiveTaskDescriptorValue;
    }

    EffectiveTaskDescriptorValue.ActionPreferredPlacementSlotId = RemappedPlacementSlotIdValue;
    EffectiveTaskDescriptorValue.ActionPreferredPlacementSlotType = RemappedPlacementSlotIdValue.SlotType;
    return EffectiveTaskDescriptorValue;
}

bool AreAllMandatoryOpeningDepotTasksComplete(const FOpeningPlanExecutionState& OpeningPlanExecutionStateValue,
                                              const FOpeningPlanDescriptor& OpeningPlanDescriptorValue)
{
    for (const FOpeningPlanStep& OpeningPlanStepValue : OpeningPlanDescriptorValue.Steps)
    {
        if (!IsMandatoryOpeningDepotTask(OpeningPlanStepValue.TaskDescriptor))
        {
            continue;
        }

        if (!OpeningPlanExecutionStateValue.IsStepCompleted(OpeningPlanStepValue.TaskDescriptor.TaskId))
        {
            return false;
        }
    }

    return true;
}

bool IsArmyMissionGoalType(const EGoalType GoalTypeValue)
{
    switch (GoalTypeValue)
    {
        case EGoalType::HoldOwnedBase:
        case EGoalType::PressureEnemy:
        case EGoalType::ClearEnemyPresence:
        case EGoalType::ScoutExpansionLocations:
            return true;
        default:
            return false;
    }
}

bool IsGoalEligibleForStrategicSeeding(const FGoalDescriptor& GoalDescriptorValue)
{
    return GoalDescriptorValue.GoalStatus == EGoalStatus::Active && !IsArmyMissionGoalType(GoalDescriptorValue.GoalType);
}

bool IsNonTerminalLifecycleState(const EOrderLifecycleState LifecycleStateValue)
{
    return !IsTerminalLifecycleState(LifecycleStateValue);
}

bool DoesOrderMatchGoal(const FCommandOrderRecord& CommandOrderRecordValue, const FGoalDescriptor& GoalDescriptorValue)
{
    return CommandOrderRecordValue.SourceGoalId == GoalDescriptorValue.GoalId;
}

const FGoalDescriptor* SelectCurrentArmyMissionGoal(const FGameStateDescriptor& GameStateDescriptorValue)
{
    if (GameStateDescriptorValue.MacroState.PrimaryProductionFocus == EProductionFocus::Defense ||
        GameStateDescriptorValue.MacroState.ActiveGamePlan == EGamePlan::Recovery)
    {
        for (const FGoalDescriptor& GoalDescriptorValue : GameStateDescriptorValue.GoalSet.ImmediateGoals)
        {
            if (GoalDescriptorValue.GoalType == EGoalType::HoldOwnedBase &&
                GoalDescriptorValue.GoalStatus == EGoalStatus::Active)
            {
                return &GoalDescriptorValue;
            }
        }
    }

    for (const FGoalDescriptor& GoalDescriptorValue : GameStateDescriptorValue.GoalSet.StrategicGoals)
    {
        if (GoalDescriptorValue.GoalType == EGoalType::PressureEnemy &&
            GoalDescriptorValue.GoalStatus == EGoalStatus::Active)
        {
            return &GoalDescriptorValue;
        }
    }

    for (const FGoalDescriptor& GoalDescriptorValue : GameStateDescriptorValue.GoalSet.StrategicGoals)
    {
        if (GoalDescriptorValue.GoalType == EGoalType::ScoutExpansionLocations &&
            GoalDescriptorValue.GoalStatus == EGoalStatus::Active)
        {
            return &GoalDescriptorValue;
        }
    }

    return nullptr;
}

bool IsCriticalRecoveryMacroState(const FGameStateDescriptor& GameStateDescriptorValue)
{
    return GameStateDescriptorValue.MacroState.ActiveBaseCount == 0U ||
           GameStateDescriptorValue.MacroState.WorkerCount < 12U;
}

void ApplyGoalDrivenCommitmentMetadata(FCommandOrderRecord& CommandOrderRecordValue,
                                       const bool IsCriticalRecoveryMacroStateValue)
{
    if (IsCriticalRecoveryMacroStateValue)
    {
        CommandOrderRecordValue.Origin = ECommandTaskOrigin::Recovery;
        CommandOrderRecordValue.CommitmentClass = ECommandCommitmentClass::MandatoryRecovery;
        CommandOrderRecordValue.ExecutionGuarantee = ECommandTaskExecutionGuarantee::MustExecute;
        CommandOrderRecordValue.RetentionPolicy = ECommandTaskRetentionPolicy::HotMustRun;
        return;
    }

    CommandOrderRecordValue.Origin = ECommandTaskOrigin::GoalMacro;
    CommandOrderRecordValue.CommitmentClass = ECommandCommitmentClass::FlexibleMacro;
    CommandOrderRecordValue.ExecutionGuarantee = ECommandTaskExecutionGuarantee::Preferred;
}

uint32_t GetReadyTownHallCount(const FGameStateDescriptor& GameStateDescriptorValue)
{
    return std::max<uint32_t>(1U, GameStateDescriptorValue.BuildPlanning.ObservedTownHallCount);
}

bool ShouldUseCriticalRecoveryMetadata(const FGameStateDescriptor& GameStateDescriptorValue,
                                       const FGoalDescriptor& GoalDescriptorValue)
{
    if (!IsCriticalRecoveryMacroState(GameStateDescriptorValue))
    {
        return false;
    }

    switch (GoalDescriptorValue.GoalType)
    {
        case EGoalType::SaturateWorkers:
        case EGoalType::MaintainSupply:
        case EGoalType::ExpandBaseCount:
            return true;
        default:
            return false;
    }
}

ETerranTaskTemplateId ResolveFallbackTaskTemplateIdForGoalDescriptor(const FGoalDescriptor& GoalDescriptorValue)
{
    switch (GoalDescriptorValue.GoalType)
    {
        case EGoalType::SaturateWorkers:
            return ETerranTaskTemplateId::TrainScv;
        case EGoalType::MaintainSupply:
            return ETerranTaskTemplateId::BuildSupplyDepot;
        case EGoalType::ExpandBaseCount:
            return ETerranTaskTemplateId::BuildCommandCenter;
        case EGoalType::BuildProductionCapacity:
            switch (GoalDescriptorValue.TargetUnitTypeId)
            {
                case UNIT_TYPEID::TERRAN_REFINERY:
                    return ETerranTaskTemplateId::BuildRefinery;
                case UNIT_TYPEID::TERRAN_BARRACKS:
                    return ETerranTaskTemplateId::BuildBarracks;
                case UNIT_TYPEID::TERRAN_FACTORY:
                    return ETerranTaskTemplateId::BuildFactory;
                case UNIT_TYPEID::TERRAN_STARPORT:
                    return ETerranTaskTemplateId::BuildStarport;
                default:
                    return ETerranTaskTemplateId::Invalid;
            }
        case EGoalType::UnlockTechnology:
            switch (GoalDescriptorValue.TargetUnitTypeId)
            {
                case UNIT_TYPEID::TERRAN_BARRACKSREACTOR:
                    return ETerranTaskTemplateId::BuildBarracksReactor;
                case UNIT_TYPEID::TERRAN_FACTORYTECHLAB:
                    return ETerranTaskTemplateId::BuildFactoryTechLab;
                case UNIT_TYPEID::TERRAN_ENGINEERINGBAY:
                    return ETerranTaskTemplateId::BuildEngineeringBay;
                case UNIT_TYPEID::TERRAN_STARPORTREACTOR:
                    return ETerranTaskTemplateId::BuildStarportReactor;
                default:
                    return ETerranTaskTemplateId::Invalid;
            }
        case EGoalType::ResearchUpgrade:
            switch (GoalDescriptorValue.TargetUpgradeId.ToType())
            {
                case UPGRADE_ID::STIMPACK:
                    return ETerranTaskTemplateId::ResearchStimpack;
                case UPGRADE_ID::SHIELDWALL:
                    return ETerranTaskTemplateId::ResearchCombatShield;
                case UPGRADE_ID::TERRANINFANTRYWEAPONSLEVEL1:
                    return ETerranTaskTemplateId::ResearchTerranInfantryWeaponsLevel1;
                case UPGRADE_ID::PUNISHERGRENADES:
                    return ETerranTaskTemplateId::ResearchConcussiveShells;
                default:
                    return ETerranTaskTemplateId::Invalid;
            }
        case EGoalType::ProduceArmy:
            switch (GoalDescriptorValue.TargetUnitTypeId)
            {
                case UNIT_TYPEID::TERRAN_MARINE:
                    return ETerranTaskTemplateId::TrainMarine;
                case UNIT_TYPEID::TERRAN_MARAUDER:
                    return ETerranTaskTemplateId::TrainMarauder;
                case UNIT_TYPEID::TERRAN_HELLION:
                    return ETerranTaskTemplateId::TrainHellion;
                case UNIT_TYPEID::TERRAN_CYCLONE:
                    return ETerranTaskTemplateId::TrainCyclone;
                case UNIT_TYPEID::TERRAN_SIEGETANK:
                    return ETerranTaskTemplateId::TrainSiegeTank;
                case UNIT_TYPEID::TERRAN_WIDOWMINE:
                    return ETerranTaskTemplateId::TrainWidowMine;
                case UNIT_TYPEID::TERRAN_MEDIVAC:
                    return ETerranTaskTemplateId::TrainMedivac;
                case UNIT_TYPEID::TERRAN_LIBERATOR:
                    return ETerranTaskTemplateId::TrainLiberator;
                case UNIT_TYPEID::TERRAN_VIKINGFIGHTER:
                    return ETerranTaskTemplateId::TrainVikingFighter;
                default:
                    return ETerranTaskTemplateId::Invalid;
            }
        default:
            return ETerranTaskTemplateId::Invalid;
    }
}

bool TryPopulateGoalOrderFromTaskTemplate(const FGameStateDescriptor& GameStateDescriptorValue,
                                          const FGoalDescriptor& GoalDescriptorValue,
                                          const uint64_t CurrentGameLoopValue,
                                          FCommandOrderRecord& CommandOrderRecordValue)
{
    ETerranTaskTemplateId TaskTemplateIdValue = ETerranTaskTemplateId::Invalid;
    const FTerranGoalDefinition* TerranGoalDefinitionValue =
        FTerranGoalDictionary::TryGetByGoalId(GoalDescriptorValue.GoalId);
    if (TerranGoalDefinitionValue != nullptr)
    {
        TaskTemplateIdValue = TerranGoalDefinitionValue->TaskTemplateId;
    }

    const FTerranTaskTemplateDefinition* TerranTaskTemplateDefinitionValue =
        FTerranTaskTemplateDictionary::TryGetByTemplateId(TaskTemplateIdValue);
    if (TerranTaskTemplateDefinitionValue == nullptr)
    {
        TaskTemplateIdValue = ResolveFallbackTaskTemplateIdForGoalDescriptor(GoalDescriptorValue);
        TerranTaskTemplateDefinitionValue = FTerranTaskTemplateDictionary::TryGetByTemplateId(TaskTemplateIdValue);
    }
    if (TerranTaskTemplateDefinitionValue == nullptr)
    {
        return false;
    }

    CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::StrategicDirector, NullTag, TerranTaskTemplateDefinitionValue->ActionAbilityId,
        GoalDescriptorValue.BasePriorityValue,
        DetermineIntentDomain(TerranTaskTemplateDefinitionValue->ActionAbilityId,
                              TerranTaskTemplateDefinitionValue->TaskType),
        CurrentGameLoopValue);
    CommandOrderRecordValue.TaskPackageKind = TerranTaskTemplateDefinitionValue->PackageKind;
    CommandOrderRecordValue.TaskNeedKind = TerranTaskTemplateDefinitionValue->NeedKind;
    CommandOrderRecordValue.TaskType = TerranTaskTemplateDefinitionValue->TaskType;
    CommandOrderRecordValue.Origin = TerranTaskTemplateDefinitionValue->Origin;
    CommandOrderRecordValue.CommitmentClass = TerranTaskTemplateDefinitionValue->CommitmentClass;
    CommandOrderRecordValue.ExecutionGuarantee = TerranTaskTemplateDefinitionValue->ExecutionGuarantee;
    CommandOrderRecordValue.RetentionPolicy = TerranTaskTemplateDefinitionValue->RetentionPolicy;
    CommandOrderRecordValue.BlockedTaskWakeKind = TerranTaskTemplateDefinitionValue->BlockedTaskWakeKind;
    CommandOrderRecordValue.ProducerUnitTypeId = TerranTaskTemplateDefinitionValue->ActionProducerUnitTypeId;
    CommandOrderRecordValue.ResultUnitTypeId = TerranTaskTemplateDefinitionValue->ActionResultUnitTypeId;
    CommandOrderRecordValue.UpgradeId = TerranTaskTemplateDefinitionValue->ActionUpgradeId;
    CommandOrderRecordValue.PreferredPlacementSlotType =
        TerranTaskTemplateDefinitionValue->DefaultPreferredPlacementSlotType;
    CommandOrderRecordValue.TargetCount =
        std::max<uint32_t>(TerranTaskTemplateDefinitionValue->DefaultTargetCount, GoalDescriptorValue.TargetCount);
    CommandOrderRecordValue.RequestedQueueCount =
        std::max<uint32_t>(1U, TerranTaskTemplateDefinitionValue->DefaultRequestedQueueCount);

    const bool ShouldUseCriticalRecoveryMetadataValue =
        ShouldUseCriticalRecoveryMetadata(GameStateDescriptorValue, GoalDescriptorValue);
    ApplyGoalDrivenCommitmentMetadata(CommandOrderRecordValue, ShouldUseCriticalRecoveryMetadataValue);
    if (ShouldUseCriticalRecoveryMetadataValue)
    {
        CommandOrderRecordValue.RetentionPolicy = ECommandTaskRetentionPolicy::HotMustRun;
    }

    switch (GoalDescriptorValue.GoalType)
    {
        case EGoalType::SaturateWorkers:
        {
            const uint32_t WorkerDeficitValue =
                GoalDescriptorValue.TargetCount > GameStateDescriptorValue.MacroState.WorkerCount
                    ? GoalDescriptorValue.TargetCount - GameStateDescriptorValue.MacroState.WorkerCount
                    : 0U;
            CommandOrderRecordValue.RequestedQueueCount = std::max<uint32_t>(
                1U, std::min<uint32_t>(WorkerDeficitValue, GetReadyTownHallCount(GameStateDescriptorValue)));
            CommandOrderRecordValue.RetentionPolicy = ECommandTaskRetentionPolicy::HotMustRun;
            return true;
        }
        case EGoalType::ProduceArmy:
        {
            const uint32_t ProjectedUnitCountValue =
                GameStateDescriptorValue.ProductionState.GetProjectedUnitCount(GoalDescriptorValue.TargetUnitTypeId);
            const uint32_t UnitDeficitValue =
                GoalDescriptorValue.TargetCount > ProjectedUnitCountValue
                    ? (GoalDescriptorValue.TargetCount - ProjectedUnitCountValue)
                    : 0U;
            const uint32_t CurrentProducerCapacityValue =
                GameStateDescriptorValue.ProductionState.GetCurrentProducerCapacity(
                    CommandOrderRecordValue.ProducerUnitTypeId);
            const uint32_t CurrentProducerOccupancyValue =
                GameStateDescriptorValue.ProductionState.GetCurrentProducerOccupancy(
                    CommandOrderRecordValue.ProducerUnitTypeId);
            const uint32_t FreeProducerQueueCapacityValue =
                CurrentProducerCapacityValue > CurrentProducerOccupancyValue
                    ? (CurrentProducerCapacityValue - CurrentProducerOccupancyValue)
                    : 0U;
            CommandOrderRecordValue.RequestedQueueCount = std::max<uint32_t>(
                1U,
                std::min<uint32_t>(std::max<uint32_t>(1U, UnitDeficitValue),
                                   std::max<uint32_t>(1U, FreeProducerQueueCapacityValue)));
            return true;
        }
        default:
            return true;
    }
}

bool TryPopulateGoalOrder(const FGameStateDescriptor& GameStateDescriptorValue, const FGoalDescriptor& GoalDescriptorValue,
                          const uint64_t CurrentGameLoopValue, FCommandOrderRecord& CommandOrderRecordValue)
{
    return TryPopulateGoalOrderFromTaskTemplate(GameStateDescriptorValue, GoalDescriptorValue, CurrentGameLoopValue,
                                                CommandOrderRecordValue);
}

bool HasActiveStrategicOrderForGoal(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                    const FGoalDescriptor& GoalDescriptorValue)
{
    return CommandAuthoritySchedulingStateValue.HasActiveStrategicOrderForGoalId(GoalDescriptorValue.GoalId);
}

bool DoesRemainingOpeningPlanCoverOrder(const FGameStateDescriptor& GameStateDescriptorValue,
                                        const FCommandOrderRecord& CommandOrderRecordValue)
{
    switch (CommandOrderRecordValue.TaskType)
    {
        case ECommandTaskType::WorkerProduction:
        case ECommandTaskType::Supply:
        case ECommandTaskType::Expansion:
        case ECommandTaskType::Recovery:
            return false;
        default:
            break;
    }

    const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState;
    return CommandAuthoritySchedulingStateValue.HasEquivalentActiveTaskSignature(
        FCommandTaskSignatureKey::FromOrderRecord(CommandOrderRecordValue));
}

bool IsOpeningIncomplete(const FGameStateDescriptor& GameStateDescriptorValue)
{
    return GameStateDescriptorValue.OpeningPlanExecutionState.LifecycleState != EOpeningPlanLifecycleState::Completed;
}

bool IsMandatoryExactOpeningStructureTask(const FCommandTaskDescriptor& CommandTaskDescriptorValue)
{
    return CommandTaskDescriptorValue.CommitmentClass == ECommandCommitmentClass::MandatoryOpening &&
           CommandTaskDescriptorValue.ExecutionGuarantee == ECommandTaskExecutionGuarantee::MustExecute &&
           CommandTaskDescriptorValue.ActionKind == ECommandTaskActionKind::BuildStructure &&
           CommandTaskDescriptorValue.ActionPreferredPlacementSlotType != EBuildPlacementSlotType::Unknown;
}

bool IsMandatoryProducerBoundOpeningAddonTask(const FCommandTaskDescriptor& CommandTaskDescriptorValue)
{
    return CommandTaskDescriptorValue.CommitmentClass == ECommandCommitmentClass::MandatoryOpening &&
           CommandTaskDescriptorValue.ExecutionGuarantee == ECommandTaskExecutionGuarantee::MustExecute &&
           CommandTaskDescriptorValue.ActionKind == ECommandTaskActionKind::BuildAddon &&
           CommandTaskDescriptorValue.ActionPreferredProducerPlacementSlotId.IsValid();
}

bool IsOpeningWallStructureSlotType(const EBuildPlacementSlotType BuildPlacementSlotTypeValue)
{
    switch (BuildPlacementSlotTypeValue)
    {
        case EBuildPlacementSlotType::MainRampDepotLeft:
        case EBuildPlacementSlotType::MainRampBarracksWithAddon:
        case EBuildPlacementSlotType::MainRampDepotRight:
        case EBuildPlacementSlotType::NaturalEntranceDepotLeft:
        case EBuildPlacementSlotType::NaturalEntranceDepotRight:
            return true;
        default:
            return false;
    }
}

bool IsMandatoryOpeningWallStructureTask(const FCommandTaskDescriptor& CommandTaskDescriptorValue)
{
    return IsMandatoryExactOpeningStructureTask(CommandTaskDescriptorValue) &&
           IsOpeningWallStructureSlotType(CommandTaskDescriptorValue.ActionPreferredPlacementSlotType);
}

bool IsFlexibleOpeningSupplyTask(const FCommandTaskDescriptor& CommandTaskDescriptorValue)
{
    return CommandTaskDescriptorValue.ActionAbilityId == ABILITY_ID::BUILD_SUPPLYDEPOT &&
           (CommandTaskDescriptorValue.CommitmentClass != ECommandCommitmentClass::MandatoryOpening ||
            CommandTaskDescriptorValue.ExecutionGuarantee != ECommandTaskExecutionGuarantee::MustExecute);
}

bool HasOutstandingFlexibleSupplyDepotDemand(const FGameStateDescriptor& GameStateDescriptorValue)
{
    const uint32_t DesiredSupplyDepotCountValue =
        FTerranGoalRuleLibrary::DetermineDesiredSupplyDepotCount(GameStateDescriptorValue);
    return GameStateDescriptorValue.ProductionState.GetProjectedBuildingCount(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) <
           DesiredSupplyDepotCountValue;
}

bool IsHotMustRunOpeningTask(const FCommandTaskDescriptor& CommandTaskDescriptorValue)
{
    return CommandTaskDescriptorValue.Origin == ECommandTaskOrigin::Opening &&
           (CommandTaskDescriptorValue.RetentionPolicy == ECommandTaskRetentionPolicy::HotMustRun ||
            CommandTaskDescriptorValue.ExecutionGuarantee == ECommandTaskExecutionGuarantee::MustExecute);
}

bool DoesOpeningTaskRequireMandatorySequenceGuard(const FCommandTaskDescriptor& CommandTaskDescriptorValue)
{
    if (IsFlexibleOpeningSupplyTask(CommandTaskDescriptorValue))
    {
        return false;
    }

    switch (CommandTaskDescriptorValue.TaskType)
    {
        case ECommandTaskType::Supply:
        case ECommandTaskType::ProductionStructure:
        case ECommandTaskType::TechStructure:
        case ECommandTaskType::AddOn:
        case ECommandTaskType::UnitProduction:
        case ECommandTaskType::UpgradeResearch:
        case ECommandTaskType::StaticDefense:
            return true;
        case ECommandTaskType::Recovery:
        case ECommandTaskType::WorkerProduction:
        case ECommandTaskType::Expansion:
        case ECommandTaskType::Refinery:
        case ECommandTaskType::ArmyMission:
        case ECommandTaskType::Unknown:
        default:
            return false;
    }
}

bool HasIncompleteEarlierMandatoryExactOpeningStructureTask(
    const FOpeningPlanExecutionState& OpeningPlanExecutionStateValue,
    const FOpeningPlanDescriptor& OpeningPlanDescriptorValue, const uint32_t TaskIdValue)
{
    for (const FOpeningPlanStep& OpeningPlanStepValue : OpeningPlanDescriptorValue.Steps)
    {
        const FCommandTaskDescriptor OpeningTaskDescriptorValue = ResolveEffectiveOpeningTaskDescriptor(
            OpeningPlanExecutionStateValue, OpeningPlanStepValue.TaskDescriptor);
        if (OpeningTaskDescriptorValue.TaskId >= TaskIdValue)
        {
            break;
        }

        if (OpeningPlanExecutionStateValue.IsStepCompleted(OpeningTaskDescriptorValue.TaskId) ||
            !IsMandatoryExactOpeningStructureTask(OpeningTaskDescriptorValue))
        {
            continue;
        }

        return true;
    }

    return false;
}

bool HasIncompleteEarlierMandatoryOpeningWallStructureTask(
    const FOpeningPlanExecutionState& OpeningPlanExecutionStateValue,
    const FOpeningPlanDescriptor& OpeningPlanDescriptorValue, const uint32_t TaskIdValue)
{
    for (const FOpeningPlanStep& OpeningPlanStepValue : OpeningPlanDescriptorValue.Steps)
    {
        const FCommandTaskDescriptor OpeningTaskDescriptorValue = ResolveEffectiveOpeningTaskDescriptor(
            OpeningPlanExecutionStateValue, OpeningPlanStepValue.TaskDescriptor);
        if (OpeningTaskDescriptorValue.TaskId >= TaskIdValue)
        {
            break;
        }

        if (OpeningPlanExecutionStateValue.IsStepCompleted(OpeningTaskDescriptorValue.TaskId) ||
            !IsMandatoryOpeningWallStructureTask(OpeningTaskDescriptorValue))
        {
            continue;
        }

        return true;
    }

    return false;
}

bool ShouldDelayOpeningTaskUntilMandatorySequenceAdvances(
    const FOpeningPlanExecutionState& OpeningPlanExecutionStateValue,
    const FOpeningPlanDescriptor& OpeningPlanDescriptorValue,
    const FCommandTaskDescriptor& CommandTaskDescriptorValue)
{
    if (IsFlexibleOpeningSupplyTask(CommandTaskDescriptorValue))
    {
        return OpeningPlanExecutionStateValue.WallChainState == EOpeningWallChainState::RampActive &&
               !AreAllMandatoryOpeningDepotTasksComplete(OpeningPlanExecutionStateValue, OpeningPlanDescriptorValue);
    }

    if (IsMandatoryOpeningWallStructureTask(CommandTaskDescriptorValue))
    {
        return HasIncompleteEarlierMandatoryOpeningWallStructureTask(
            OpeningPlanExecutionStateValue, OpeningPlanDescriptorValue, CommandTaskDescriptorValue.TaskId);
    }

    if (IsMandatoryProducerBoundOpeningAddonTask(CommandTaskDescriptorValue))
    {
        return HasIncompleteEarlierMandatoryExactOpeningStructureTask(
            OpeningPlanExecutionStateValue, OpeningPlanDescriptorValue, CommandTaskDescriptorValue.TaskId);
    }

    if (!DoesOpeningTaskRequireMandatorySequenceGuard(CommandTaskDescriptorValue))
    {
        return false;
    }

    return HasIncompleteEarlierMandatoryExactOpeningStructureTask(
        OpeningPlanExecutionStateValue, OpeningPlanDescriptorValue, CommandTaskDescriptorValue.TaskId);
}

bool HasAnyIncompleteMandatoryOpeningSequenceTask(const FOpeningPlanExecutionState& OpeningPlanExecutionStateValue,
                                                  const FOpeningPlanDescriptor& OpeningPlanDescriptorValue)
{
    for (const FOpeningPlanStep& OpeningPlanStepValue : OpeningPlanDescriptorValue.Steps)
    {
        const FCommandTaskDescriptor OpeningTaskDescriptorValue = ResolveEffectiveOpeningTaskDescriptor(
            OpeningPlanExecutionStateValue, OpeningPlanStepValue.TaskDescriptor);
        if (OpeningPlanExecutionStateValue.IsStepCompleted(OpeningTaskDescriptorValue.TaskId) ||
            !IsMandatoryExactOpeningStructureTask(OpeningTaskDescriptorValue))
        {
            continue;
        }

        return true;
    }

    return false;
}

bool DoesIncompleteMandatoryOpeningStructureGuardBlockOrder(const FGameStateDescriptor& GameStateDescriptorValue,
                                                            const FCommandOrderRecord& CommandOrderRecordValue)
{
    if (!IsOpeningIncomplete(GameStateDescriptorValue) ||
        CommandOrderRecordValue.Origin != ECommandTaskOrigin::GoalMacro)
    {
        return false;
    }

    switch (CommandOrderRecordValue.TaskType)
    {
        case ECommandTaskType::Supply:
        case ECommandTaskType::ProductionStructure:
        case ECommandTaskType::TechStructure:
        case ECommandTaskType::AddOn:
        case ECommandTaskType::UnitProduction:
        case ECommandTaskType::UpgradeResearch:
        case ECommandTaskType::StaticDefense:
            break;
        case ECommandTaskType::WorkerProduction:
        case ECommandTaskType::Expansion:
        case ECommandTaskType::Refinery:
        case ECommandTaskType::Recovery:
        case ECommandTaskType::ArmyMission:
        case ECommandTaskType::Unknown:
        default:
            return false;
    }

    const FOpeningPlanExecutionState& OpeningPlanExecutionStateValue = GameStateDescriptorValue.OpeningPlanExecutionState;
    const FOpeningPlanDescriptor& OpeningPlanDescriptorValue =
        FOpeningPlanRegistry::GetOpeningPlanDescriptor(OpeningPlanExecutionStateValue.ActivePlanId);
    if (!HasAnyIncompleteMandatoryOpeningSequenceTask(OpeningPlanExecutionStateValue, OpeningPlanDescriptorValue))
    {
        return false;
    }

    if (CommandOrderRecordValue.TaskType == ECommandTaskType::Supply)
    {
        return OpeningPlanExecutionStateValue.WallChainState == EOpeningWallChainState::RampActive &&
               !AreAllMandatoryOpeningDepotTasksComplete(OpeningPlanExecutionStateValue, OpeningPlanDescriptorValue);
    }

    return true;
}

}  // namespace

void FCommandAuthorityProcessor::ProcessSchedulerStep(FGameStateDescriptor& GameStateDescriptorValue) const
{
    static const FTerranCommandTaskAdmissionService DefaultCommandTaskAdmissionServiceValue;
    ProcessSchedulerStep(GameStateDescriptorValue, DefaultCommandTaskAdmissionServiceValue);
}

void FCommandAuthorityProcessor::ProcessSchedulerStep(
    FGameStateDescriptor& GameStateDescriptorValue,
    const ICommandTaskAdmissionService& CommandTaskAdmissionServiceValue) const
{
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState;

    CommandAuthoritySchedulingStateValue.BeginMutationBatch();
    InitializeOpeningPlan(GameStateDescriptorValue);
    CommandTaskAdmissionServiceValue.RefreshStimulusState(GameStateDescriptorValue);
    CommandTaskAdmissionServiceValue.ReactivateBlockedTasks(GameStateDescriptorValue);
    UpdateCompletedOpeningSteps(GameStateDescriptorValue);
    SeedReadyStrategicOrders(GameStateDescriptorValue, CommandTaskAdmissionServiceValue);
    SeedGoalDrivenStrategicOrders(GameStateDescriptorValue, CommandTaskAdmissionServiceValue);
    CommandAuthoritySchedulingStateValue.EndMutationBatch();

    CommandAuthoritySchedulingStateValue.BeginMutationBatch();
    EnsureStrategicChildOrders(GameStateDescriptorValue);
    UpdateCompletedOpeningSteps(GameStateDescriptorValue);
    CommandAuthoritySchedulingStateValue.EndMutationBatch();
}

void FCommandAuthorityProcessor::InitializeOpeningPlan(FGameStateDescriptor& GameStateDescriptorValue) const
{
    FOpeningPlanExecutionState& OpeningPlanExecutionStateValue = GameStateDescriptorValue.OpeningPlanExecutionState;
    if (OpeningPlanExecutionStateValue.LifecycleState != EOpeningPlanLifecycleState::Uninitialized)
    {
        return;
    }

    OpeningPlanExecutionStateValue.SetActivePlan(EOpeningPlanId::TerranTwoBaseMMMFrameOpening);
}

void FCommandAuthorityProcessor::UpdateCompletedOpeningSteps(FGameStateDescriptor& GameStateDescriptorValue) const
{
    FOpeningPlanExecutionState& OpeningPlanExecutionStateValue = GameStateDescriptorValue.OpeningPlanExecutionState;
    const FOpeningPlanDescriptor& OpeningPlanDescriptorValue =
        FOpeningPlanRegistry::GetOpeningPlanDescriptor(OpeningPlanExecutionStateValue.ActivePlanId);
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState;

    for (const FOpeningPlanStep& OpeningPlanStepValue : OpeningPlanDescriptorValue.Steps)
    {
        const FCommandTaskDescriptor& CommandTaskDescriptorValue = OpeningPlanStepValue.TaskDescriptor;
        if (!OpeningPlanExecutionStateValue.IsStepCompleted(CommandTaskDescriptorValue.TaskId))
        {
            continue;
        }

        uint32_t StrategicOrderIdValue = 0U;
        if (!OpeningPlanExecutionStateValue.TryGetPlanOrderId(CommandTaskDescriptorValue.TaskId, StrategicOrderIdValue))
        {
            continue;
        }

        size_t StrategicOrderIndexValue = 0U;
        if (!CommandAuthoritySchedulingStateValue.TryGetOrderIndex(StrategicOrderIdValue, StrategicOrderIndexValue))
        {
            continue;
        }

        const FCommandOrderRecord StrategicOrderRecordValue =
            CommandAuthoritySchedulingStateValue.GetOrderRecord(StrategicOrderIndexValue);
        if (!ShouldUseExactPlacementSlotObservedMatch(StrategicOrderRecordValue) ||
            DoesExactPlacementSlotContainExpectedStructure(GameStateDescriptorValue, StrategicOrderRecordValue))
        {
            continue;
        }

        OpeningPlanExecutionStateValue.MarkStepIncomplete(CommandTaskDescriptorValue.TaskId);
        CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(StrategicOrderIdValue, EOrderLifecycleState::Queued);
        CommandAuthoritySchedulingStateValue.ClearOrderDeferralState(StrategicOrderIdValue);
        if (OpeningPlanExecutionStateValue.LifecycleState == EOpeningPlanLifecycleState::Completed)
        {
            OpeningPlanExecutionStateValue.LifecycleState = EOpeningPlanLifecycleState::Active;
        }
    }

    for (const FOpeningPlanStep& OpeningPlanStepValue : OpeningPlanDescriptorValue.Steps)
    {
        const FCommandTaskDescriptor EffectiveTaskDescriptorValue =
            ResolveEffectiveOpeningTaskDescriptor(OpeningPlanExecutionStateValue, OpeningPlanStepValue.TaskDescriptor);
        if (OpeningPlanExecutionStateValue.IsStepCompleted(EffectiveTaskDescriptorValue.TaskId))
        {
            continue;
        }

        FCommandOrderRecord CompletionProbeValue;
        CompletionProbeValue.ResultUnitTypeId = EffectiveTaskDescriptorValue.ActionResultUnitTypeId;
        CompletionProbeValue.UpgradeId = EffectiveTaskDescriptorValue.ActionUpgradeId;
        CompletionProbeValue.TargetCount = EffectiveTaskDescriptorValue.CompletionObservedCountAtLeast;
        CompletionProbeValue.PreferredPlacementSlotType =
            EffectiveTaskDescriptorValue.ActionPreferredPlacementSlotType;
        CompletionProbeValue.PreferredPlacementSlotId = EffectiveTaskDescriptorValue.ActionPreferredPlacementSlotId;
        if (!DoesOrderTargetMatchObservedState(GameStateDescriptorValue, CompletionProbeValue))
        {
            continue;
        }

        OpeningPlanExecutionStateValue.MarkStepCompleted(EffectiveTaskDescriptorValue.TaskId);

        uint32_t StrategicOrderIdValue = 0U;
        if (OpeningPlanExecutionStateValue.TryGetPlanOrderId(EffectiveTaskDescriptorValue.TaskId, StrategicOrderIdValue))
        {
            GameStateDescriptorValue.CommandAuthoritySchedulingState.SetOrderLifecycleState(StrategicOrderIdValue,
                                                                                           EOrderLifecycleState::Completed);

            const size_t OrderCountValue = CommandAuthoritySchedulingStateValue.OrderIds.size();
            for (size_t OrderIndexValue = 0U; OrderIndexValue < OrderCountValue; ++OrderIndexValue)
            {
                if (CommandAuthoritySchedulingStateValue.ParentOrderIds[OrderIndexValue] != StrategicOrderIdValue ||
                    CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] !=
                        ECommandAuthorityLayer::EconomyAndProduction ||
                    IsTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
                {
                    continue;
                }

                CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(
                    CommandAuthoritySchedulingStateValue.OrderIds[OrderIndexValue], EOrderLifecycleState::Completed);
            }
        }
    }

    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size(); ++OrderIndexValue)
    {
        const EOrderLifecycleState LifecycleStateValue =
            CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue];
        if (LifecycleStateValue == EOrderLifecycleState::Completed || LifecycleStateValue == EOrderLifecycleState::Expired ||
            LifecycleStateValue == EOrderLifecycleState::Aborted)
        {
            continue;
        }

        const FCommandOrderRecord CommandOrderRecordValue =
            CommandAuthoritySchedulingStateValue.GetOrderRecord(OrderIndexValue);
        if (CommandOrderRecordValue.TargetCount == 0U ||
            !DoesOrderTargetMatchObservedState(GameStateDescriptorValue, CommandOrderRecordValue))
        {
            continue;
        }

        CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(CommandOrderRecordValue.OrderId,
                                                                    EOrderLifecycleState::Completed);
    }

    if (AreAllMandatoryOpeningDepotTasksComplete(OpeningPlanExecutionStateValue, OpeningPlanDescriptorValue))
    {
        OpeningPlanExecutionStateValue.WallChainState = EOpeningWallChainState::Completed;
    }

    if (OpeningPlanExecutionStateValue.CompletedStepIds.size() >= OpeningPlanDescriptorValue.Steps.size())
    {
        OpeningPlanExecutionStateValue.LifecycleState = EOpeningPlanLifecycleState::Completed;
    }
}

void FCommandAuthorityProcessor::SeedReadyStrategicOrders(
    FGameStateDescriptor& GameStateDescriptorValue,
    const ICommandTaskAdmissionService& CommandTaskAdmissionServiceValue) const
{
    FOpeningPlanExecutionState& OpeningPlanExecutionStateValue = GameStateDescriptorValue.OpeningPlanExecutionState;
    const FOpeningPlanDescriptor& OpeningPlanDescriptorValue =
        FOpeningPlanRegistry::GetOpeningPlanDescriptor(OpeningPlanExecutionStateValue.ActivePlanId);
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState;

    uint32_t SeededOrderCountValue = 0U;
    for (const FOpeningPlanStep& OpeningPlanStepValue : OpeningPlanDescriptorValue.Steps)
    {
        const FCommandTaskDescriptor EffectiveTaskDescriptorValue =
            ResolveEffectiveOpeningTaskDescriptor(OpeningPlanExecutionStateValue, OpeningPlanStepValue.TaskDescriptor);
        if (OpeningPlanExecutionStateValue.HasSeededStep(EffectiveTaskDescriptorValue.TaskId) ||
            OpeningPlanExecutionStateValue.IsStepCompleted(EffectiveTaskDescriptorValue.TaskId))
        {
            continue;
        }
        const bool bBypassStrategicCapValue = IsHotMustRunOpeningTask(EffectiveTaskDescriptorValue);
        if (SeededOrderCountValue >= CommandAuthoritySchedulingStateValue.MaxStrategicOrdersPerStep &&
            !bBypassStrategicCapValue)
        {
            continue;
        }
        if (GameStateDescriptorValue.CurrentGameLoop < EffectiveTaskDescriptorValue.TriggerMinGameLoop)
        {
            return;
        }
        if (!AreRequiredTasksCompleted(OpeningPlanExecutionStateValue, EffectiveTaskDescriptorValue))
        {
            continue;
        }
        if (IsFlexibleOpeningSupplyTask(EffectiveTaskDescriptorValue) &&
            !HasOutstandingFlexibleSupplyDepotDemand(GameStateDescriptorValue))
        {
            continue;
        }
        if (ShouldDelayOpeningTaskUntilMandatorySequenceAdvances(OpeningPlanExecutionStateValue,
                                                                 OpeningPlanDescriptorValue,
                                                                 EffectiveTaskDescriptorValue))
        {
            continue;
        }

        const uint32_t StrategicOrderIdValue = [&]() -> uint32_t
        {
            uint32_t AdmittedOrderIdValue = 0U;
            return CommandTaskAdmissionServiceValue.TryAdmitOpeningTask(
                       GameStateDescriptorValue, EffectiveTaskDescriptorValue, AdmittedOrderIdValue)
                       ? AdmittedOrderIdValue
                       : 0U;
        }();
        if (StrategicOrderIdValue == 0U)
        {
            continue;
        }
        OpeningPlanExecutionStateValue.RecordSeededStep(EffectiveTaskDescriptorValue.TaskId, StrategicOrderIdValue);
        if (!bBypassStrategicCapValue)
        {
            ++SeededOrderCountValue;
        }
    }
}

void FCommandAuthorityProcessor::SeedGoalDrivenStrategicOrders(
    FGameStateDescriptor& GameStateDescriptorValue,
    const ICommandTaskAdmissionService& CommandTaskAdmissionServiceValue) const
{
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState;
    const FGoalDescriptor* ArmyMissionGoalDescriptorValue = SelectCurrentArmyMissionGoal(GameStateDescriptorValue);

    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] !=
                ECommandAuthorityLayer::StrategicDirector ||
            CommandAuthoritySchedulingStateValue.TaskTypes[OrderIndexValue] != ECommandTaskType::ArmyMission ||
            !IsNonTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        if (ArmyMissionGoalDescriptorValue != nullptr &&
            CommandAuthoritySchedulingStateValue.SourceGoalIds[OrderIndexValue] ==
                ArmyMissionGoalDescriptorValue->GoalId)
        {
            continue;
        }

        CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(
            CommandAuthoritySchedulingStateValue.OrderIds[OrderIndexValue], EOrderLifecycleState::Expired);
    }

    if (ArmyMissionGoalDescriptorValue != nullptr &&
        !HasActiveStrategicOrderForGoal(CommandAuthoritySchedulingStateValue, *ArmyMissionGoalDescriptorValue))
    {
        FCommandOrderRecord ArmyMissionOrderValue = FCommandOrderRecord::CreateNoTarget(
            ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::INVALID,
            ArmyMissionGoalDescriptorValue->BasePriorityValue, EIntentDomain::ArmyCombat,
            GameStateDescriptorValue.CurrentGameLoop);
        ArmyMissionOrderValue.TaskPackageKind =
            ArmyMissionGoalDescriptorValue->GoalType == EGoalType::HoldOwnedBase ? ECommandTaskPackageKind::Defense
                                                                                 : ECommandTaskPackageKind::TimingAttack;
        ArmyMissionOrderValue.TaskNeedKind = ECommandTaskNeedKind::Unit;
        ArmyMissionOrderValue.TaskType = ECommandTaskType::ArmyMission;
        ArmyMissionOrderValue.Origin = ECommandTaskOrigin::GoalMacro;
        ArmyMissionOrderValue.CommitmentClass = ECommandCommitmentClass::Opportunistic;
        ArmyMissionOrderValue.ExecutionGuarantee = ECommandTaskExecutionGuarantee::Preferred;
        ArmyMissionOrderValue.SourceGoalId = ArmyMissionGoalDescriptorValue->GoalId;
        ArmyMissionOrderValue.OwningArmyIndex = 0;
        uint32_t ArmyMissionOrderIdValue = 0U;
        CommandTaskAdmissionServiceValue.TryAdmitGoalDrivenOrder(GameStateDescriptorValue, ArmyMissionOrderValue,
                                                                 ArmyMissionOrderIdValue);
    }

    const std::array<const std::vector<FGoalDescriptor>*, 3U> GoalListsValue = {
        &GameStateDescriptorValue.GoalSet.ImmediateGoals,
        &GameStateDescriptorValue.GoalSet.NearTermGoals,
        &GameStateDescriptorValue.GoalSet.StrategicGoals};

    for (const std::vector<FGoalDescriptor>* GoalListValue : GoalListsValue)
    {
        if (GoalListValue == nullptr)
        {
            continue;
        }

        for (const FGoalDescriptor& GoalDescriptorValue : *GoalListValue)
        {
            if (!IsGoalEligibleForStrategicSeeding(GoalDescriptorValue) ||
                HasActiveStrategicOrderForGoal(CommandAuthoritySchedulingStateValue, GoalDescriptorValue))
            {
                continue;
            }

            FCommandOrderRecord GoalOrderValue;
            if (!TryPopulateGoalOrder(GameStateDescriptorValue, GoalDescriptorValue,
                                      GameStateDescriptorValue.CurrentGameLoop, GoalOrderValue))
            {
                continue;
            }

            GoalOrderValue.SourceGoalId = GoalDescriptorValue.GoalId;
            if (DoesRemainingOpeningPlanCoverOrder(GameStateDescriptorValue, GoalOrderValue))
            {
                continue;
            }
            if (DoesIncompleteMandatoryOpeningStructureGuardBlockOrder(GameStateDescriptorValue, GoalOrderValue))
            {
                continue;
            }
            uint32_t GoalOrderIdValue = 0U;
            CommandTaskAdmissionServiceValue.TryAdmitGoalDrivenOrder(GameStateDescriptorValue, GoalOrderValue,
                                                                     GoalOrderIdValue);
        }
    }
}

void FCommandAuthorityProcessor::EnsureStrategicChildOrders(FGameStateDescriptor& GameStateDescriptorValue) const
{
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState;
    const std::vector<size_t> StrategicOrderIndicesValue = CommandAuthoritySchedulingStateValue.StrategicOrderIndices;

    for (const size_t StrategicOrderIndexValue : StrategicOrderIndicesValue)
    {
        const FCommandOrderRecord StrategicOrderValue =
            CommandAuthoritySchedulingStateValue.GetOrderRecord(StrategicOrderIndexValue);
        if (StrategicOrderValue.SourceLayer != ECommandAuthorityLayer::StrategicDirector ||
            IsTerminalLifecycleState(StrategicOrderValue.LifecycleState) ||
            (StrategicOrderValue.TargetCount > 0U &&
             DoesOrderTargetMatchObservedState(GameStateDescriptorValue, StrategicOrderValue)))
        {
            continue;
        }

        if (StrategicOrderValue.TaskType == ECommandTaskType::ArmyMission)
        {
            size_t ArmyChildOrderIndexValue = 0U;
            if (CommandAuthoritySchedulingStateValue.TryGetActiveChildOrderIndex(
                    StrategicOrderValue.OrderId, ECommandAuthorityLayer::Army, ArmyChildOrderIndexValue))
            {
                continue;
            }

            FCommandOrderRecord ArmyOrderValue = FCommandOrderRecord::CreateNoTarget(
                ECommandAuthorityLayer::Army, NullTag, ABILITY_ID::INVALID, StrategicOrderValue.BasePriorityValue,
                EIntentDomain::ArmyCombat, GameStateDescriptorValue.CurrentGameLoop, 0U, StrategicOrderValue.OrderId,
                StrategicOrderValue.OwningArmyIndex);
            ArmyOrderValue.SourceGoalId = StrategicOrderValue.SourceGoalId;
            ArmyOrderValue.TaskPackageKind = StrategicOrderValue.TaskPackageKind;
            ArmyOrderValue.TaskNeedKind = StrategicOrderValue.TaskNeedKind;
            ArmyOrderValue.TaskType = StrategicOrderValue.TaskType;
            ArmyOrderValue.Origin = StrategicOrderValue.Origin;
            ArmyOrderValue.CommitmentClass = StrategicOrderValue.CommitmentClass;
            ArmyOrderValue.ExecutionGuarantee = StrategicOrderValue.ExecutionGuarantee;
            ArmyOrderValue.EffectivePriorityValue = StrategicOrderValue.EffectivePriorityValue;
            ArmyOrderValue.PriorityTier = StrategicOrderValue.PriorityTier;
            ArmyOrderValue.OwningArmyIndex = StrategicOrderValue.OwningArmyIndex;
            CommandAuthoritySchedulingStateValue.EnqueueOrder(ArmyOrderValue);
            continue;
        }

        size_t EconomyChildOrderIndexValue = 0U;
        if (CommandAuthoritySchedulingStateValue.TryGetActiveChildOrderIndex(
                StrategicOrderValue.OrderId, ECommandAuthorityLayer::EconomyAndProduction, EconomyChildOrderIndexValue))
        {
            continue;
        }

        FCommandOrderRecord EconomyOrderValue = FCommandOrderRecord::CreateNoTarget(
            ECommandAuthorityLayer::EconomyAndProduction, NullTag, StrategicOrderValue.AbilityId,
            StrategicOrderValue.BasePriorityValue, StrategicOrderValue.IntentDomain,
            GameStateDescriptorValue.CurrentGameLoop, 0U, StrategicOrderValue.OrderId);
        EconomyOrderValue.SourceGoalId = StrategicOrderValue.SourceGoalId;
        EconomyOrderValue.TaskPackageKind = StrategicOrderValue.TaskPackageKind;
        EconomyOrderValue.TaskNeedKind = StrategicOrderValue.TaskNeedKind;
        EconomyOrderValue.TaskType = StrategicOrderValue.TaskType;
        EconomyOrderValue.Origin = StrategicOrderValue.Origin;
        EconomyOrderValue.CommitmentClass = StrategicOrderValue.CommitmentClass;
        EconomyOrderValue.ExecutionGuarantee = StrategicOrderValue.ExecutionGuarantee;
        EconomyOrderValue.EffectivePriorityValue = StrategicOrderValue.EffectivePriorityValue;
        EconomyOrderValue.PriorityTier = StrategicOrderValue.PriorityTier;
        EconomyOrderValue.PlanStepId = StrategicOrderValue.PlanStepId;
        EconomyOrderValue.TargetCount = StrategicOrderValue.TargetCount;
        EconomyOrderValue.RequestedQueueCount = StrategicOrderValue.RequestedQueueCount;
        EconomyOrderValue.ProducerUnitTypeId = StrategicOrderValue.ProducerUnitTypeId;
        EconomyOrderValue.ResultUnitTypeId = StrategicOrderValue.ResultUnitTypeId;
        EconomyOrderValue.UpgradeId = StrategicOrderValue.UpgradeId;
        EconomyOrderValue.PreferredPlacementSlotType = StrategicOrderValue.PreferredPlacementSlotType;
        EconomyOrderValue.PreferredPlacementSlotId = StrategicOrderValue.PreferredPlacementSlotId;
        EconomyOrderValue.PreferredProducerPlacementSlotId = StrategicOrderValue.PreferredProducerPlacementSlotId;
        CommandAuthoritySchedulingStateValue.EnqueueOrder(EconomyOrderValue);
    }
}

bool FCommandAuthorityProcessor::AreRequiredTasksCompleted(
    const FOpeningPlanExecutionState& OpeningPlanExecutionStateValue,
    const FCommandTaskDescriptor& CommandTaskDescriptorValue) const
{
    for (const uint32_t RequiredStepIdValue : CommandTaskDescriptorValue.TriggerRequiredCompletedTaskIds)
    {
        if (!OpeningPlanExecutionStateValue.IsStepCompleted(RequiredStepIdValue))
        {
            return false;
        }
    }

    return true;
}

bool FCommandAuthorityProcessor::DoesOrderTargetMatchObservedState(
    const FGameStateDescriptor& GameStateDescriptorValue, const FCommandOrderRecord& CommandOrderRecordValue) const
{
    if (CommandOrderRecordValue.TargetCount == 0U)
    {
        return false;
    }

    if (ShouldUseExactPlacementSlotObservedMatch(CommandOrderRecordValue))
    {
        return DoesExactPlacementSlotContainExpectedStructure(GameStateDescriptorValue, CommandOrderRecordValue);
    }

    return GetObservedCountForOrder(GameStateDescriptorValue.BuildPlanning, CommandOrderRecordValue) >=
           CommandOrderRecordValue.TargetCount;
}

uint32_t FCommandAuthorityProcessor::GetObservedCountForOrder(const FBuildPlanningState& BuildPlanningStateValue,
                                                              const FCommandOrderRecord& CommandOrderRecordValue) const
{
    if (CommandOrderRecordValue.UpgradeId.ToType() != UPGRADE_ID::INVALID)
    {
        return GetUpgradeCount(BuildPlanningStateValue.ObservedCompletedUpgradeCounts, CommandOrderRecordValue.UpgradeId);
    }

    switch (CommandOrderRecordValue.ResultUnitTypeId)
    {
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
            return BuildPlanningStateValue.ObservedTownHallCount;
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
            return BuildPlanningStateValue.ObservedOrbitalCommandCount;
        default:
            break;
    }

    if (IsTerranBuilding(CommandOrderRecordValue.ResultUnitTypeId))
    {
        return GetBuildingCount(BuildPlanningStateValue.ObservedBuildingCounts, CommandOrderRecordValue.ResultUnitTypeId);
    }

    return GetUnitCount(BuildPlanningStateValue.ObservedUnitCounts, CommandOrderRecordValue.ResultUnitTypeId);
}

}  // namespace sc2
