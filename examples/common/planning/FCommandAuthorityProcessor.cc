#include "common/planning/FCommandAuthorityProcessor.h"

#include "common/build_orders/FOpeningPlanRegistry.h"
#include "common/terran_models.h"

namespace sc2
{
namespace
{

EIntentDomain DetermineIntentDomain(const FCommandOrderRecord& CommandOrderRecordValue)
{
    if (CommandOrderRecordValue.TaskType == ECommandTaskType::ArmyMission)
    {
        return EIntentDomain::ArmyCombat;
    }

    switch (CommandOrderRecordValue.AbilityId.ToType())
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

bool ShouldUseExactWallSlotObservedMatch(const FGameStateDescriptor& GameStateDescriptorValue,
                                         const FCommandOrderRecord& CommandOrderRecordValue)
{
    return CommandOrderRecordValue.PreferredPlacementSlotType != EBuildPlacementSlotType::Unknown &&
           GameStateDescriptorValue.RampWallDescriptor.bIsValid &&
           IsRampWallSlotType(CommandOrderRecordValue.PreferredPlacementSlotType);
}

bool DoesExactWallSlotContainExpectedStructure(const FGameStateDescriptor& GameStateDescriptorValue,
                                               const FCommandOrderRecord& CommandOrderRecordValue)
{
    return GameStateDescriptorValue.ObservedRampWallState.GetObservedWallSlotState(
               CommandOrderRecordValue.PreferredPlacementSlotType) == EObservedWallSlotState::Occupied;
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

bool TryPopulateUpgradeGoalOrder(const FGoalDescriptor& GoalDescriptorValue, const uint64_t CurrentGameLoopValue,
                                 FCommandOrderRecord& CommandOrderRecordValue)
{
    const ABILITY_ID AbilityIdValue = TerranUpgradeIdToResearchAbilityId(GoalDescriptorValue.TargetUpgradeId);
    if (AbilityIdValue == ABILITY_ID::INVALID)
    {
        return false;
    }

    UNIT_TYPEID ProducerUnitTypeIdValue = UNIT_TYPEID::INVALID;
    switch (GoalDescriptorValue.TargetUpgradeId.ToType())
    {
        case UPGRADE_ID::STIMPACK:
        case UPGRADE_ID::SHIELDWALL:
        case UPGRADE_ID::PUNISHERGRENADES:
            ProducerUnitTypeIdValue = UNIT_TYPEID::TERRAN_BARRACKSTECHLAB;
            break;
        case UPGRADE_ID::TERRANINFANTRYWEAPONSLEVEL1:
            ProducerUnitTypeIdValue = UNIT_TYPEID::TERRAN_ENGINEERINGBAY;
            break;
        default:
            return false;
    }

    CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::StrategicDirector, NullTag, AbilityIdValue, GoalDescriptorValue.BasePriorityValue,
        EIntentDomain::UnitProduction, CurrentGameLoopValue);
    CommandOrderRecordValue.TaskPackageKind = ECommandTaskPackageKind::TechTransition;
    CommandOrderRecordValue.TaskNeedKind = ECommandTaskNeedKind::Upgrade;
    CommandOrderRecordValue.TaskType = ECommandTaskType::UpgradeResearch;
    CommandOrderRecordValue.ProducerUnitTypeId = ProducerUnitTypeIdValue;
    CommandOrderRecordValue.UpgradeId = GoalDescriptorValue.TargetUpgradeId;
    CommandOrderRecordValue.TargetCount = std::max<uint32_t>(1U, GoalDescriptorValue.TargetCount);
    return true;
}

bool TryPopulateGoalOrder(const FGoalDescriptor& GoalDescriptorValue, const uint64_t CurrentGameLoopValue,
                          FCommandOrderRecord& CommandOrderRecordValue)
{
    switch (GoalDescriptorValue.GoalType)
    {
        case EGoalType::SaturateWorkers:
            CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
                ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::TRAIN_SCV,
                GoalDescriptorValue.BasePriorityValue, EIntentDomain::UnitProduction, CurrentGameLoopValue);
            CommandOrderRecordValue.TaskPackageKind = ECommandTaskPackageKind::Macro;
            CommandOrderRecordValue.TaskNeedKind = ECommandTaskNeedKind::Unit;
            CommandOrderRecordValue.TaskType = ECommandTaskType::WorkerProduction;
            CommandOrderRecordValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_COMMANDCENTER;
            CommandOrderRecordValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
            CommandOrderRecordValue.TargetCount = GoalDescriptorValue.TargetCount;
            return true;
        case EGoalType::MaintainSupply:
            CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
                ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::BUILD_SUPPLYDEPOT,
                GoalDescriptorValue.BasePriorityValue, EIntentDomain::StructureBuild, CurrentGameLoopValue);
            CommandOrderRecordValue.TaskPackageKind = ECommandTaskPackageKind::Supply;
            CommandOrderRecordValue.TaskNeedKind = ECommandTaskNeedKind::Structure;
            CommandOrderRecordValue.TaskType = ECommandTaskType::Supply;
            CommandOrderRecordValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
            CommandOrderRecordValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_SUPPLYDEPOT;
            CommandOrderRecordValue.TargetCount = GoalDescriptorValue.TargetCount;
            return true;
        case EGoalType::ExpandBaseCount:
            CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
                ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::BUILD_COMMANDCENTER,
                GoalDescriptorValue.BasePriorityValue, EIntentDomain::StructureBuild, CurrentGameLoopValue);
            CommandOrderRecordValue.TaskPackageKind = ECommandTaskPackageKind::Expansion;
            CommandOrderRecordValue.TaskNeedKind = ECommandTaskNeedKind::Expansion;
            CommandOrderRecordValue.TaskType = ECommandTaskType::Expansion;
            CommandOrderRecordValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
            CommandOrderRecordValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_COMMANDCENTER;
            CommandOrderRecordValue.TargetCount = GoalDescriptorValue.TargetCount;
            return true;
        case EGoalType::BuildProductionCapacity:
            switch (GoalDescriptorValue.TargetUnitTypeId)
            {
                case UNIT_TYPEID::TERRAN_REFINERY:
                    CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
                        ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::BUILD_REFINERY,
                        GoalDescriptorValue.BasePriorityValue, EIntentDomain::StructureBuild, CurrentGameLoopValue);
                    CommandOrderRecordValue.TaskPackageKind = ECommandTaskPackageKind::Macro;
                    CommandOrderRecordValue.TaskNeedKind = ECommandTaskNeedKind::Structure;
                    CommandOrderRecordValue.TaskType = ECommandTaskType::Refinery;
                    CommandOrderRecordValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
                    CommandOrderRecordValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_REFINERY;
                    CommandOrderRecordValue.TargetCount = GoalDescriptorValue.TargetCount;
                    return true;
                case UNIT_TYPEID::TERRAN_BARRACKS:
                    CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
                        ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::BUILD_BARRACKS,
                        GoalDescriptorValue.BasePriorityValue, EIntentDomain::StructureBuild, CurrentGameLoopValue);
                    CommandOrderRecordValue.TaskPackageKind = ECommandTaskPackageKind::ProductionScale;
                    CommandOrderRecordValue.TaskNeedKind = ECommandTaskNeedKind::Structure;
                    CommandOrderRecordValue.TaskType = ECommandTaskType::ProductionStructure;
                    CommandOrderRecordValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
                    CommandOrderRecordValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKS;
                    CommandOrderRecordValue.TargetCount = GoalDescriptorValue.TargetCount;
                    return true;
                case UNIT_TYPEID::TERRAN_FACTORY:
                    CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
                        ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::BUILD_FACTORY,
                        GoalDescriptorValue.BasePriorityValue, EIntentDomain::StructureBuild, CurrentGameLoopValue);
                    CommandOrderRecordValue.TaskPackageKind = ECommandTaskPackageKind::ProductionScale;
                    CommandOrderRecordValue.TaskNeedKind = ECommandTaskNeedKind::Structure;
                    CommandOrderRecordValue.TaskType = ECommandTaskType::ProductionStructure;
                    CommandOrderRecordValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
                    CommandOrderRecordValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_FACTORY;
                    CommandOrderRecordValue.TargetCount = GoalDescriptorValue.TargetCount;
                    return true;
                case UNIT_TYPEID::TERRAN_STARPORT:
                    CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
                        ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::BUILD_STARPORT,
                        GoalDescriptorValue.BasePriorityValue, EIntentDomain::StructureBuild, CurrentGameLoopValue);
                    CommandOrderRecordValue.TaskPackageKind = ECommandTaskPackageKind::ProductionScale;
                    CommandOrderRecordValue.TaskNeedKind = ECommandTaskNeedKind::Structure;
                    CommandOrderRecordValue.TaskType = ECommandTaskType::ProductionStructure;
                    CommandOrderRecordValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
                    CommandOrderRecordValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_STARPORT;
                    CommandOrderRecordValue.TargetCount = GoalDescriptorValue.TargetCount;
                    return true;
                case UNIT_TYPEID::TERRAN_ENGINEERINGBAY:
                    CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
                        ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::BUILD_ENGINEERINGBAY,
                        GoalDescriptorValue.BasePriorityValue, EIntentDomain::StructureBuild, CurrentGameLoopValue);
                    CommandOrderRecordValue.TaskPackageKind = ECommandTaskPackageKind::TechTransition;
                    CommandOrderRecordValue.TaskNeedKind = ECommandTaskNeedKind::Structure;
                    CommandOrderRecordValue.TaskType = ECommandTaskType::TechStructure;
                    CommandOrderRecordValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
                    CommandOrderRecordValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_ENGINEERINGBAY;
                    CommandOrderRecordValue.TargetCount = GoalDescriptorValue.TargetCount;
                    return true;
                default:
                    return false;
            }
        case EGoalType::UnlockTechnology:
            switch (GoalDescriptorValue.TargetUnitTypeId)
            {
                case UNIT_TYPEID::TERRAN_BARRACKSREACTOR:
                    CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
                        ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::BUILD_REACTOR_BARRACKS,
                        GoalDescriptorValue.BasePriorityValue, EIntentDomain::StructureBuild, CurrentGameLoopValue);
                    CommandOrderRecordValue.TaskPackageKind = ECommandTaskPackageKind::TechTransition;
                    CommandOrderRecordValue.TaskNeedKind = ECommandTaskNeedKind::AddOn;
                    CommandOrderRecordValue.TaskType = ECommandTaskType::AddOn;
                    CommandOrderRecordValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKS;
                    CommandOrderRecordValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKSREACTOR;
                    CommandOrderRecordValue.TargetCount = GoalDescriptorValue.TargetCount;
                    return true;
                case UNIT_TYPEID::TERRAN_FACTORYTECHLAB:
                    CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
                        ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::BUILD_TECHLAB_FACTORY,
                        GoalDescriptorValue.BasePriorityValue, EIntentDomain::StructureBuild, CurrentGameLoopValue);
                    CommandOrderRecordValue.TaskPackageKind = ECommandTaskPackageKind::TechTransition;
                    CommandOrderRecordValue.TaskNeedKind = ECommandTaskNeedKind::AddOn;
                    CommandOrderRecordValue.TaskType = ECommandTaskType::AddOn;
                    CommandOrderRecordValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_FACTORY;
                    CommandOrderRecordValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_FACTORYTECHLAB;
                    CommandOrderRecordValue.TargetCount = GoalDescriptorValue.TargetCount;
                    return true;
                case UNIT_TYPEID::TERRAN_STARPORTREACTOR:
                    CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
                        ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::BUILD_REACTOR_STARPORT,
                        GoalDescriptorValue.BasePriorityValue, EIntentDomain::StructureBuild, CurrentGameLoopValue);
                    CommandOrderRecordValue.TaskPackageKind = ECommandTaskPackageKind::TechTransition;
                    CommandOrderRecordValue.TaskNeedKind = ECommandTaskNeedKind::AddOn;
                    CommandOrderRecordValue.TaskType = ECommandTaskType::AddOn;
                    CommandOrderRecordValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_STARPORT;
                    CommandOrderRecordValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_STARPORTREACTOR;
                    CommandOrderRecordValue.TargetCount = GoalDescriptorValue.TargetCount;
                    return true;
                case UNIT_TYPEID::TERRAN_ENGINEERINGBAY:
                    CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
                        ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::BUILD_ENGINEERINGBAY,
                        GoalDescriptorValue.BasePriorityValue, EIntentDomain::StructureBuild, CurrentGameLoopValue);
                    CommandOrderRecordValue.TaskPackageKind = ECommandTaskPackageKind::TechTransition;
                    CommandOrderRecordValue.TaskNeedKind = ECommandTaskNeedKind::Structure;
                    CommandOrderRecordValue.TaskType = ECommandTaskType::TechStructure;
                    CommandOrderRecordValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
                    CommandOrderRecordValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_ENGINEERINGBAY;
                    CommandOrderRecordValue.TargetCount = GoalDescriptorValue.TargetCount;
                    return true;
                default:
                    return false;
            }
        case EGoalType::ResearchUpgrade:
            return TryPopulateUpgradeGoalOrder(GoalDescriptorValue, CurrentGameLoopValue, CommandOrderRecordValue);
        case EGoalType::ProduceArmy:
            switch (GoalDescriptorValue.TargetUnitTypeId)
            {
                case UNIT_TYPEID::TERRAN_MARINE:
                    CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
                        ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::TRAIN_MARINE,
                        GoalDescriptorValue.BasePriorityValue, EIntentDomain::UnitProduction, CurrentGameLoopValue);
                    CommandOrderRecordValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKS;
                    break;
                case UNIT_TYPEID::TERRAN_MARAUDER:
                    CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
                        ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::TRAIN_MARAUDER,
                        GoalDescriptorValue.BasePriorityValue, EIntentDomain::UnitProduction, CurrentGameLoopValue);
                    CommandOrderRecordValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKS;
                    break;
                case UNIT_TYPEID::TERRAN_CYCLONE:
                    CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
                        ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::TRAIN_CYCLONE,
                        GoalDescriptorValue.BasePriorityValue, EIntentDomain::UnitProduction, CurrentGameLoopValue);
                    CommandOrderRecordValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_FACTORY;
                    break;
                case UNIT_TYPEID::TERRAN_SIEGETANK:
                    CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
                        ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::TRAIN_SIEGETANK,
                        GoalDescriptorValue.BasePriorityValue, EIntentDomain::UnitProduction, CurrentGameLoopValue);
                    CommandOrderRecordValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_FACTORY;
                    break;
                case UNIT_TYPEID::TERRAN_MEDIVAC:
                    CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
                        ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::TRAIN_MEDIVAC,
                        GoalDescriptorValue.BasePriorityValue, EIntentDomain::UnitProduction, CurrentGameLoopValue);
                    CommandOrderRecordValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_STARPORT;
                    break;
                case UNIT_TYPEID::TERRAN_LIBERATOR:
                    CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
                        ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::TRAIN_LIBERATOR,
                        GoalDescriptorValue.BasePriorityValue, EIntentDomain::UnitProduction, CurrentGameLoopValue);
                    CommandOrderRecordValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_STARPORT;
                    break;
                default:
                    return false;
            }
            CommandOrderRecordValue.TaskPackageKind = ECommandTaskPackageKind::TimingAttack;
            CommandOrderRecordValue.TaskNeedKind = ECommandTaskNeedKind::Unit;
            CommandOrderRecordValue.TaskType = ECommandTaskType::UnitProduction;
            CommandOrderRecordValue.ResultUnitTypeId = GoalDescriptorValue.TargetUnitTypeId;
            CommandOrderRecordValue.TargetCount = GoalDescriptorValue.TargetCount;
            return true;
        default:
            return false;
    }
}

