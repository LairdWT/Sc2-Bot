#include "common/planning/FTerranEconomyProductionOrderExpander.h"

#include <algorithm>
#include <limits>

#include "common/bot_status_models.h"
#include "common/economic_models.h"
#include "sc2api/sc2_map_info.h"
#include "sc2api/sc2_unit_filters.h"

namespace sc2
{
namespace
{

bool IsReactorUnitType(const UNIT_TYPEID UnitTypeValue)
{
    switch (UnitTypeValue)
    {
        case UNIT_TYPEID::TERRAN_REACTOR:
        case UNIT_TYPEID::TERRAN_BARRACKSREACTOR:
        case UNIT_TYPEID::TERRAN_FACTORYREACTOR:
        case UNIT_TYPEID::TERRAN_STARPORTREACTOR:
            return true;
        default:
            return false;
    }
}

bool IsTechLabUnitType(const UNIT_TYPEID UnitTypeValue)
{
    switch (UnitTypeValue)
    {
        case UNIT_TYPEID::TERRAN_TECHLAB:
        case UNIT_TYPEID::TERRAN_BARRACKSTECHLAB:
        case UNIT_TYPEID::TERRAN_FACTORYTECHLAB:
        case UNIT_TYPEID::TERRAN_STARPORTTECHLAB:
            return true;
        default:
            return false;
    }
}

bool IsOrderTerminal(const EOrderLifecycleState LifecycleStateValue)
{
    switch (LifecycleStateValue)
    {
        case EOrderLifecycleState::Completed:
        case EOrderLifecycleState::Aborted:
        case EOrderLifecycleState::Expired:
            return true;
        default:
            return false;
    }
}

bool IsSupplyDepotResultUnitType(const UNIT_TYPEID ResultUnitTypeIdValue)
{
    return ResultUnitTypeIdValue == UNIT_TYPEID::TERRAN_SUPPLYDEPOT;
}

bool IsRampWallSlotType(const EBuildPlacementSlotType BuildPlacementSlotTypeValue)
{
    switch (BuildPlacementSlotTypeValue)
    {
        case EBuildPlacementSlotType::MainRampDepotLeft:
        case EBuildPlacementSlotType::MainRampBarracksWithAddon:
        case EBuildPlacementSlotType::MainRampDepotRight:
            return true;
        default:
            return false;
    }
}

bool DoesPlacementSlotSatisfyFootprintPolicy(const FFrameContext& FrameValue,
                                             const FBuildPlacementSlot& BuildPlacementSlotValue,
                                             const ABILITY_ID StructureAbilityIdValue);

uint32_t GetObservedBuildingCount(const FBuildPlanningState& BuildPlanningStateValue, const UNIT_TYPEID BuildingTypeIdValue)
{
    switch (BuildingTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
            return BuildPlanningStateValue.ObservedTownHallCount;
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
            return BuildPlanningStateValue.ObservedOrbitalCommandCount;
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOT:
            return static_cast<uint32_t>(
                       BuildPlanningStateValue.ObservedBuildingCounts[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_SUPPLYDEPOT)]) +
                   static_cast<uint32_t>(BuildPlanningStateValue.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED)]);
        case UNIT_TYPEID::TERRAN_BARRACKS:
            return static_cast<uint32_t>(
                       BuildPlanningStateValue.ObservedBuildingCounts[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_BARRACKS)]) +
                   static_cast<uint32_t>(BuildPlanningStateValue.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_BARRACKSFLYING)]);
        case UNIT_TYPEID::TERRAN_FACTORY:
            return static_cast<uint32_t>(
                       BuildPlanningStateValue.ObservedBuildingCounts[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_FACTORY)]) +
                   static_cast<uint32_t>(BuildPlanningStateValue.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_FACTORYFLYING)]);
        case UNIT_TYPEID::TERRAN_STARPORT:
            return static_cast<uint32_t>(
                       BuildPlanningStateValue.ObservedBuildingCounts[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_STARPORT)]) +
                   static_cast<uint32_t>(BuildPlanningStateValue.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_STARPORTFLYING)]);
        case UNIT_TYPEID::TERRAN_REFINERY:
            return static_cast<uint32_t>(
                       BuildPlanningStateValue.ObservedBuildingCounts[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_REFINERY)]) +
                   static_cast<uint32_t>(BuildPlanningStateValue.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_REFINERYRICH)]);
        default:
        {
            const size_t BuildingTypeIndexValue = GetTerranBuildingTypeIndex(BuildingTypeIdValue);
            return IsTerranBuildingTypeIndexValid(BuildingTypeIndexValue)
                       ? static_cast<uint32_t>(BuildPlanningStateValue.ObservedBuildingCounts[BuildingTypeIndexValue])
                       : 0U;
        }
    }
}

uint32_t GetObservedUnitCount(const FBuildPlanningState& BuildPlanningStateValue, const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_HELLION:
            return static_cast<uint32_t>(
                       BuildPlanningStateValue.ObservedUnitCounts[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_HELLION)]) +
                   static_cast<uint32_t>(
                       BuildPlanningStateValue.ObservedUnitCounts[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_HELLIONTANK)]);
        case UNIT_TYPEID::TERRAN_LIBERATOR:
            return static_cast<uint32_t>(
                       BuildPlanningStateValue.ObservedUnitCounts[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_LIBERATOR)]) +
                   static_cast<uint32_t>(
                       BuildPlanningStateValue.ObservedUnitCounts[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_LIBERATORAG)]);
        case UNIT_TYPEID::TERRAN_SIEGETANK:
            return static_cast<uint32_t>(
                       BuildPlanningStateValue.ObservedUnitCounts[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_SIEGETANK)]) +
                   static_cast<uint32_t>(BuildPlanningStateValue.ObservedUnitCounts[GetTerranUnitTypeIndex(
                       UNIT_TYPEID::TERRAN_SIEGETANKSIEGED)]);
        default:
        {
            const size_t UnitTypeIndexValue = GetTerranUnitTypeIndex(UnitTypeIdValue);
            return IsTerranUnitTypeIndexValid(UnitTypeIndexValue)
                       ? static_cast<uint32_t>(BuildPlanningStateValue.ObservedUnitCounts[UnitTypeIndexValue])
                       : 0U;
        }
    }
}

uint32_t GetObservedUpgradeCount(const FBuildPlanningState& BuildPlanningStateValue, const UpgradeID UpgradeIdValue)
{
    const size_t UpgradeTypeIndexValue = GetTerranUpgradeTypeIndex(UpgradeIdValue);
    return IsTerranUpgradeTypeIndexValid(UpgradeTypeIndexValue)
               ? static_cast<uint32_t>(BuildPlanningStateValue.ObservedCompletedUpgradeCounts[UpgradeTypeIndexValue])
               : 0U;
}

uint32_t GetObservedCountForOrder(const FBuildPlanningState& BuildPlanningStateValue,
                                  const FCommandOrderRecord& CommandOrderRecordValue)
{
    if (CommandOrderRecordValue.UpgradeId.ToType() != UPGRADE_ID::INVALID)
    {
        return GetObservedUpgradeCount(BuildPlanningStateValue, CommandOrderRecordValue.UpgradeId);
    }

    if (IsTerranBuilding(CommandOrderRecordValue.ResultUnitTypeId) ||
        CommandOrderRecordValue.ResultUnitTypeId == UNIT_TYPEID::TERRAN_COMMANDCENTER)
    {
        return GetObservedBuildingCount(BuildPlanningStateValue, CommandOrderRecordValue.ResultUnitTypeId);
    }

    return GetObservedUnitCount(BuildPlanningStateValue, CommandOrderRecordValue.ResultUnitTypeId);
}

bool DoesOrderTargetMatchObservedState(const FGameStateDescriptor& GameStateDescriptorValue,
                                       const FCommandOrderRecord& CommandOrderRecordValue)
{
    if (CommandOrderRecordValue.TargetCount == 0U)
    {
        return false;
    }

    if (CommandOrderRecordValue.PreferredPlacementSlotType != EBuildPlacementSlotType::Unknown &&
        GameStateDescriptorValue.RampWallDescriptor.bIsValid &&
        IsRampWallSlotType(CommandOrderRecordValue.PreferredPlacementSlotType))
    {
        return GameStateDescriptorValue.ObservedRampWallState.GetObservedWallSlotState(
                   CommandOrderRecordValue.PreferredPlacementSlotType) == EObservedWallSlotState::Occupied;
    }

    return GetObservedCountForOrder(GameStateDescriptorValue.BuildPlanning, CommandOrderRecordValue) >=
           CommandOrderRecordValue.TargetCount;
}

bool ShouldUseReservedPlacementSlotAwaitingState(const FCommandOrderRecord& CommandOrderRecordValue)
{
    return CommandOrderRecordValue.LastDeferralReason == ECommandOrderDeferralReason::AwaitingObservedCompletion &&
           CommandOrderRecordValue.ReservedPlacementSlotId.IsValid();
}

uint32_t GetObservedInConstructionCountForOrder(const FBuildPlanningState& BuildPlanningStateValue,
                                                const FCommandOrderRecord& CommandOrderRecordValue)
{
    if (CommandOrderRecordValue.UpgradeId.ToType() != UPGRADE_ID::INVALID)
    {
        return 0U;
    }

    if (CommandOrderRecordValue.ResultUnitTypeId == UNIT_TYPEID::TERRAN_COMMANDCENTER)
    {
        return static_cast<uint32_t>(BuildPlanningStateValue.ObservedBuildingsInConstruction[GetTerranBuildingTypeIndex(
            UNIT_TYPEID::TERRAN_COMMANDCENTER)]);
    }

    if (IsTerranBuilding(CommandOrderRecordValue.ResultUnitTypeId))
    {
        const size_t BuildingTypeIndexValue = GetTerranBuildingTypeIndex(CommandOrderRecordValue.ResultUnitTypeId);
        return IsTerranBuildingTypeIndexValid(BuildingTypeIndexValue)
                   ? static_cast<uint32_t>(BuildPlanningStateValue.ObservedBuildingsInConstruction[BuildingTypeIndexValue])
                   : 0U;
    }

    const size_t UnitTypeIndexValue = GetTerranUnitTypeIndex(CommandOrderRecordValue.ResultUnitTypeId);
    return IsTerranUnitTypeIndexValid(UnitTypeIndexValue)
               ? static_cast<uint32_t>(BuildPlanningStateValue.ObservedUnitsInConstruction[UnitTypeIndexValue])
               : 0U;
}