bool HasActiveStrategicOrderForGoal(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                    const FGoalDescriptor& GoalDescriptorValue)
{
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] !=
                ECommandAuthorityLayer::StrategicDirector ||
            !IsNonTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        const FCommandOrderRecord CommandOrderRecordValue =
            CommandAuthoritySchedulingStateValue.GetOrderRecord(OrderIndexValue);
        if (DoesOrderMatchGoal(CommandOrderRecordValue, GoalDescriptorValue))
        {
            return true;
        }
    }

    return false;
}

bool DoesRemainingOpeningPlanCoverOrder(const FGameStateDescriptor& GameStateDescriptorValue,
                                        const FCommandOrderRecord& CommandOrderRecordValue)
{
    const FOpeningPlanExecutionState& OpeningPlanExecutionStateValue = GameStateDescriptorValue.OpeningPlanExecutionState;
    const FOpeningPlanDescriptor& OpeningPlanDescriptorValue =
        FOpeningPlanRegistry::GetOpeningPlanDescriptor(OpeningPlanExecutionStateValue.ActivePlanId);

    for (const FOpeningPlanStep& OpeningPlanStepValue : OpeningPlanDescriptorValue.Steps)
    {
        const FCommandTaskDescriptor& CommandTaskDescriptorValue = OpeningPlanStepValue.TaskDescriptor;
        if (OpeningPlanExecutionStateValue.IsStepCompleted(CommandTaskDescriptorValue.TaskId))
        {
            continue;
        }

        if (CommandTaskDescriptorValue.ActionUpgradeId.ToType() != UPGRADE_ID::INVALID &&
            CommandTaskDescriptorValue.ActionUpgradeId == CommandOrderRecordValue.UpgradeId)
        {
            return CommandTaskDescriptorValue.ActionTargetCount >= CommandOrderRecordValue.TargetCount;
        }

        if (CommandTaskDescriptorValue.ActionAbilityId == CommandOrderRecordValue.AbilityId &&
            CommandTaskDescriptorValue.ActionResultUnitTypeId == CommandOrderRecordValue.ResultUnitTypeId &&
            CommandTaskDescriptorValue.ActionTargetCount >= CommandOrderRecordValue.TargetCount)
        {
            return true;
        }
    }

    return false;
}

}  // namespace

void FCommandAuthorityProcessor::ProcessSchedulerStep(FGameStateDescriptor& GameStateDescriptorValue) const
{
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState;

    CommandAuthoritySchedulingStateValue.BeginMutationBatch();
    InitializeOpeningPlan(GameStateDescriptorValue);
    UpdateCompletedOpeningSteps(GameStateDescriptorValue);
    SeedReadyStrategicOrders(GameStateDescriptorValue);
    SeedGoalDrivenStrategicOrders(GameStateDescriptorValue);
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
        if (!ShouldUseExactWallSlotObservedMatch(GameStateDescriptorValue, StrategicOrderRecordValue) ||
            DoesExactWallSlotContainExpectedStructure(GameStateDescriptorValue, StrategicOrderRecordValue))
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
        const FCommandTaskDescriptor& CommandTaskDescriptorValue = OpeningPlanStepValue.TaskDescriptor;
        if (OpeningPlanExecutionStateValue.IsStepCompleted(CommandTaskDescriptorValue.TaskId))
        {
            continue;
        }

        FCommandOrderRecord CompletionProbeValue;
        CompletionProbeValue.ResultUnitTypeId = CommandTaskDescriptorValue.ActionResultUnitTypeId;
        CompletionProbeValue.UpgradeId = CommandTaskDescriptorValue.ActionUpgradeId;
        CompletionProbeValue.TargetCount = CommandTaskDescriptorValue.CompletionObservedCountAtLeast;
        CompletionProbeValue.PreferredPlacementSlotType =
            CommandTaskDescriptorValue.ActionPreferredPlacementSlotType;
        CompletionProbeValue.PreferredPlacementSlotId = CommandTaskDescriptorValue.ActionPreferredPlacementSlotId;
        if (!DoesOrderTargetMatchObservedState(GameStateDescriptorValue, CompletionProbeValue))
        {
            continue;
        }

        OpeningPlanExecutionStateValue.MarkStepCompleted(CommandTaskDescriptorValue.TaskId);

        uint32_t StrategicOrderIdValue = 0U;
        if (OpeningPlanExecutionStateValue.TryGetPlanOrderId(CommandTaskDescriptorValue.TaskId, StrategicOrderIdValue))
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

    if (OpeningPlanExecutionStateValue.CompletedStepIds.size() >= OpeningPlanDescriptorValue.Steps.size())
    {
        OpeningPlanExecutionStateValue.LifecycleState = EOpeningPlanLifecycleState::Completed;
    }
}