uint32_t CountCurrentOrdersForAbility(const FAgentState& AgentStateValue, const ABILITY_ID AbilityIdValue)
{
    uint32_t OrderCountValue = 0U;
    for (const Unit* UnitValue : AgentStateValue.UnitContainer.ControlledUnits)
    {
        if (UnitValue == nullptr)
        {
            continue;
        }

        for (const UnitOrder& UnitOrderValue : UnitValue->orders)
        {
            if (UnitOrderValue.ability_id == AbilityIdValue)
            {
                ++OrderCountValue;
            }
        }
    }

    return OrderCountValue;
}

uint32_t CountPendingSchedulerOrdersForAbility(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                               const ABILITY_ID AbilityIdValue)
{
    uint32_t OrderCountValue = 0U;
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] != ECommandAuthorityLayer::UnitExecution ||
            CommandAuthoritySchedulingStateValue.AbilityIds[OrderIndexValue] != AbilityIdValue ||
            IsOrderTerminal(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        ++OrderCountValue;
    }

    return OrderCountValue;
}

uint32_t CountPendingIntentsForAbility(const FIntentBuffer& IntentBufferValue, const ABILITY_ID AbilityIdValue)
{
    uint32_t IntentCountValue = 0U;
    for (const FUnitIntent& IntentValue : IntentBufferValue.Intents)
    {
        if (IntentValue.Ability == AbilityIdValue)
        {
            ++IntentCountValue;
        }
    }

    return IntentCountValue;
}

ECommandOrderDeferralReason GetWorkerAvailabilityDeferralReason(const FAgentState& AgentStateValue)
{
    return AgentStateValue.Units.GetWorkerCount() == 0U ? ECommandOrderDeferralReason::NoProducer
                                                        : ECommandOrderDeferralReason::ProducerBusy;
}

bool HasActiveSchedulerOrderForActor(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                     const Tag ActorTagValue)
{
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.ActorTags[OrderIndexValue] != ActorTagValue ||
            IsOrderTerminal(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        return true;
    }

    return false;
}

uint32_t CountActiveSchedulerOrdersForActor(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                            const Tag ActorTagValue)
{
    uint32_t OrderCountValue = 0U;
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.ActorTags[OrderIndexValue] == ActorTagValue &&
            !IsOrderTerminal(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            ++OrderCountValue;
        }
    }

    return OrderCountValue;
}

uint32_t CountActiveUnitProductionSchedulerOrdersForActor(
    const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue, const Tag ActorTagValue)
{
    uint32_t OrderCountValue = 0U;
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.ActorTags[OrderIndexValue] != ActorTagValue ||
            IsOrderTerminal(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]) ||
            CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] != ECommandAuthorityLayer::UnitExecution ||
            CommandAuthoritySchedulingStateValue.IntentDomains[OrderIndexValue] != EIntentDomain::UnitProduction)
        {
            continue;
        }

        ++OrderCountValue;
    }

    return OrderCountValue;
}

bool HasConflictingSchedulerOrderForProductionActor(
    const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue, const Tag ActorTagValue)
{
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.ActorTags[OrderIndexValue] != ActorTagValue ||
            IsOrderTerminal(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        if (CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] == ECommandAuthorityLayer::UnitExecution &&
            CommandAuthoritySchedulingStateValue.IntentDomains[OrderIndexValue] == EIntentDomain::UnitProduction)
        {
            continue;
        }

        return true;
    }

    return false;
}

bool HasActiveUnitExecutionChild(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                 const uint32_t ParentOrderIdValue)
{
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.ParentOrderIds[OrderIndexValue] == ParentOrderIdValue &&
            CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] == ECommandAuthorityLayer::UnitExecution &&
            !IsOrderTerminal(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            return true;
        }
    }

    return false;
}

uint32_t CountActiveUnitExecutionChildrenForParent(
    const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue, const uint32_t ParentOrderIdValue)
{
    uint32_t ActiveChildCountValue = 0U;
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.ParentOrderIds[OrderIndexValue] != ParentOrderIdValue ||
            CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] != ECommandAuthorityLayer::UnitExecution ||
            IsOrderTerminal(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        ++ActiveChildCountValue;
    }

    return ActiveChildCountValue;
}

bool TryGetLatestUnitExecutionChildDispatchGameLoop(
    const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue, const uint32_t ParentOrderIdValue,
    uint64_t& OutLatestDispatchGameLoopValue)
{
    bool HasLatestDispatchGameLoopValue = false;
    OutLatestDispatchGameLoopValue = 0U;

    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.ParentOrderIds[OrderIndexValue] != ParentOrderIdValue ||
            CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] != ECommandAuthorityLayer::UnitExecution)
        {
            continue;
        }

        const uint64_t DispatchGameLoopValue = CommandAuthoritySchedulingStateValue.DispatchGameLoops[OrderIndexValue];
        if (DispatchGameLoopValue == 0U || (HasLatestDispatchGameLoopValue &&
                                            DispatchGameLoopValue <= OutLatestDispatchGameLoopValue))
        {
            continue;
        }

        HasLatestDispatchGameLoopValue = true;
        OutLatestDispatchGameLoopValue = DispatchGameLoopValue;
    }

    return HasLatestDispatchGameLoopValue;
}

bool IsStructureBuildAbility(const ABILITY_ID AbilityIdValue)
{
    switch (AbilityIdValue)
    {
        case ABILITY_ID::BUILD_SUPPLYDEPOT:
        case ABILITY_ID::BUILD_BARRACKS:
        case ABILITY_ID::BUILD_FACTORY:
        case ABILITY_ID::BUILD_STARPORT:
        case ABILITY_ID::BUILD_COMMANDCENTER:
        case ABILITY_ID::BUILD_REFINERY:
        case ABILITY_ID::BUILD_BUNKER:
        case ABILITY_ID::BUILD_ENGINEERINGBAY:
            return true;
        default:
            return false;
    }
}

bool IsUnitProductionAbility(const ABILITY_ID AbilityIdValue)
{
    switch (AbilityIdValue)
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
            return true;
        default:
            return false;
    }
}

bool HasOutstandingSupplyDepotDemand(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                     const FBuildPlanningState& BuildPlanningStateValue)
{
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (IsOrderTerminal(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        if (!IsSupplyDepotResultUnitType(CommandAuthoritySchedulingStateValue.ResultUnitTypeIds[OrderIndexValue]))
        {
            continue;
        }

        FCommandOrderRecord SupplyDepotOrderValue =
            CommandAuthoritySchedulingStateValue.GetOrderRecord(OrderIndexValue);
        if (GetObservedCountForOrder(BuildPlanningStateValue, SupplyDepotOrderValue) <
            SupplyDepotOrderValue.TargetCount)
        {
            return true;
        }
    }

    return false;
}

int GetEffectiveEconomyOrderPriority(const FCommandOrderRecord& EconomyOrderValue,
                                     const FBuildPlanningState& BuildPlanningStateValue,
                                     const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue)
{
    (void)BuildPlanningStateValue;
    (void)CommandAuthoritySchedulingStateValue;
    return EconomyOrderValue.EffectivePriorityValue;
}

void CopyTaskMetadataToChildOrder(const FCommandOrderRecord& SourceOrderValue, FCommandOrderRecord& ChildOrderValue)
{
    ChildOrderValue.SourceGoalId = SourceOrderValue.SourceGoalId;
    ChildOrderValue.TaskPackageKind = SourceOrderValue.TaskPackageKind;
    ChildOrderValue.TaskNeedKind = SourceOrderValue.TaskNeedKind;
    ChildOrderValue.TaskType = SourceOrderValue.TaskType;
    ChildOrderValue.EffectivePriorityValue = SourceOrderValue.EffectivePriorityValue;
    ChildOrderValue.PriorityTier = SourceOrderValue.PriorityTier;
}

FBuildPlacementContext CreateBuildPlacementContext(const Point2D& BaseLocationValue,
                                                   const std::vector<Point2D>& ExpansionLocationsValue,
                                                   const FRampWallDescriptor& RampWallDescriptorValue,
                                                   const FMainBaseLayoutDescriptor& MainBaseLayoutDescriptorValue)
{
    FBuildPlacementContext BuildPlacementContextValue;
    BuildPlacementContextValue.BaseLocation = BaseLocationValue;
    BuildPlacementContextValue.RampWallDescriptor = RampWallDescriptorValue;
    BuildPlacementContextValue.MainBaseLayoutDescriptor = MainBaseLayoutDescriptorValue;

    float BestDistanceSquaredValue = std::numeric_limits<float>::max();
    for (const Point2D& ExpansionLocationValue : ExpansionLocationsValue)
    {
        const float DistanceSquaredValue = DistanceSquared2D(BaseLocationValue, ExpansionLocationValue);
        if (DistanceSquaredValue < 16.0f || DistanceSquaredValue >= BestDistanceSquaredValue)
        {
            continue;
        }

        BestDistanceSquaredValue = DistanceSquaredValue;
        BuildPlacementContextValue.NaturalLocation = ExpansionLocationValue;
    }

    return BuildPlacementContextValue;
}

bool TryFindPlacementSlotById(const std::vector<FBuildPlacementSlot>& BuildPlacementSlotsValue,
                              const FBuildPlacementSlotId& BuildPlacementSlotIdValue,
                              FBuildPlacementSlot& OutBuildPlacementSlotValue)
{
    for (const FBuildPlacementSlot& BuildPlacementSlotValue : BuildPlacementSlotsValue)
    {
        if (BuildPlacementSlotValue.SlotId != BuildPlacementSlotIdValue)
        {
            continue;
        }

        OutBuildPlacementSlotValue = BuildPlacementSlotValue;
        return true;
    }

    return false;
}

float GetPlacementSlotOccupancyRadiusSquared(const FBuildPlacementSlot& BuildPlacementSlotValue)
{
    switch (BuildPlacementSlotValue.SlotId.SlotType)
    {
        case EBuildPlacementSlotType::MainRampDepotLeft:
        case EBuildPlacementSlotType::MainRampBarracksWithAddon:
        case EBuildPlacementSlotType::MainRampDepotRight:
        case EBuildPlacementSlotType::NaturalApproachDepot:
        case EBuildPlacementSlotType::MainSupportDepot:
        case EBuildPlacementSlotType::MainBarracksWithAddon:
        case EBuildPlacementSlotType::MainFactoryWithAddon:
        case EBuildPlacementSlotType::MainStarportWithAddon:
        case EBuildPlacementSlotType::MainProductionWithAddon:
        case EBuildPlacementSlotType::MainSupportStructure:
        case EBuildPlacementSlotType::Unknown:
        default:
            return 6.25f;
    }
}

Point2D GetStructureFootprintHalfExtentsForUnitType(const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOT:
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED:
        case UNIT_TYPEID::TERRAN_REACTOR:
        case UNIT_TYPEID::TERRAN_BARRACKSREACTOR:
        case UNIT_TYPEID::TERRAN_FACTORYREACTOR:
        case UNIT_TYPEID::TERRAN_STARPORTREACTOR:
        case UNIT_TYPEID::TERRAN_TECHLAB:
        case UNIT_TYPEID::TERRAN_BARRACKSTECHLAB:
        case UNIT_TYPEID::TERRAN_FACTORYTECHLAB:
        case UNIT_TYPEID::TERRAN_STARPORTTECHLAB:
            return Point2D(1.0f, 1.0f);
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
        case UNIT_TYPEID::TERRAN_PLANETARYFORTRESS:
            return Point2D(2.5f, 2.5f);
        case UNIT_TYPEID::TERRAN_REFINERY:
        case UNIT_TYPEID::TERRAN_REFINERYRICH:
            return Point2D(1.5f, 1.5f);
        default:
            return Point2D(1.5f, 1.5f);
    }
}