void FCommandAuthorityProcessor::SeedReadyStrategicOrders(FGameStateDescriptor& GameStateDescriptorValue) const
{
    FOpeningPlanExecutionState& OpeningPlanExecutionStateValue = GameStateDescriptorValue.OpeningPlanExecutionState;
    const FOpeningPlanDescriptor& OpeningPlanDescriptorValue =
        FOpeningPlanRegistry::GetOpeningPlanDescriptor(OpeningPlanExecutionStateValue.ActivePlanId);
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState;

    uint32_t SeededOrderCountValue = 0U;
    for (const FOpeningPlanStep& OpeningPlanStepValue : OpeningPlanDescriptorValue.Steps)
    {
        const FCommandTaskDescriptor& CommandTaskDescriptorValue = OpeningPlanStepValue.TaskDescriptor;
        if (SeededOrderCountValue >= CommandAuthoritySchedulingStateValue.MaxStrategicOrdersPerStep)
        {
            return;
        }
        if (OpeningPlanExecutionStateValue.HasSeededStep(CommandTaskDescriptorValue.TaskId) ||
            OpeningPlanExecutionStateValue.IsStepCompleted(CommandTaskDescriptorValue.TaskId))
        {
            continue;
        }
        if (GameStateDescriptorValue.CurrentGameLoop < CommandTaskDescriptorValue.TriggerMinGameLoop)
        {
            return;
        }
        if (!AreRequiredTasksCompleted(OpeningPlanExecutionStateValue, CommandTaskDescriptorValue))
        {
            continue;
        }

        FCommandOrderRecord StrategicOrderValue = FCommandOrderRecord::CreateNoTarget(
            ECommandAuthorityLayer::StrategicDirector, NullTag, CommandTaskDescriptorValue.ActionAbilityId,
            CommandTaskDescriptorValue.BasePriorityValue, EIntentDomain::StructureBuild,
            CommandTaskDescriptorValue.TriggerMinGameLoop);
        StrategicOrderValue.TaskPackageKind = CommandTaskDescriptorValue.PackageKind;
        StrategicOrderValue.TaskNeedKind = CommandTaskDescriptorValue.NeedKind;
        StrategicOrderValue.TaskType = CommandTaskDescriptorValue.TaskType;
        StrategicOrderValue.SourceGoalId = CommandTaskDescriptorValue.SourceGoalId;
        StrategicOrderValue.PlanStepId = CommandTaskDescriptorValue.TaskId;
        StrategicOrderValue.TargetCount = CommandTaskDescriptorValue.ActionTargetCount;
        StrategicOrderValue.RequestedQueueCount = CommandTaskDescriptorValue.ActionRequestedQueueCount;
        StrategicOrderValue.ProducerUnitTypeId = CommandTaskDescriptorValue.ActionProducerUnitTypeId;
        StrategicOrderValue.ResultUnitTypeId = CommandTaskDescriptorValue.ActionResultUnitTypeId;
        StrategicOrderValue.UpgradeId = CommandTaskDescriptorValue.ActionUpgradeId;
        StrategicOrderValue.PreferredPlacementSlotType =
            CommandTaskDescriptorValue.ActionPreferredPlacementSlotType;
        StrategicOrderValue.PreferredPlacementSlotId = CommandTaskDescriptorValue.ActionPreferredPlacementSlotId;
        StrategicOrderValue.IntentDomain = DetermineIntentDomain(StrategicOrderValue);

        const uint32_t StrategicOrderIdValue =
            CommandAuthoritySchedulingStateValue.EnqueueOrder(StrategicOrderValue);
        OpeningPlanExecutionStateValue.RecordSeededStep(CommandTaskDescriptorValue.TaskId, StrategicOrderIdValue);
        ++SeededOrderCountValue;
    }
}

void FCommandAuthorityProcessor::SeedGoalDrivenStrategicOrders(FGameStateDescriptor& GameStateDescriptorValue) const
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
        ArmyMissionOrderValue.SourceGoalId = ArmyMissionGoalDescriptorValue->GoalId;
        ArmyMissionOrderValue.OwningArmyIndex = 0;
        CommandAuthoritySchedulingStateValue.EnqueueOrder(ArmyMissionOrderValue);
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
            if (!TryPopulateGoalOrder(GoalDescriptorValue, GameStateDescriptorValue.CurrentGameLoop, GoalOrderValue))
            {
                continue;
            }

            GoalOrderValue.SourceGoalId = GoalDescriptorValue.GoalId;
            if (DoesRemainingOpeningPlanCoverOrder(GameStateDescriptorValue, GoalOrderValue))
            {
                continue;
            }

            CommandAuthoritySchedulingStateValue.EnqueueOrder(GoalOrderValue);
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

    if (ShouldUseExactWallSlotObservedMatch(GameStateDescriptorValue, CommandOrderRecordValue))
    {
        return DoesExactWallSlotContainExpectedStructure(GameStateDescriptorValue, CommandOrderRecordValue);
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