Point2D GetStructureFootprintHalfExtentsForAbility(const ABILITY_ID StructureAbilityIdValue)
{
    switch (StructureAbilityIdValue)
    {
        case ABILITY_ID::BUILD_SUPPLYDEPOT:
            return Point2D(1.0f, 1.0f);
        case ABILITY_ID::BUILD_COMMANDCENTER:
            return Point2D(2.5f, 2.5f);
        case ABILITY_ID::BUILD_BARRACKS:
        case ABILITY_ID::BUILD_FACTORY:
        case ABILITY_ID::BUILD_STARPORT:
        case ABILITY_ID::BUILD_REFINERY:
        default:
            return Point2D(1.5f, 1.5f);
    }
}

bool DoAxisAlignedFootprintsOverlap(const Point2D& CenterPointOneValue, const Point2D& HalfExtentsOneValue,
                                    const Point2D& CenterPointTwoValue, const Point2D& HalfExtentsTwoValue)
{
    constexpr float FootprintTouchToleranceValue = 0.05f;
    return std::fabs(CenterPointOneValue.x - CenterPointTwoValue.x) <
               ((HalfExtentsOneValue.x + HalfExtentsTwoValue.x) - FootprintTouchToleranceValue) &&
           std::fabs(CenterPointOneValue.y - CenterPointTwoValue.y) <
               ((HalfExtentsOneValue.y + HalfExtentsTwoValue.y) - FootprintTouchToleranceValue);
}

const Unit* FindObservedStructureOccupyingPlacementSlot(const ObservationInterface& ObservationValue,
                                                        const ABILITY_ID StructureAbilityIdValue,
                                                        const FBuildPlacementSlot& BuildPlacementSlotValue)
{
    const Units SelfUnitsValue = ObservationValue.GetUnits(Unit::Alliance::Self);
    const Point2D SlotBuildPointValue = BuildPlacementSlotValue.BuildPoint;
    const Point2D SlotFootprintHalfExtentsValue =
        GetStructureFootprintHalfExtentsForAbility(StructureAbilityIdValue);

    const Unit* BestOccupyingUnitValue = nullptr;
    float BestDistanceSquaredValue = std::numeric_limits<float>::max();
    for (const Unit* SelfUnitValue : SelfUnitsValue)
    {
        if (SelfUnitValue == nullptr || !SelfUnitValue->is_building || SelfUnitValue->is_flying)
        {
            continue;
        }

        const Point2D ObservedStructureHalfExtentsValue =
            GetStructureFootprintHalfExtentsForUnitType(SelfUnitValue->unit_type.ToType());
        if (!DoAxisAlignedFootprintsOverlap(Point2D(SelfUnitValue->pos), ObservedStructureHalfExtentsValue,
                                            SlotBuildPointValue, SlotFootprintHalfExtentsValue))
        {
            continue;
        }

        const float DistanceSquaredValue = DistanceSquared2D(Point2D(SelfUnitValue->pos), SlotBuildPointValue);
        if (DistanceSquaredValue >= BestDistanceSquaredValue)
        {
            continue;
        }

        BestDistanceSquaredValue = DistanceSquaredValue;
        BestOccupyingUnitValue = SelfUnitValue;
    }

    return BestOccupyingUnitValue;
}

bool HasObservedBuildOrderTargetingPlacementSlot(const ObservationInterface& ObservationValue,
                                                 const ABILITY_ID AbilityIdValue,
                                                 const FBuildPlacementSlot& BuildPlacementSlotValue)
{
    const Units SelfUnitsValue = ObservationValue.GetUnits(Unit::Alliance::Self);
    const float OccupancyRadiusSquaredValue = GetPlacementSlotOccupancyRadiusSquared(BuildPlacementSlotValue);
    const Point2D SlotBuildPointValue = BuildPlacementSlotValue.BuildPoint;

    for (const Unit* SelfUnitValue : SelfUnitsValue)
    {
        if (SelfUnitValue == nullptr)
        {
            continue;
        }

        for (const UnitOrder& UnitOrderValue : SelfUnitValue->orders)
        {
            if (UnitOrderValue.ability_id != AbilityIdValue)
            {
                continue;
            }

            const float DistanceSquaredValue =
                DistanceSquared2D(UnitOrderValue.target_pos, SlotBuildPointValue);
            if (DistanceSquaredValue <= OccupancyRadiusSquaredValue)
            {
                return true;
            }
        }
    }

    return false;
}

bool IsPlacementSlotClaimedByOtherOrder(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                        const FBuildPlacementSlotId& BuildPlacementSlotIdValue,
                                        const uint32_t IgnoredOrderIdValue)
{
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.OrderIds[OrderIndexValue] == IgnoredOrderIdValue ||
            IsOrderTerminal(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        FBuildPlacementSlotId ReservedPlacementSlotIdValue;
        ReservedPlacementSlotIdValue.SlotType =
            CommandAuthoritySchedulingStateValue.ReservedPlacementSlotTypes[OrderIndexValue];
        ReservedPlacementSlotIdValue.Ordinal =
            CommandAuthoritySchedulingStateValue.ReservedPlacementSlotOrdinals[OrderIndexValue];
        if (ReservedPlacementSlotIdValue.IsValid() && ReservedPlacementSlotIdValue == BuildPlacementSlotIdValue)
        {
            return true;
        }
    }

    return false;
}

bool TrySelectPlacementSlotCandidate(const FFrameContext& FrameValue,
                                     const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                     const FCommandOrderRecord& EconomyOrderValue, const Unit& WorkerUnitValue,
                                     const FBuildPlacementSlot& BuildPlacementSlotValue,
                                     FBuildPlacementSlot& OutBuildPlacementSlotValue)
{
    if (FrameValue.Observation == nullptr || FrameValue.GameInfo == nullptr || FrameValue.Query == nullptr)
    {
        return false;
    }

    const Point2D ClampedBuildPointValue =
        ClampToPlayable(*FrameValue.GameInfo, BuildPlacementSlotValue.BuildPoint);
    FBuildPlacementSlot NormalizedBuildPlacementSlotValue = BuildPlacementSlotValue;
    NormalizedBuildPlacementSlotValue.BuildPoint = ClampedBuildPointValue;

    if (IsPlacementSlotClaimedByOtherOrder(CommandAuthoritySchedulingStateValue,
                                           NormalizedBuildPlacementSlotValue.SlotId,
                                           EconomyOrderValue.OrderId))
    {
        return false;
    }

    if (FindObservedStructureOccupyingPlacementSlot(*FrameValue.Observation, EconomyOrderValue.AbilityId,
                                                    NormalizedBuildPlacementSlotValue) != nullptr)
    {
        return false;
    }

    if (!DoesPlacementSlotSatisfyFootprintPolicy(FrameValue, NormalizedBuildPlacementSlotValue,
                                                 EconomyOrderValue.AbilityId))
    {
        return false;
    }

    if (!FrameValue.Query->Placement(EconomyOrderValue.AbilityId, ClampedBuildPointValue, &WorkerUnitValue))
    {
        return false;
    }

    OutBuildPlacementSlotValue = NormalizedBuildPlacementSlotValue;
    return true;
}

bool TrySelectStructurePlacementSlot(const FFrameContext& FrameValue,
                                     const FGameStateDescriptor& GameStateDescriptorValue,
                                     const IBuildPlacementService& BuildPlacementServiceValue,
                                     const std::vector<Point2D>& ExpansionLocationsValue,
                                     const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                     const FCommandOrderRecord& EconomyOrderValue, const Unit& WorkerUnitValue,
                                     FBuildPlacementSlot& OutBuildPlacementSlotValue,
                                     ECommandOrderDeferralReason& OutDeferralReasonValue)
{
    if (FrameValue.Observation == nullptr || FrameValue.Query == nullptr)
    {
        OutDeferralReasonValue = ECommandOrderDeferralReason::NoValidPlacement;
        return false;
    }

    const Point2D BaseLocationValue = Point2D(FrameValue.Observation->GetStartLocation());
    const FBuildPlacementContext BuildPlacementContextValue = CreateBuildPlacementContext(
        BaseLocationValue, ExpansionLocationsValue, GameStateDescriptorValue.RampWallDescriptor,
        GameStateDescriptorValue.MainBaseLayoutDescriptor);
    const std::vector<FBuildPlacementSlot> BuildPlacementSlotsValue =
        BuildPlacementServiceValue.GetStructurePlacementSlots(GameStateDescriptorValue, EconomyOrderValue.AbilityId,
                                                              BuildPlacementContextValue);

    if (EconomyOrderValue.ReservedPlacementSlotId.IsValid())
    {
        FBuildPlacementSlot ReservedBuildPlacementSlotValue;
        if (!TryFindPlacementSlotById(BuildPlacementSlotsValue, EconomyOrderValue.ReservedPlacementSlotId,
                                      ReservedBuildPlacementSlotValue))
        {
            OutDeferralReasonValue = ECommandOrderDeferralReason::ReservedSlotInvalidated;
            return false;
        }

        if (!TrySelectPlacementSlotCandidate(FrameValue, CommandAuthoritySchedulingStateValue,
                                             EconomyOrderValue, WorkerUnitValue, ReservedBuildPlacementSlotValue,
                                             OutBuildPlacementSlotValue))
        {
            OutDeferralReasonValue = ECommandOrderDeferralReason::ReservedSlotOccupied;
            return false;
        }

        return true;
    }

    if (EconomyOrderValue.PreferredPlacementSlotId.IsValid())
    {
        FBuildPlacementSlot PreferredBuildPlacementSlotValue;
        if (!TryFindPlacementSlotById(BuildPlacementSlotsValue, EconomyOrderValue.PreferredPlacementSlotId,
                                      PreferredBuildPlacementSlotValue))
        {
            OutDeferralReasonValue = ECommandOrderDeferralReason::ReservedSlotInvalidated;
            return false;
        }

        if (!TrySelectPlacementSlotCandidate(FrameValue, CommandAuthoritySchedulingStateValue,
                                             EconomyOrderValue, WorkerUnitValue, PreferredBuildPlacementSlotValue,
                                             OutBuildPlacementSlotValue))
        {
            OutDeferralReasonValue = ECommandOrderDeferralReason::ReservedSlotOccupied;
            return false;
        }

        return true;
    }

    bool HasPreferredPlacementSlotCandidatesValue = false;
    if (EconomyOrderValue.PreferredPlacementSlotType != EBuildPlacementSlotType::Unknown)
    {
        for (const FBuildPlacementSlot& BuildPlacementSlotValue : BuildPlacementSlotsValue)
        {
            if (BuildPlacementSlotValue.SlotId.SlotType != EconomyOrderValue.PreferredPlacementSlotType)
            {
                continue;
            }

            HasPreferredPlacementSlotCandidatesValue = true;
            if (TrySelectPlacementSlotCandidate(FrameValue, CommandAuthoritySchedulingStateValue,
                                                EconomyOrderValue, WorkerUnitValue, BuildPlacementSlotValue,
                                                OutBuildPlacementSlotValue))
            {
                return true;
            }
        }

        if (HasPreferredPlacementSlotCandidatesValue)
        {
            OutDeferralReasonValue = ECommandOrderDeferralReason::ReservedSlotOccupied;
            return false;
        }
    }

    for (const FBuildPlacementSlot& BuildPlacementSlotValue : BuildPlacementSlotsValue)
    {
        if (TrySelectPlacementSlotCandidate(FrameValue, CommandAuthoritySchedulingStateValue,
                                            EconomyOrderValue, WorkerUnitValue, BuildPlacementSlotValue,
                                            OutBuildPlacementSlotValue))
        {
            return true;
        }
    }

    OutDeferralReasonValue = ECommandOrderDeferralReason::NoValidPlacement;
    return false;
}

enum class EReservedPlacementSlotState : uint8_t
{
    Active,
    Invalidated,
    Occupied,
};

EReservedPlacementSlotState GetAwaitingReservedPlacementSlotState(
    const FFrameContext& FrameValue, const FGameStateDescriptor& GameStateDescriptorValue,
    const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
    const IBuildPlacementService& BuildPlacementServiceValue, const std::vector<Point2D>& ExpansionLocationsValue,
    const FCommandOrderRecord& EconomyOrderValue, FBuildPlacementSlot& OutReservedBuildPlacementSlotValue)
{
    if (FrameValue.Observation == nullptr || FrameValue.GameInfo == nullptr || FrameValue.Query == nullptr ||
        !EconomyOrderValue.ReservedPlacementSlotId.IsValid())
    {
        return EReservedPlacementSlotState::Active;
    }

    const Point2D BaseLocationValue = Point2D(FrameValue.Observation->GetStartLocation());
    const FBuildPlacementContext BuildPlacementContextValue = CreateBuildPlacementContext(
        BaseLocationValue, ExpansionLocationsValue, GameStateDescriptorValue.RampWallDescriptor,
        GameStateDescriptorValue.MainBaseLayoutDescriptor);
    const std::vector<FBuildPlacementSlot> BuildPlacementSlotsValue =
        BuildPlacementServiceValue.GetStructurePlacementSlots(GameStateDescriptorValue, EconomyOrderValue.AbilityId,
                                                              BuildPlacementContextValue);
    if (!TryFindPlacementSlotById(BuildPlacementSlotsValue, EconomyOrderValue.ReservedPlacementSlotId,
                                  OutReservedBuildPlacementSlotValue))
    {
        return EReservedPlacementSlotState::Invalidated;
    }

    const Point2D ClampedBuildPointValue =
        ClampToPlayable(*FrameValue.GameInfo, OutReservedBuildPlacementSlotValue.BuildPoint);
    OutReservedBuildPlacementSlotValue.BuildPoint = ClampedBuildPointValue;

    if (FindObservedStructureOccupyingPlacementSlot(*FrameValue.Observation, EconomyOrderValue.AbilityId,
                                                    OutReservedBuildPlacementSlotValue) != nullptr)
    {
        return EReservedPlacementSlotState::Active;
    }

    if (HasObservedBuildOrderTargetingPlacementSlot(*FrameValue.Observation, EconomyOrderValue.AbilityId,
                                                    OutReservedBuildPlacementSlotValue))
    {
        constexpr uint64_t BuildStartGraceGameLoopsValue = 192U;
        uint64_t LatestChildDispatchGameLoopValue = 0U;
        if (!TryGetLatestUnitExecutionChildDispatchGameLoop(CommandAuthoritySchedulingStateValue,
                                                            EconomyOrderValue.OrderId,
                                                            LatestChildDispatchGameLoopValue) ||
            GameStateDescriptorValue.CurrentGameLoop <
                (LatestChildDispatchGameLoopValue + BuildStartGraceGameLoopsValue))
        {
            return EReservedPlacementSlotState::Active;
        }
    }

    return FrameValue.Query->Placement(EconomyOrderValue.AbilityId, ClampedBuildPointValue)
               ? EReservedPlacementSlotState::Invalidated
               : EReservedPlacementSlotState::Occupied;
}

bool DoesStructureAbilityRequireAddonClearance(const ABILITY_ID StructureAbilityIdValue)
{
    switch (StructureAbilityIdValue)
    {
        case ABILITY_ID::BUILD_BARRACKS:
        case ABILITY_ID::BUILD_FACTORY:
        case ABILITY_ID::BUILD_STARPORT:
            return true;
        default:
            return false;
    }
}

Point2D GetAddonFootprintCenter(const Point2D& StructureBuildPointValue)
{
    return Point2D(StructureBuildPointValue.x + 2.5f, StructureBuildPointValue.y - 0.5f);
}

bool DoesAddonFootprintSupportTerrain(const GameInfo& GameInfoValue, const Point2D& StructureBuildPointValue)
{
    const PlacementGrid PlacementGridValue(GameInfoValue);
    const Point2D AddonCenterValue = GetAddonFootprintCenter(StructureBuildPointValue);

    static const std::array<Point2D, 5> AddonSampleOffsetsValue =
    {{
        Point2D(0.0f, 0.0f),
        Point2D(-0.5f, -0.5f),
        Point2D(-0.5f, 0.5f),
        Point2D(0.5f, -0.5f),
        Point2D(0.5f, 0.5f),
    }};

    for (const Point2D& AddonSampleOffsetValue : AddonSampleOffsetsValue)
    {
        const Point2D SamplePointValue(AddonCenterValue.x + AddonSampleOffsetValue.x,
                                       AddonCenterValue.y + AddonSampleOffsetValue.y);
        if (!PlacementGridValue.IsPlacable(SamplePointValue))
        {
            return false;
        }
    }

    return true;
}

bool DoesAddonFootprintAvoidObservedStructures(const ObservationInterface& ObservationValue,
                                               const Point2D& StructureBuildPointValue)
{
    const Point2D AddonCenterValue = GetAddonFootprintCenter(StructureBuildPointValue);
    const Point2D AddonFootprintHalfExtentsValue(1.0f, 1.0f);
    const Units SelfUnitsValue = ObservationValue.GetUnits(Unit::Alliance::Self);

    for (const Unit* SelfUnitValue : SelfUnitsValue)
    {
        if (SelfUnitValue == nullptr || !IsTerranBuilding(SelfUnitValue->unit_type.ToType()) ||
            SelfUnitValue->is_flying)
        {
            continue;
        }

        const Point2D ObservedStructureHalfExtentsValue =
            GetStructureFootprintHalfExtentsForUnitType(SelfUnitValue->unit_type.ToType());
        if (DoAxisAlignedFootprintsOverlap(Point2D(SelfUnitValue->pos), ObservedStructureHalfExtentsValue,
                                           AddonCenterValue, AddonFootprintHalfExtentsValue))
        {
            return false;
        }
    }

    return true;
}

bool DoesPlacementSlotSatisfyFootprintPolicy(const FFrameContext& FrameValue,
                                             const FBuildPlacementSlot& BuildPlacementSlotValue,
                                             const ABILITY_ID StructureAbilityIdValue)
{
    switch (BuildPlacementSlotValue.FootprintPolicy)
    {
        case EBuildPlacementFootprintPolicy::StructureOnly:
            return true;
        case EBuildPlacementFootprintPolicy::RequiresAddonClearance:
            if (FrameValue.Observation == nullptr || FrameValue.GameInfo == nullptr ||
                !DoesStructureAbilityRequireAddonClearance(StructureAbilityIdValue))
            {
                return false;
            }

            return DoesAddonFootprintSupportTerrain(*FrameValue.GameInfo, BuildPlacementSlotValue.BuildPoint) &&
                   DoesAddonFootprintAvoidObservedStructures(*FrameValue.Observation,
                                                             BuildPlacementSlotValue.BuildPoint);
        default:
            return false;
    }
}

bool CanReserveResources(const FBuildPlanningState& BuildPlanningStateValue, const uint32_t MineralsCostValue,
                         const uint32_t VespeneCostValue, const uint32_t SupplyCostValue)
{
    return BuildPlanningStateValue.AvailableMinerals >=
               (BuildPlanningStateValue.ReservedMinerals + MineralsCostValue) &&
           BuildPlanningStateValue.AvailableVespene >= (BuildPlanningStateValue.ReservedVespene + VespeneCostValue) &&
           BuildPlanningStateValue.AvailableSupply >= (BuildPlanningStateValue.ReservedSupply + SupplyCostValue);
}

bool TryReserveResources(FBuildPlanningState& BuildPlanningStateValue, const uint32_t MineralsCostValue,
                         const uint32_t VespeneCostValue, const uint32_t SupplyCostValue)
{
    if (!CanReserveResources(BuildPlanningStateValue, MineralsCostValue, VespeneCostValue, SupplyCostValue))
    {
        return false;
    }

    BuildPlanningStateValue.ReservedMinerals += MineralsCostValue;
    BuildPlanningStateValue.ReservedVespene += VespeneCostValue;
    BuildPlanningStateValue.ReservedSupply += SupplyCostValue;
    return true;
}

void ReleaseReservedResources(FBuildPlanningState& BuildPlanningStateValue, const uint32_t MineralsCostValue,
                              const uint32_t VespeneCostValue, const uint32_t SupplyCostValue)
{
    BuildPlanningStateValue.ReservedMinerals -= MineralsCostValue;
    BuildPlanningStateValue.ReservedVespene -= VespeneCostValue;
    BuildPlanningStateValue.ReservedSupply -= SupplyCostValue;
}

bool TryReserveStructureCost(FBuildPlanningState& BuildPlanningStateValue, const UNIT_TYPEID StructureTypeIdValue)
{
    const FBuildingCostData& BuildingCostDataValue = TERRAN_ECONOMIC_DATA.GetBuildingCostData(StructureTypeIdValue);
    return TryReserveResources(BuildPlanningStateValue, BuildingCostDataValue.CostData.Minerals,
                               BuildingCostDataValue.CostData.Vespine, 0U);
}

bool TryReserveUnitCost(FBuildPlanningState& BuildPlanningStateValue, const UNIT_TYPEID UnitTypeIdValue)
{
    const FUnitCostData& UnitCostDataValue = TERRAN_ECONOMIC_DATA.GetUnitCostData(UnitTypeIdValue);
    return TryReserveResources(BuildPlanningStateValue, UnitCostDataValue.CostData.Minerals,
                               UnitCostDataValue.CostData.Vespine, UnitCostDataValue.CostData.Supply);
}

bool TryReserveUpgradeCost(FBuildPlanningState& BuildPlanningStateValue, const ABILITY_ID UpgradeAbilityIdValue)
{
    const FUpgradeCostData& UpgradeCostDataValue = TERRAN_ECONOMIC_DATA.GetUpgradeCostData(UpgradeAbilityIdValue);
    return TryReserveResources(BuildPlanningStateValue, UpgradeCostDataValue.CostData.Minerals,
                               UpgradeCostDataValue.CostData.Vespine, 0U);
}

bool HasReactorAddon(const ObservationInterface& ObservationValue, const Unit& ProductionUnitValue)
{
    if (ProductionUnitValue.add_on_tag == NullTag)
    {
        return false;
    }

    const Unit* AddonUnitValue = ObservationValue.GetUnit(ProductionUnitValue.add_on_tag);
    return AddonUnitValue != nullptr && IsReactorUnitType(AddonUnitValue->unit_type.ToType());
}

bool HasTechLabAddon(const ObservationInterface& ObservationValue, const Unit& ProductionUnitValue)
{
    if (ProductionUnitValue.add_on_tag == NullTag)
    {
        return false;
    }

    const Unit* AddonUnitValue = ObservationValue.GetUnit(ProductionUnitValue.add_on_tag);
    return AddonUnitValue != nullptr && IsTechLabUnitType(AddonUnitValue->unit_type.ToType());
}

int GetProductionQueueCapacity(const ObservationInterface& ObservationValue, const Unit& ProductionUnitValue)
{
    return HasReactorAddon(ObservationValue, ProductionUnitValue) ? 2 : 1;
}

int GetCommittedProductionOrderCount(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                     const Unit& ProductionUnitValue)
{
    return static_cast<int>(ProductionUnitValue.orders.size()) +
           static_cast<int>(CountActiveUnitProductionSchedulerOrdersForActor(CommandAuthoritySchedulingStateValue,
                                                                             ProductionUnitValue.tag));
}

const Unit* SelectBuildWorker(const FAgentState& AgentStateValue, const FIntentBuffer& IntentBufferValue,
                              const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                              const UNIT_TYPEID WorkerTypeIdValue, const Point2D& BuildAnchorValue)
{
    const Unit* IdleWorkerValue = nullptr;
    const Unit* GatheringWorkerValue = nullptr;
    const Unit* FallbackWorkerValue = nullptr;
    float GatheringDistanceValue = std::numeric_limits<float>::max();
    float FallbackDistanceValue = std::numeric_limits<float>::max();

    for (const Unit* WorkerValue : AgentStateValue.UnitContainer.ControlledUnits)
    {
        if (WorkerValue == nullptr || WorkerValue->unit_type.ToType() != WorkerTypeIdValue ||
            IntentBufferValue.HasIntentForActorInDomain(WorkerValue->tag, EIntentDomain::StructureBuild) ||
            HasActiveSchedulerOrderForActor(CommandAuthoritySchedulingStateValue, WorkerValue->tag))
        {
            continue;
        }

        if (WorkerValue->orders.empty())
        {
            IdleWorkerValue = WorkerValue;
            break;
        }

        const float DistanceValue = DistanceSquared2D(WorkerValue->pos, BuildAnchorValue);
        if (WorkerValue->orders.front().ability_id == ABILITY_ID::HARVEST_GATHER)
        {
            if (GatheringWorkerValue == nullptr || DistanceValue < GatheringDistanceValue)
            {
                GatheringWorkerValue = WorkerValue;
                GatheringDistanceValue = DistanceValue;
            }
            continue;
        }

        if (FallbackWorkerValue == nullptr || DistanceValue < FallbackDistanceValue)
        {
            FallbackWorkerValue = WorkerValue;
            FallbackDistanceValue = DistanceValue;
        }
    }

    if (IdleWorkerValue != nullptr)
    {
        return IdleWorkerValue;
    }
    if (GatheringWorkerValue != nullptr)
    {
        return GatheringWorkerValue;
    }
    return FallbackWorkerValue;
}

bool TryGetStructureBuildPoint(const FFrameContext& FrameValue, const FGameStateDescriptor& GameStateDescriptorValue,
                               const IBuildPlacementService& BuildPlacementServiceValue,
                               const std::vector<Point2D>& ExpansionLocationsValue,
                               const ABILITY_ID StructureAbilityIdValue, const Unit& WorkerUnitValue,
                               Point2D& OutBuildPointValue)
{
    if (FrameValue.Observation == nullptr || FrameValue.Query == nullptr)
    {
        return false;
    }

    const Point2D BaseLocationValue = Point2D(FrameValue.Observation->GetStartLocation());
    const FBuildPlacementContext BuildPlacementContextValue = CreateBuildPlacementContext(
        BaseLocationValue, ExpansionLocationsValue, GameStateDescriptorValue.RampWallDescriptor,
        GameStateDescriptorValue.MainBaseLayoutDescriptor);
    const std::vector<FBuildPlacementSlot> BuildPlacementSlotsValue =
        BuildPlacementServiceValue.GetStructurePlacementSlots(GameStateDescriptorValue, StructureAbilityIdValue,
                                                              BuildPlacementContextValue);
    const GameInfo& GameInfoValue = FrameValue.Observation->GetGameInfo();

    for (const FBuildPlacementSlot& BuildPlacementSlotValue : BuildPlacementSlotsValue)
    {
        const Point2D ClampedCandidateValue = ClampToPlayable(GameInfoValue, BuildPlacementSlotValue.BuildPoint);
        FBuildPlacementSlot NormalizedBuildPlacementSlotValue = BuildPlacementSlotValue;
        NormalizedBuildPlacementSlotValue.BuildPoint = ClampedCandidateValue;

        if (!DoesPlacementSlotSatisfyFootprintPolicy(FrameValue, NormalizedBuildPlacementSlotValue,
                                                     StructureAbilityIdValue))
        {
            continue;
        }

        if (FrameValue.Query->Placement(StructureAbilityIdValue, ClampedCandidateValue, &WorkerUnitValue))
        {
            OutBuildPointValue = ClampedCandidateValue;
            return true;
        }
    }

    return false;
}

Point2D GetNextExpansionLocation(const FFrameContext& FrameValue, const std::vector<Point2D>& ExpansionLocationsValue)
{
    if (FrameValue.Observation == nullptr || FrameValue.Query == nullptr)
    {
        return Point2D(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN());
    }

    const Units TownHallUnitsValue = FrameValue.Observation->GetUnits(Unit::Alliance::Self, IsTownHall());
    const Point2D StartLocationValue = Point2D(FrameValue.Observation->GetStartLocation());
    float BestDistanceValue = std::numeric_limits<float>::max();
    Point2D BestExpansionLocationValue(std::numeric_limits<float>::quiet_NaN(),
                                       std::numeric_limits<float>::quiet_NaN());

    for (const Point2D& ExpansionLocationValue : ExpansionLocationsValue)
    {
        if (Distance2D(StartLocationValue, ExpansionLocationValue) < 4.0f)
        {
            continue;
        }

        bool IsOccupiedValue = false;
        for (const Unit* TownHallUnitValue : TownHallUnitsValue)
        {
            if (TownHallUnitValue != nullptr &&
                Distance2D(Point2D(TownHallUnitValue->pos), ExpansionLocationValue) < 8.0f)
            {
                IsOccupiedValue = true;
                break;
            }
        }

        if (IsOccupiedValue || !FrameValue.Query->Placement(ABILITY_ID::BUILD_COMMANDCENTER, ExpansionLocationValue))
        {
            continue;
        }

        const float DistanceValue = DistanceSquared2D(StartLocationValue, ExpansionLocationValue);
        if (DistanceValue < BestDistanceValue)
        {
            BestDistanceValue = DistanceValue;
            BestExpansionLocationValue = ExpansionLocationValue;
        }
    }

    return BestExpansionLocationValue;
}

}  // namespace

void FTerranEconomyProductionOrderExpander::ExpandEconomyAndProductionOrders(
    const FFrameContext& FrameValue, const FAgentState& AgentStateValue, FGameStateDescriptor& GameStateDescriptorValue,
    FIntentBuffer& IntentBufferValue, const IBuildPlacementService& BuildPlacementServiceValue,
    const std::vector<Point2D>& ExpansionLocationsValue) const
{
    if (FrameValue.Observation == nullptr)
    {
        return;
    }

    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState;
    std::vector<size_t> PlanningOrderIndicesValue = CommandAuthoritySchedulingStateValue.PlanningProcessIndices;
    std::stable_sort(PlanningOrderIndicesValue.begin(), PlanningOrderIndicesValue.end(),
                     [&GameStateDescriptorValue, &CommandAuthoritySchedulingStateValue](
                         const size_t LeftOrderIndexValue, const size_t RightOrderIndexValue)
                     {
                         const FCommandOrderRecord LeftOrderValue =
                             CommandAuthoritySchedulingStateValue.GetOrderRecord(LeftOrderIndexValue);
                         const FCommandOrderRecord RightOrderValue =
                             CommandAuthoritySchedulingStateValue.GetOrderRecord(RightOrderIndexValue);
                         return GetEffectiveEconomyOrderPriority(LeftOrderValue, GameStateDescriptorValue.BuildPlanning,
                                                                 CommandAuthoritySchedulingStateValue) >
                                GetEffectiveEconomyOrderPriority(RightOrderValue, GameStateDescriptorValue.BuildPlanning,
                                                                 CommandAuthoritySchedulingStateValue);
                     });
    const uint64_t CurrentStepValue = GameStateDescriptorValue.CurrentStep;
    const uint64_t CurrentGameLoopValue = GameStateDescriptorValue.CurrentGameLoop;

    for (const size_t PlanningOrderIndexValue : PlanningOrderIndicesValue)
    {
        const FCommandOrderRecord EconomyOrderValue =
            CommandAuthoritySchedulingStateValue.GetOrderRecord(PlanningOrderIndexValue);
        if (EconomyOrderValue.SourceLayer != ECommandAuthorityLayer::EconomyAndProduction ||
            EconomyOrderValue.LifecycleState != EOrderLifecycleState::Queued)
        {
            continue;
        }

        if (DoesOrderTargetMatchObservedState(GameStateDescriptorValue, EconomyOrderValue))
        {
            CommandAuthoritySchedulingStateValue.SetOrderDeferralState(
                EconomyOrderValue.OrderId, ECommandOrderDeferralReason::TargetAlreadySatisfied, CurrentStepValue,
                CurrentGameLoopValue);
            CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(EconomyOrderValue.OrderId,
                                                                       EOrderLifecycleState::Completed);
            continue;
        }

        const bool IsUnitProductionOrderValue = IsUnitProductionAbility(EconomyOrderValue.AbilityId);
        const uint32_t ActiveUnitExecutionChildCountValue =
            CountActiveUnitExecutionChildrenForParent(CommandAuthoritySchedulingStateValue, EconomyOrderValue.OrderId);
        if ((!IsUnitProductionOrderValue &&
             HasActiveUnitExecutionChild(CommandAuthoritySchedulingStateValue, EconomyOrderValue.OrderId)) ||
            (IsUnitProductionOrderValue &&
             ActiveUnitExecutionChildCountValue >= std::max(EconomyOrderValue.RequestedQueueCount, 1U)))
        {
            CommandAuthoritySchedulingStateValue.SetOrderDeferralState(
                EconomyOrderValue.OrderId, ECommandOrderDeferralReason::AwaitingObservedCompletion, CurrentStepValue,
                CurrentGameLoopValue);
            continue;
        }

        if (ShouldUseReservedPlacementSlotAwaitingState(EconomyOrderValue))
        {
            FBuildPlacementSlot ReservedBuildPlacementSlotValue;
            const EReservedPlacementSlotState ReservedPlacementSlotStateValue =
                GetAwaitingReservedPlacementSlotState(FrameValue, GameStateDescriptorValue,
                                                      CommandAuthoritySchedulingStateValue,
                                                      BuildPlacementServiceValue, ExpansionLocationsValue,
                                                      EconomyOrderValue, ReservedBuildPlacementSlotValue);
            switch (ReservedPlacementSlotStateValue)
            {
                case EReservedPlacementSlotState::Invalidated:
                    CommandAuthoritySchedulingStateValue.ClearOrderReservedPlacementSlot(EconomyOrderValue.OrderId);
                    CommandAuthoritySchedulingStateValue.SetOrderDeferralState(
                        EconomyOrderValue.OrderId, ECommandOrderDeferralReason::ReservedSlotInvalidated,
                        CurrentStepValue, CurrentGameLoopValue);
                    break;
                case EReservedPlacementSlotState::Occupied:
                    CommandAuthoritySchedulingStateValue.SetOrderDeferralState(
                        EconomyOrderValue.OrderId, ECommandOrderDeferralReason::ReservedSlotOccupied,
                        CurrentStepValue, CurrentGameLoopValue);
                    continue;
                case EReservedPlacementSlotState::Active:
                    CommandAuthoritySchedulingStateValue.SetOrderDeferralState(
                        EconomyOrderValue.OrderId, ECommandOrderDeferralReason::AwaitingObservedCompletion,
                        CurrentStepValue, CurrentGameLoopValue);
                    continue;
                default:
                    break;
            }
        }

        const uint32_t ObservedCountValue =
            GetObservedCountForOrder(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue);
        const uint32_t CurrentOrderCountValue = IsStructureBuildAbility(EconomyOrderValue.AbilityId)
                                                    ? 0U
                                                    : CountCurrentOrdersForAbility(AgentStateValue,
                                                                                  EconomyOrderValue.AbilityId);
        const uint32_t PendingOrderCountValue =
            CurrentOrderCountValue +
            CountPendingSchedulerOrdersForAbility(CommandAuthoritySchedulingStateValue, EconomyOrderValue.AbilityId) +
            CountPendingIntentsForAbility(IntentBufferValue, EconomyOrderValue.AbilityId);
        const uint32_t ProjectedCountValue =
            ObservedCountValue +
            GetObservedInConstructionCountForOrder(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue) +
            PendingOrderCountValue;
        if (ProjectedCountValue >= EconomyOrderValue.TargetCount)
        {
            CommandAuthoritySchedulingStateValue.SetOrderDeferralState(
                EconomyOrderValue.OrderId, ECommandOrderDeferralReason::AwaitingObservedCompletion, CurrentStepValue,
                CurrentGameLoopValue);
            continue;
        }

        FCommandOrderRecord UnitExecutionOrderValue;
        bool CreatedOrderValue = false;
        ECommandOrderDeferralReason DeferralReasonValue = ECommandOrderDeferralReason::None;

        switch (EconomyOrderValue.AbilityId.ToType())
        {
            case ABILITY_ID::BUILD_SUPPLYDEPOT:
            case ABILITY_ID::BUILD_BARRACKS:
            case ABILITY_ID::BUILD_FACTORY:
            case ABILITY_ID::BUILD_STARPORT:
            case ABILITY_ID::BUILD_BUNKER:
            case ABILITY_ID::BUILD_ENGINEERINGBAY:
            {
                const Point2D BaseLocationValue = Point2D(FrameValue.Observation->GetStartLocation());
                const FBuildPlacementContext BuildPlacementContextValue = CreateBuildPlacementContext(
                    BaseLocationValue, ExpansionLocationsValue, GameStateDescriptorValue.RampWallDescriptor,
                    GameStateDescriptorValue.MainBaseLayoutDescriptor);
                const Point2D BuildAnchorValue = BuildPlacementServiceValue.GetPrimaryStructureAnchor(
                    GameStateDescriptorValue, BuildPlacementContextValue);
                const Unit* WorkerUnitValue = SelectBuildWorker(AgentStateValue, IntentBufferValue,
                                                                CommandAuthoritySchedulingStateValue,
                                                                UNIT_TYPEID::TERRAN_SCV, BuildAnchorValue);
                if (WorkerUnitValue == nullptr)
                {
                    DeferralReasonValue = GetWorkerAvailabilityDeferralReason(AgentStateValue);
                    break;
                }

                FBuildPlacementSlot SelectedBuildPlacementSlotValue;
                if (!TrySelectStructurePlacementSlot(FrameValue, GameStateDescriptorValue, BuildPlacementServiceValue,
                                                     ExpansionLocationsValue,
                                                     CommandAuthoritySchedulingStateValue, EconomyOrderValue,
                                                     *WorkerUnitValue, SelectedBuildPlacementSlotValue,
                                                     DeferralReasonValue))
                {
                    break;
                }

                if (!TryReserveStructureCost(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue.ResultUnitTypeId))
                {
                    DeferralReasonValue = ECommandOrderDeferralReason::InsufficientResources;
                    break;
                }

                UnitExecutionOrderValue = FCommandOrderRecord::CreatePointTarget(
                    ECommandAuthorityLayer::UnitExecution, WorkerUnitValue->tag, EconomyOrderValue.AbilityId,
                    SelectedBuildPlacementSlotValue.BuildPoint, EconomyOrderValue.BasePriorityValue,
                    EIntentDomain::StructureBuild,
                    GameStateDescriptorValue.CurrentGameLoop, 0U, EconomyOrderValue.OrderId, -1, -1, false, true,
                    false);
                CopyTaskMetadataToChildOrder(EconomyOrderValue, UnitExecutionOrderValue);
                UnitExecutionOrderValue.PreferredPlacementSlotType = EconomyOrderValue.PreferredPlacementSlotType;
                UnitExecutionOrderValue.PreferredPlacementSlotId = EconomyOrderValue.PreferredPlacementSlotId;
                CommandAuthoritySchedulingStateValue.SetOrderReservedPlacementSlot(EconomyOrderValue.OrderId,
                                                                                   SelectedBuildPlacementSlotValue.SlotId);
                UnitExecutionOrderValue.ReservedPlacementSlotId = SelectedBuildPlacementSlotValue.SlotId;
                CreatedOrderValue = true;
                break;
            }
            case ABILITY_ID::BUILD_REFINERY:
            {
                if (FrameValue.Query == nullptr)
                {
                    DeferralReasonValue = ECommandOrderDeferralReason::NoValidPlacement;
                    break;
                }

                if (!TryReserveStructureCost(GameStateDescriptorValue.BuildPlanning, UNIT_TYPEID::TERRAN_REFINERY))
                {
                    DeferralReasonValue = ECommandOrderDeferralReason::InsufficientResources;
                    break;
                }

                const Units TownHallUnitsValue = FrameValue.Observation->GetUnits(Unit::Alliance::Self, IsTownHall());
                const Units GeyserUnitsValue = FrameValue.Observation->GetUnits(Unit::Alliance::Neutral, IsGeyser());
                const Point2D StartLocationValue = Point2D(FrameValue.Observation->GetStartLocation());
                Units SortedTownHallUnitsValue;
                SortedTownHallUnitsValue.reserve(TownHallUnitsValue.size());
                bool HasCompletedTownHallValue = false;
                bool HasValidGeyserPlacementValue = false;
                bool HasWorkerAvailabilityFailureValue = false;

                for (const Unit* TownHallUnitValue : TownHallUnitsValue)
                {
                    if (TownHallUnitValue == nullptr || TownHallUnitValue->build_progress < 1.0f)
                    {
                        continue;
                    }

                    HasCompletedTownHallValue = true;
                    SortedTownHallUnitsValue.push_back(TownHallUnitValue);
                }

                std::stable_sort(SortedTownHallUnitsValue.begin(), SortedTownHallUnitsValue.end(),
                                 [&StartLocationValue](const Unit* LeftTownHallUnitValue,
                                                       const Unit* RightTownHallUnitValue)
                                 {
                                     return DistanceSquared2D(Point2D(LeftTownHallUnitValue->pos),
                                                              StartLocationValue) <
                                            DistanceSquared2D(Point2D(RightTownHallUnitValue->pos),
                                                              StartLocationValue);
                                 });

                for (const Unit* TownHallUnitValue : SortedTownHallUnitsValue)
                {
                    Units NearbyGeyserUnitsValue;
                    NearbyGeyserUnitsValue.reserve(GeyserUnitsValue.size());

                    for (const Unit* GeyserUnitValue : GeyserUnitsValue)
                    {
                        if (GeyserUnitValue == nullptr || Distance2D(TownHallUnitValue->pos, GeyserUnitValue->pos) > 15.0f)
                        {
                            continue;
                        }

                        NearbyGeyserUnitsValue.push_back(GeyserUnitValue);
                    }

                    std::stable_sort(NearbyGeyserUnitsValue.begin(), NearbyGeyserUnitsValue.end(),
                                     [TownHallUnitValue](const Unit* LeftGeyserUnitValue,
                                                         const Unit* RightGeyserUnitValue)
                                     {
                                         return DistanceSquared2D(Point2D(LeftGeyserUnitValue->pos),
                                                                  Point2D(TownHallUnitValue->pos)) <
                                                DistanceSquared2D(Point2D(RightGeyserUnitValue->pos),
                                                                  Point2D(TownHallUnitValue->pos));
                                     });

                    for (const Unit* GeyserUnitValue : NearbyGeyserUnitsValue)
                    {

                        if (!FrameValue.Query->Placement(ABILITY_ID::BUILD_REFINERY, GeyserUnitValue->pos))
                        {
                            continue;
                        }

                        HasValidGeyserPlacementValue = true;
                        const Unit* WorkerUnitValue = SelectBuildWorker(AgentStateValue, IntentBufferValue,
                                                                        CommandAuthoritySchedulingStateValue,
                                                                        UNIT_TYPEID::TERRAN_SCV, GeyserUnitValue->pos);
                        if (WorkerUnitValue == nullptr)
                        {
                            HasWorkerAvailabilityFailureValue = true;
                            continue;
                        }

                        UnitExecutionOrderValue = FCommandOrderRecord::CreateUnitTarget(
                            ECommandAuthorityLayer::UnitExecution, WorkerUnitValue->tag, EconomyOrderValue.AbilityId,
                            GeyserUnitValue->tag, EconomyOrderValue.BasePriorityValue, EIntentDomain::StructureBuild,
                            GameStateDescriptorValue.CurrentGameLoop, 0U, EconomyOrderValue.OrderId);
                        CopyTaskMetadataToChildOrder(EconomyOrderValue, UnitExecutionOrderValue);
                        CreatedOrderValue = true;
                        break;
                    }

                    if (CreatedOrderValue)
                    {
                        break;
                    }
                }

                if (!CreatedOrderValue)
                {
                    const FBuildingCostData& RefineryCostDataValue =
                        TERRAN_ECONOMIC_DATA.GetBuildingCostData(UNIT_TYPEID::TERRAN_REFINERY);
                    ReleaseReservedResources(GameStateDescriptorValue.BuildPlanning, RefineryCostDataValue.CostData.Minerals,
                                             RefineryCostDataValue.CostData.Vespine, 0U);
                    if (HasWorkerAvailabilityFailureValue)
                    {
                        DeferralReasonValue = GetWorkerAvailabilityDeferralReason(AgentStateValue);
                    }
                    else if (HasValidGeyserPlacementValue)
                    {
                        DeferralReasonValue = ECommandOrderDeferralReason::NoProducer;
                    }
                    else if (HasCompletedTownHallValue)
                    {
                        DeferralReasonValue = ECommandOrderDeferralReason::NoValidPlacement;
                    }
                    else
                    {
                        DeferralReasonValue = ECommandOrderDeferralReason::NoProducer;
                    }
                }
                break;
            }
            case ABILITY_ID::BUILD_COMMANDCENTER:
            {
                const Point2D ExpansionLocationValue = GetNextExpansionLocation(FrameValue, ExpansionLocationsValue);
                if (!IsPointFinite(ExpansionLocationValue))
                {
                    DeferralReasonValue = ECommandOrderDeferralReason::NoValidPlacement;
                    break;
                }

                const Unit* WorkerUnitValue = SelectBuildWorker(AgentStateValue, IntentBufferValue,
                                                                CommandAuthoritySchedulingStateValue,
                                                                UNIT_TYPEID::TERRAN_SCV, ExpansionLocationValue);
                if (WorkerUnitValue == nullptr)
                {
                    DeferralReasonValue = GetWorkerAvailabilityDeferralReason(AgentStateValue);
                    break;
                }

                if (!TryReserveStructureCost(GameStateDescriptorValue.BuildPlanning, UNIT_TYPEID::TERRAN_COMMANDCENTER))
                {
                    DeferralReasonValue = ECommandOrderDeferralReason::InsufficientResources;
                    break;
                }

                UnitExecutionOrderValue = FCommandOrderRecord::CreatePointTarget(
                    ECommandAuthorityLayer::UnitExecution, WorkerUnitValue->tag, EconomyOrderValue.AbilityId,
                    ExpansionLocationValue, EconomyOrderValue.BasePriorityValue, EIntentDomain::StructureBuild,
                    GameStateDescriptorValue.CurrentGameLoop, 0U, EconomyOrderValue.OrderId, -1, -1, false, true,
                    false);
                CopyTaskMetadataToChildOrder(EconomyOrderValue, UnitExecutionOrderValue);
                CreatedOrderValue = true;
                break;
            }
            case ABILITY_ID::MORPH_ORBITALCOMMAND:
            {
                if (!TryReserveResources(GameStateDescriptorValue.BuildPlanning, 150U, 0U, 0U))
                {
                    DeferralReasonValue = ECommandOrderDeferralReason::InsufficientResources;
                    break;
                }

                const Units TownHallUnitsValue = FrameValue.Observation->GetUnits(Unit::Alliance::Self, IsTownHall());
                const Unit* TownHallUnitValue = nullptr;
                bool HasBusyCommandCenterValue = false;
                for (const Unit* CandidateTownHallUnitValue : TownHallUnitsValue)
                {
                    if (CandidateTownHallUnitValue == nullptr ||
                        CandidateTownHallUnitValue->unit_type.ToType() != UNIT_TYPEID::TERRAN_COMMANDCENTER)
                    {
                        continue;
                    }

                    if (CandidateTownHallUnitValue->build_progress < 1.0f || !CandidateTownHallUnitValue->orders.empty() ||
                        IntentBufferValue.HasIntentForActor(CandidateTownHallUnitValue->tag) ||
                        HasActiveSchedulerOrderForActor(CommandAuthoritySchedulingStateValue,
                                                        CandidateTownHallUnitValue->tag))
                    {
                        HasBusyCommandCenterValue = true;
                        continue;
                    }

                    TownHallUnitValue = CandidateTownHallUnitValue;
                    break;
                }

                if (TownHallUnitValue == nullptr)
                {
                    ReleaseReservedResources(GameStateDescriptorValue.BuildPlanning, 150U, 0U, 0U);
                    DeferralReasonValue = HasBusyCommandCenterValue ? ECommandOrderDeferralReason::ProducerBusy
                                                                    : ECommandOrderDeferralReason::NoProducer;
                    break;
                }

                UnitExecutionOrderValue = FCommandOrderRecord::CreateNoTarget(
                    ECommandAuthorityLayer::UnitExecution, TownHallUnitValue->tag, EconomyOrderValue.AbilityId,
                    EconomyOrderValue.BasePriorityValue, EIntentDomain::StructureBuild,
                    GameStateDescriptorValue.CurrentGameLoop, 0U, EconomyOrderValue.OrderId);
                CopyTaskMetadataToChildOrder(EconomyOrderValue, UnitExecutionOrderValue);
                CreatedOrderValue = true;
                break;
            }
            case ABILITY_ID::BUILD_REACTOR_BARRACKS:
            case ABILITY_ID::BUILD_TECHLAB_BARRACKS:
            {
                const uint32_t MineralsCostValue =
                    EconomyOrderValue.AbilityId == ABILITY_ID::BUILD_REACTOR_BARRACKS ? 50U : 50U;
                const uint32_t VespeneCostValue =
                    EconomyOrderValue.AbilityId == ABILITY_ID::BUILD_REACTOR_BARRACKS ? 50U : 25U;
                if (!TryReserveResources(GameStateDescriptorValue.BuildPlanning, MineralsCostValue, VespeneCostValue,
                                         0U))
                {
                    DeferralReasonValue = ECommandOrderDeferralReason::InsufficientResources;
                    break;
                }

                const Units BarracksUnitsValue =
                    FrameValue.Observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_BARRACKS));
                const Unit* BarracksUnitValue = nullptr;
                bool HasBusyBarracksWithoutAddonValue = false;
                for (const Unit* CandidateBarracksUnitValue : BarracksUnitsValue)
                {
                    if (CandidateBarracksUnitValue == nullptr || CandidateBarracksUnitValue->add_on_tag != NullTag)
                    {
                        continue;
                    }

                    if (CandidateBarracksUnitValue->build_progress < 1.0f ||
                        !CandidateBarracksUnitValue->orders.empty() ||
                        IntentBufferValue.HasIntentForActor(CandidateBarracksUnitValue->tag) ||
                        HasActiveSchedulerOrderForActor(CommandAuthoritySchedulingStateValue,
                                                        CandidateBarracksUnitValue->tag))
                    {
                        HasBusyBarracksWithoutAddonValue = true;
                        continue;
                    }

                    BarracksUnitValue = CandidateBarracksUnitValue;
                    break;
                }

                if (BarracksUnitValue == nullptr)
                {
                    ReleaseReservedResources(GameStateDescriptorValue.BuildPlanning, MineralsCostValue,
                                             VespeneCostValue, 0U);
                    DeferralReasonValue = HasBusyBarracksWithoutAddonValue ? ECommandOrderDeferralReason::ProducerBusy
                                                                           : ECommandOrderDeferralReason::NoProducer;
                    break;
                }

                UnitExecutionOrderValue = FCommandOrderRecord::CreateNoTarget(
                    ECommandAuthorityLayer::UnitExecution, BarracksUnitValue->tag, EconomyOrderValue.AbilityId,
                    EconomyOrderValue.BasePriorityValue, EIntentDomain::StructureBuild,
                    GameStateDescriptorValue.CurrentGameLoop, 0U, EconomyOrderValue.OrderId);
                CopyTaskMetadataToChildOrder(EconomyOrderValue, UnitExecutionOrderValue);
                CreatedOrderValue = true;
                break;
            }
            case ABILITY_ID::BUILD_REACTOR_FACTORY:
            case ABILITY_ID::BUILD_TECHLAB_FACTORY:
            {
                const uint32_t MineralsCostValue =
                    EconomyOrderValue.AbilityId == ABILITY_ID::BUILD_REACTOR_FACTORY ? 50U : 50U;
                const uint32_t VespeneCostValue =
                    EconomyOrderValue.AbilityId == ABILITY_ID::BUILD_REACTOR_FACTORY ? 50U : 25U;
                if (!TryReserveResources(GameStateDescriptorValue.BuildPlanning, MineralsCostValue, VespeneCostValue,
                                         0U))
                {
                    DeferralReasonValue = ECommandOrderDeferralReason::InsufficientResources;
                    break;
                }

                const Units FactoryUnitsValue =
                    FrameValue.Observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_FACTORY));
                const Unit* FactoryUnitValue = nullptr;
                bool HasBusyFactoryWithoutAddonValue = false;
                for (const Unit* CandidateFactoryUnitValue : FactoryUnitsValue)
                {
                    if (CandidateFactoryUnitValue == nullptr || CandidateFactoryUnitValue->add_on_tag != NullTag)
                    {
                        continue;
                    }

                    if (CandidateFactoryUnitValue->build_progress < 1.0f || !CandidateFactoryUnitValue->orders.empty() ||
                        IntentBufferValue.HasIntentForActor(CandidateFactoryUnitValue->tag) ||
                        HasActiveSchedulerOrderForActor(CommandAuthoritySchedulingStateValue,
                                                        CandidateFactoryUnitValue->tag))
                    {
                        HasBusyFactoryWithoutAddonValue = true;
                        continue;
                    }

                    FactoryUnitValue = CandidateFactoryUnitValue;
                    break;
                }

                if (FactoryUnitValue == nullptr)
                {
                    ReleaseReservedResources(GameStateDescriptorValue.BuildPlanning, MineralsCostValue,
                                             VespeneCostValue, 0U);
                    DeferralReasonValue = HasBusyFactoryWithoutAddonValue ? ECommandOrderDeferralReason::ProducerBusy
                                                                          : ECommandOrderDeferralReason::NoProducer;
                    break;
                }

                UnitExecutionOrderValue = FCommandOrderRecord::CreateNoTarget(
                    ECommandAuthorityLayer::UnitExecution, FactoryUnitValue->tag, EconomyOrderValue.AbilityId,
                    EconomyOrderValue.BasePriorityValue, EIntentDomain::StructureBuild,
                    GameStateDescriptorValue.CurrentGameLoop, 0U, EconomyOrderValue.OrderId);
                CopyTaskMetadataToChildOrder(EconomyOrderValue, UnitExecutionOrderValue);
                CreatedOrderValue = true;
                break;
            }
            case ABILITY_ID::BUILD_REACTOR_STARPORT:
            {
                if (!TryReserveResources(GameStateDescriptorValue.BuildPlanning, 50U, 50U, 0U))
                {
                    DeferralReasonValue = ECommandOrderDeferralReason::InsufficientResources;
                    break;
                }

                const Units StarportUnitsValue =
                    FrameValue.Observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_STARPORT));
                const Unit* StarportUnitValue = nullptr;
                bool HasBusyStarportWithoutAddonValue = false;
                for (const Unit* CandidateStarportUnitValue : StarportUnitsValue)
                {
                    if (CandidateStarportUnitValue == nullptr || CandidateStarportUnitValue->add_on_tag != NullTag)
                    {
                        continue;
                    }

                    if (CandidateStarportUnitValue->build_progress < 1.0f ||
                        !CandidateStarportUnitValue->orders.empty() ||
                        IntentBufferValue.HasIntentForActor(CandidateStarportUnitValue->tag) ||
                        HasActiveSchedulerOrderForActor(CommandAuthoritySchedulingStateValue,
                                                        CandidateStarportUnitValue->tag))
                    {
                        HasBusyStarportWithoutAddonValue = true;
                        continue;
                    }

                    StarportUnitValue = CandidateStarportUnitValue;
                    break;
                }

                if (StarportUnitValue == nullptr)
                {
                    ReleaseReservedResources(GameStateDescriptorValue.BuildPlanning, 50U, 50U, 0U);
                    DeferralReasonValue = HasBusyStarportWithoutAddonValue ? ECommandOrderDeferralReason::ProducerBusy
                                                                           : ECommandOrderDeferralReason::NoProducer;
                    break;
                }

                UnitExecutionOrderValue = FCommandOrderRecord::CreateNoTarget(
                    ECommandAuthorityLayer::UnitExecution, StarportUnitValue->tag, EconomyOrderValue.AbilityId,
                    EconomyOrderValue.BasePriorityValue, EIntentDomain::StructureBuild,
                    GameStateDescriptorValue.CurrentGameLoop, 0U, EconomyOrderValue.OrderId);
                CopyTaskMetadataToChildOrder(EconomyOrderValue, UnitExecutionOrderValue);
                CreatedOrderValue = true;
                break;
            }
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
            {
                if (EconomyOrderValue.AbilityId == ABILITY_ID::TRAIN_SCV &&
                    GameStateDescriptorValue.BuildPlanning.AvailableSupply <= 1U &&
                    HasOutstandingSupplyDepotDemand(CommandAuthoritySchedulingStateValue,
                                                    GameStateDescriptorValue.BuildPlanning) &&
                    GameStateDescriptorValue.BuildPlanning.AvailableMinerals < 150U)
                {
                    DeferralReasonValue = ECommandOrderDeferralReason::InsufficientResources;
                    break;
                }

                const Units ProducerUnitsValue =
                    FrameValue.Observation->GetUnits(Unit::Alliance::Self, IsUnit(EconomyOrderValue.ProducerUnitTypeId));
                bool HasBusyProducerValue = false;
                for (const Unit* ProducerUnitValue : ProducerUnitsValue)
                {
                    if (ProducerUnitValue == nullptr)
                    {
                        continue;
                    }

                    if (ProducerUnitValue->build_progress < 1.0f ||
                        IntentBufferValue.HasIntentForActor(ProducerUnitValue->tag) ||
                        HasConflictingSchedulerOrderForProductionActor(CommandAuthoritySchedulingStateValue,
                                                                       ProducerUnitValue->tag))
                    {
                        HasBusyProducerValue = true;
                        continue;
                    }

                    if ((EconomyOrderValue.AbilityId == ABILITY_ID::TRAIN_MARAUDER ||
                         EconomyOrderValue.AbilityId == ABILITY_ID::TRAIN_CYCLONE ||
                         EconomyOrderValue.AbilityId == ABILITY_ID::TRAIN_SIEGETANK) &&
                        !HasTechLabAddon(*FrameValue.Observation, *ProducerUnitValue))
                    {
                        continue;
                    }

                    if (GetCommittedProductionOrderCount(CommandAuthoritySchedulingStateValue, *ProducerUnitValue) >=
                        GetProductionQueueCapacity(*FrameValue.Observation, *ProducerUnitValue))
                    {
                        HasBusyProducerValue = true;
                        continue;
                    }

                    const bool IsResearchUpgradeValue = EconomyOrderValue.UpgradeId.ToType() != UPGRADE_ID::INVALID;
                    const bool ReservedCostValue =
                        IsResearchUpgradeValue
                            ? TryReserveUpgradeCost(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue.AbilityId)
                            : TryReserveUnitCost(GameStateDescriptorValue.BuildPlanning,
                                                 EconomyOrderValue.ResultUnitTypeId);
                    if (!ReservedCostValue)
                    {
                        DeferralReasonValue = ECommandOrderDeferralReason::InsufficientResources;
                        break;
                    }

                    UnitExecutionOrderValue = FCommandOrderRecord::CreateNoTarget(
                        ECommandAuthorityLayer::UnitExecution, ProducerUnitValue->tag, EconomyOrderValue.AbilityId,
                        EconomyOrderValue.BasePriorityValue, EIntentDomain::UnitProduction,
                        GameStateDescriptorValue.CurrentGameLoop, 0U, EconomyOrderValue.OrderId);
                    CopyTaskMetadataToChildOrder(EconomyOrderValue, UnitExecutionOrderValue);
                    UnitExecutionOrderValue.Queued = !ProducerUnitValue->orders.empty();
                    CreatedOrderValue = true;
                    break;
                }
                if (!CreatedOrderValue && DeferralReasonValue == ECommandOrderDeferralReason::None)
                {
                    DeferralReasonValue = HasBusyProducerValue ? ECommandOrderDeferralReason::ProducerBusy
                                                               : ECommandOrderDeferralReason::NoProducer;
                }
                break;
            }
            default:
                DeferralReasonValue = ECommandOrderDeferralReason::NoProducer;
                break;
        }

        if (!CreatedOrderValue)
        {
            if (DeferralReasonValue != ECommandOrderDeferralReason::None)
            {
                CommandAuthoritySchedulingStateValue.SetOrderDeferralState(EconomyOrderValue.OrderId,
                                                                           DeferralReasonValue, CurrentStepValue,
                                                                           CurrentGameLoopValue);
            }
            continue;
        }

        UnitExecutionOrderValue.LifecycleState = EOrderLifecycleState::Ready;
        UnitExecutionOrderValue.PlanStepId = EconomyOrderValue.PlanStepId;
        UnitExecutionOrderValue.TargetCount = EconomyOrderValue.TargetCount;
        UnitExecutionOrderValue.RequestedQueueCount = EconomyOrderValue.RequestedQueueCount;
        UnitExecutionOrderValue.ProducerUnitTypeId = EconomyOrderValue.ProducerUnitTypeId;
        UnitExecutionOrderValue.ResultUnitTypeId = EconomyOrderValue.ResultUnitTypeId;
        UnitExecutionOrderValue.UpgradeId = EconomyOrderValue.UpgradeId;
        CommandAuthoritySchedulingStateValue.EnqueueOrder(UnitExecutionOrderValue);
        CommandAuthoritySchedulingStateValue.SetOrderDeferralState(
            EconomyOrderValue.OrderId, ECommandOrderDeferralReason::AwaitingObservedCompletion, CurrentStepValue,
            CurrentGameLoopValue);
    }
}

}  // namespace sc2
