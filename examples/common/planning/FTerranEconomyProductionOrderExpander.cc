#include "common/planning/FTerranEconomyProductionOrderExpander.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include "common/build_orders/FOpeningPlanRegistry.h"
#include "common/bot_status_models.h"
#include "common/catalogs/FTerranGoalRuleLibrary.h"
#include "common/economic_models.h"
#include "common/economy/EconomyForecastConstants.h"
#include "sc2api/sc2_map_info.h"
#include "sc2api/sc2_unit_filters.h"

namespace sc2
{
namespace
{

constexpr int FriendlyBlockerReliefIntentPriorityValue = 5000;
constexpr float ExistingMoveOrderDistanceSquaredToleranceValue = 1.0f;
constexpr float DisplacementFootprintPaddingValue = 1.5f;
constexpr uint64_t BlockerReliefRetryDelayGameLoopsValue = 32U;
constexpr uint32_t MaxSoftBlockWindowCountValue = 3U;

Point2D GetNormalizedDirection(const Point2D& DirectionValue)
{
    const float LengthSquaredValue =
        (DirectionValue.x * DirectionValue.x) + (DirectionValue.y * DirectionValue.y);
    if (LengthSquaredValue <= 0.0001f)
    {
        return Point2D(1.0f, 0.0f);
    }

    const float InverseLengthValue = 1.0f / std::sqrt(LengthSquaredValue);
    return Point2D(DirectionValue.x * InverseLengthValue, DirectionValue.y * InverseLengthValue);
}

Point2D GetCounterClockwiseLateralDirection(const Point2D& ForwardDirectionValue)
{
    return Point2D(-ForwardDirectionValue.y, ForwardDirectionValue.x);
}

const Unit* FindObservedStructureOccupyingPlacementSlot(const ObservationInterface& ObservationValue,
                                                        const ABILITY_ID StructureAbilityIdValue,
                                                        const FBuildPlacementSlot& BuildPlacementSlotValue);

bool TryResolvePreferredProducerUnit(
    const FFrameContext& FrameValue, const FGameStateDescriptor& GameStateDescriptorValue,
    const IBuildPlacementService& BuildPlacementServiceValue, const std::vector<Point2D>& ExpansionLocationsValue,
    const UNIT_TYPEID ProducerUnitTypeIdValue, const FBuildPlacementSlotId& PreferredProducerPlacementSlotIdValue,
    FBuildPlacementSlot& OutProducerPlacementSlotValue, const Unit*& OutProducerUnitValue);
UNIT_TYPEID GetProtectedAddonProducerUnitTypeForStructureAbility(const ABILITY_ID StructureAbilityIdValue);
bool DoesStructurePlacementOverlapObservedProtectedAddonLane(const FFrameContext& FrameValue,
                                                             const FCommandOrderRecord& EconomyOrderValue,
                                                             const FBuildPlacementSlot& BuildPlacementSlotValue);

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

bool IsRampWallDepotSlotType(const EBuildPlacementSlotType BuildPlacementSlotTypeValue)
{
    switch (BuildPlacementSlotTypeValue)
    {
        case EBuildPlacementSlotType::MainRampDepotLeft:
        case EBuildPlacementSlotType::MainRampDepotRight:
            return true;
        default:
            return false;
    }
}

FBuildPlacementSlotId CreatePlacementSlotId(const EBuildPlacementSlotType BuildPlacementSlotTypeValue,
                                            const uint8_t OrdinalValue)
{
    FBuildPlacementSlotId BuildPlacementSlotIdValue;
    BuildPlacementSlotIdValue.SlotType = BuildPlacementSlotTypeValue;
    BuildPlacementSlotIdValue.Ordinal = OrdinalValue;
    return BuildPlacementSlotIdValue;
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

bool IsMandatoryRampWallDepotOrder(const FCommandOrderRecord& CommandOrderRecordValue)
{
    return CommandOrderRecordValue.AbilityId == ABILITY_ID::BUILD_SUPPLYDEPOT &&
           CommandOrderRecordValue.CommitmentClass == ECommandCommitmentClass::MandatoryOpening &&
           CommandOrderRecordValue.ExecutionGuarantee == ECommandTaskExecutionGuarantee::MustExecute &&
           IsRampWallDepotSlotType(CommandOrderRecordValue.PreferredPlacementSlotType);
}

bool IsMandatoryRampWallDepotTaskDescriptor(const FCommandTaskDescriptor& CommandTaskDescriptorValue)
{
    return CommandTaskDescriptorValue.ActionAbilityId == ABILITY_ID::BUILD_SUPPLYDEPOT &&
           CommandTaskDescriptorValue.CommitmentClass == ECommandCommitmentClass::MandatoryOpening &&
           CommandTaskDescriptorValue.ExecutionGuarantee == ECommandTaskExecutionGuarantee::MustExecute &&
           IsRampWallDepotSlotType(CommandTaskDescriptorValue.ActionPreferredPlacementSlotType);
}

void ExpireOpeningStepOrders(FGameStateDescriptor& GameStateDescriptorValue, const uint32_t PlanStepIdValue)
{
    FOpeningPlanExecutionState& OpeningPlanExecutionStateValue = GameStateDescriptorValue.OpeningPlanExecutionState;
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState;
    uint32_t StrategicOrderIdValue = 0U;
    if (!OpeningPlanExecutionStateValue.TryGetPlanOrderId(PlanStepIdValue, StrategicOrderIdValue))
    {
        return;
    }

    CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(StrategicOrderIdValue, EOrderLifecycleState::Expired);
    const size_t OrderCountValue = CommandAuthoritySchedulingStateValue.OrderIds.size();
    for (size_t OrderIndexValue = 0U; OrderIndexValue < OrderCountValue; ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.ParentOrderIds[OrderIndexValue] != StrategicOrderIdValue ||
            IsTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(
            CommandAuthoritySchedulingStateValue.OrderIds[OrderIndexValue], EOrderLifecycleState::Expired);
    }

    OpeningPlanExecutionStateValue.UnseedStep(PlanStepIdValue);
}

void ActivateNaturalEntranceDepotFallback(FGameStateDescriptor& GameStateDescriptorValue)
{
    FOpeningPlanExecutionState& OpeningPlanExecutionStateValue = GameStateDescriptorValue.OpeningPlanExecutionState;
    if (OpeningPlanExecutionStateValue.WallChainState == EOpeningWallChainState::NaturalFallbackActive ||
        OpeningPlanExecutionStateValue.WallChainState == EOpeningWallChainState::Completed)
    {
        return;
    }

    OpeningPlanExecutionStateValue.WallChainState = EOpeningWallChainState::NaturalFallbackActive;
    OpeningPlanExecutionStateValue.ClearRemappedPlacementSlotIds();

    const FOpeningPlanDescriptor& OpeningPlanDescriptorValue =
        FOpeningPlanRegistry::GetOpeningPlanDescriptor(OpeningPlanExecutionStateValue.ActivePlanId);
    std::vector<FBuildPlacementSlotId> FallbackPlacementSlotIdsValue;
    FallbackPlacementSlotIdsValue.push_back(
        CreatePlacementSlotId(EBuildPlacementSlotType::NaturalEntranceDepotLeft, 0U));
    FallbackPlacementSlotIdsValue.push_back(
        CreatePlacementSlotId(EBuildPlacementSlotType::NaturalEntranceDepotRight, 0U));
    uint32_t NextFallbackSlotIndexValue = 0U;

    for (const FOpeningPlanStep& OpeningPlanStepValue : OpeningPlanDescriptorValue.Steps)
    {
        const FCommandTaskDescriptor& OpeningTaskDescriptorValue = OpeningPlanStepValue.TaskDescriptor;
        if (!IsMandatoryRampWallDepotTaskDescriptor(OpeningTaskDescriptorValue))
        {
            continue;
        }

        if (OpeningPlanExecutionStateValue.IsStepCompleted(OpeningTaskDescriptorValue.TaskId))
        {
            continue;
        }

        if (NextFallbackSlotIndexValue >= FallbackPlacementSlotIdsValue.size())
        {
            break;
        }

        OpeningPlanExecutionStateValue.SetRemappedPlacementSlotId(
            OpeningTaskDescriptorValue.TaskId, FallbackPlacementSlotIdsValue[NextFallbackSlotIndexValue]);
        OpeningPlanExecutionStateValue.ResetPlacementFailureCount(OpeningTaskDescriptorValue.TaskId);
        ExpireOpeningStepOrders(GameStateDescriptorValue, OpeningTaskDescriptorValue.TaskId);
        ++NextFallbackSlotIndexValue;
    }
}

void ApplyOpeningWallFallbackForPlacementDeferral(FGameStateDescriptor& GameStateDescriptorValue,
                                                  const FCommandOrderRecord& EconomyOrderValue,
                                                  const ECommandOrderDeferralReason DeferralReasonValue)
{
    if (!IsMandatoryRampWallDepotOrder(EconomyOrderValue))
    {
        return;
    }

    FOpeningPlanExecutionState& OpeningPlanExecutionStateValue = GameStateDescriptorValue.OpeningPlanExecutionState;
    if (OpeningPlanExecutionStateValue.WallChainState != EOpeningWallChainState::RampActive)
    {
        return;
    }

    switch (DeferralReasonValue)
    {
        case ECommandOrderDeferralReason::ReservedSlotInvalidated:
            ActivateNaturalEntranceDepotFallback(GameStateDescriptorValue);
            return;
        case ECommandOrderDeferralReason::ReservedSlotOccupied:
            if (OpeningPlanExecutionStateValue.IncrementPlacementFailureCount(EconomyOrderValue.PlanStepId) >= 3U)
            {
                ActivateNaturalEntranceDepotFallback(GameStateDescriptorValue);
            }
            return;
        default:
            OpeningPlanExecutionStateValue.ResetPlacementFailureCount(EconomyOrderValue.PlanStepId);
            return;
    }
}

bool IsTerranScvProducerUnitType(const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
        case UNIT_TYPEID::TERRAN_PLANETARYFORTRESS:
            return true;
        default:
            return false;
    }
}

Units GetEligibleWorkerProductionStructures(const ObservationInterface& ObservationValue)
{
    return ObservationValue.GetUnits(Unit::Alliance::Self,
                                     [](const Unit& UnitValue)
                                     { return IsTerranScvProducerUnitType(UnitValue.unit_type.ToType()); });
}

bool DoesPlacementSlotSatisfyFootprintPolicy(const FFrameContext& FrameValue,
                                             const FBuildPlacementSlot& BuildPlacementSlotValue,
                                             const ABILITY_ID StructureAbilityIdValue);

Point2D GetStructureFootprintHalfExtentsForAbility(const ABILITY_ID StructureAbilityIdValue);

Point2D GetStructureFootprintHalfExtentsForUnitType(const UNIT_TYPEID UnitTypeIdValue);

bool DoAxisAlignedFootprintsOverlap(const Point2D& CenterPointOneValue, const Point2D& HalfExtentsOneValue,
                                    const Point2D& CenterPointTwoValue, const Point2D& HalfExtentsTwoValue);

bool DoesStructureAbilityRequireAddonClearance(const ABILITY_ID StructureAbilityIdValue);

Point2D GetAddonFootprintCenter(const Point2D& StructureBuildPointValue);

bool DoesAddonFootprintSupportTerrain(const GameInfo& GameInfoValue, const Point2D& StructureBuildPointValue);

bool DoesAddonFootprintAvoidObservedStructures(const ObservationInterface& ObservationValue,
                                               const Point2D& StructureBuildPointValue);

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

uint32_t GetProjectedCountForOrder(const FGameStateDescriptor& GameStateDescriptorValue,
                                   const FCommandOrderRecord& CommandOrderRecordValue)
{
    if (CommandOrderRecordValue.UpgradeId.ToType() != UPGRADE_ID::INVALID)
    {
        const size_t UpgradeTypeIndexValue = GetTerranUpgradeTypeIndex(CommandOrderRecordValue.UpgradeId);
        return IsTerranUpgradeTypeIndexValid(UpgradeTypeIndexValue)
                   ? static_cast<uint32_t>(GameStateDescriptorValue.BuildPlanning
                                               .ObservedCompletedUpgradeCounts[UpgradeTypeIndexValue]) +
                         GameStateDescriptorValue.SchedulerOutlook.GetScheduledUpgradeCount(
                             CommandOrderRecordValue.UpgradeId)
                   : 0U;
    }

    if (IsTerranBuilding(CommandOrderRecordValue.ResultUnitTypeId) ||
        CommandOrderRecordValue.ResultUnitTypeId == UNIT_TYPEID::TERRAN_COMMANDCENTER)
    {
        return GameStateDescriptorValue.ProductionState.GetProjectedBuildingCount(
            CommandOrderRecordValue.ResultUnitTypeId);
    }

    return GameStateDescriptorValue.ProductionState.GetProjectedUnitCount(CommandOrderRecordValue.ResultUnitTypeId);
}

bool DoesOrderTargetMatchObservedState(const FGameStateDescriptor& GameStateDescriptorValue,
                                       const FCommandOrderRecord& CommandOrderRecordValue)
{
    if (CommandOrderRecordValue.TargetCount == 0U)
    {
        return false;
    }

    FBuildPlacementSlotId BuildPlacementSlotIdValue;
    if (TryGetObservedExactPlacementSlotId(CommandOrderRecordValue, BuildPlacementSlotIdValue))
    {
        return GameStateDescriptorValue.ObservedPlacementSlotState.GetObservedPlacementSlotState(
                   BuildPlacementSlotIdValue) == EObservedWallSlotState::Occupied;
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

bool HasActiveSchedulerOrderForActorExcludingOrder(
    const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
    const Tag ActorTagValue,
    const uint32_t IgnoredOrderIdValue)
{
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.OrderIds[OrderIndexValue] == IgnoredOrderIdValue ||
            CommandAuthoritySchedulingStateValue.ActorTags[OrderIndexValue] != ActorTagValue ||
            IsOrderTerminal(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        // Skip UnitExecution children of the ignored parent order so the morph
        // dispatch does not self-block when its own child targets the same actor.
        if (CommandAuthoritySchedulingStateValue.ParentOrderIds[OrderIndexValue] ==
            static_cast<int>(IgnoredOrderIdValue))
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
            CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue] == EOrderLifecycleState::Dispatched ||
            CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] != ECommandAuthorityLayer::UnitExecution ||
            CommandAuthoritySchedulingStateValue.IntentDomains[OrderIndexValue] != EIntentDomain::UnitProduction)
        {
            continue;
        }

        ++OrderCountValue;
    }

    return OrderCountValue;
}

bool HasPendingMorphOrderForActor(
    const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue, const Tag ActorTagValue,
    const ABILITY_ID MorphAbilityIdValue)
{
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.ActorTags[OrderIndexValue] != ActorTagValue ||
            IsOrderTerminal(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        if (CommandAuthoritySchedulingStateValue.AbilityIds[OrderIndexValue] == MorphAbilityIdValue)
        {
            return true;
        }
    }

    return false;
}

uint32_t CountPendingMorphOrders(
    const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
    const ABILITY_ID MorphAbilityIdValue)
{
    uint32_t PendingMorphOrderCountValue = 0U;
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (IsOrderTerminal(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        if (CommandAuthoritySchedulingStateValue.AbilityIds[OrderIndexValue] == MorphAbilityIdValue)
        {
            ++PendingMorphOrderCountValue;
        }
    }

    return PendingMorphOrderCountValue;
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

        if (CommandAuthoritySchedulingStateValue.IntentDomains[OrderIndexValue] == EIntentDomain::StructureBuild &&
            CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue] != EOrderLifecycleState::Dispatched)
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

bool DoesOrderSelfContributeToProjectedCount(
    const FGameStateDescriptor& GameStateDescriptorValue,
    const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
    const FCommandOrderRecord& CommandOrderRecordValue)
{
    if (HasActiveUnitExecutionChild(CommandAuthoritySchedulingStateValue, CommandOrderRecordValue.OrderId))
    {
        return false;
    }

    if (CommandOrderRecordValue.ResultUnitTypeId == UNIT_TYPEID::INVALID &&
        CommandOrderRecordValue.UpgradeId.ToType() == UPGRADE_ID::INVALID)
    {
        return false;
    }

    if (CommandOrderRecordValue.DispatchAttemptCount == 0U)
    {
        return true;
    }

    const uint32_t CurrentObservedCountValue =
        GetObservedCountForOrder(GameStateDescriptorValue.BuildPlanning, CommandOrderRecordValue);
    const uint32_t CurrentObservedInConstructionCountValue =
        GetObservedInConstructionCountForOrder(GameStateDescriptorValue.BuildPlanning, CommandOrderRecordValue);

    return !(CurrentObservedCountValue > CommandOrderRecordValue.ObservedCountAtDispatch ||
             CurrentObservedInConstructionCountValue >
                 CommandOrderRecordValue.ObservedInConstructionCountAtDispatch);
}

uint32_t GetProjectedCountForOrderExcludingSelf(
    const FGameStateDescriptor& GameStateDescriptorValue,
    const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
    const FCommandOrderRecord& CommandOrderRecordValue)
{
    uint32_t ProjectedCountValue = GetProjectedCountForOrder(GameStateDescriptorValue, CommandOrderRecordValue);
    if (ProjectedCountValue == 0U)
    {
        return 0U;
    }

    if (DoesOrderSelfContributeToProjectedCount(GameStateDescriptorValue, CommandAuthoritySchedulingStateValue,
                                                CommandOrderRecordValue))
    {
        --ProjectedCountValue;
    }

    return ProjectedCountValue;
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

bool IsFriendlyMovableBlockerUnit(const Unit& UnitValue, const Tag IgnoredActorTagValue)
{
    if (UnitValue.tag == IgnoredActorTagValue || !UnitValue.is_alive || UnitValue.is_building ||
        UnitValue.is_flying || UnitValue.build_progress < 1.0f)
    {
        return false;
    }

    for (const UnitOrder& UnitOrderValue : UnitValue.orders)
    {
        if (IsStructureBuildAbility(UnitOrderValue.ability_id))
        {
            return false;
        }
    }

    return true;
}

bool DoesUnitOverlapFootprint(const Unit& UnitValue, const Point2D& FootprintCenterValue,
                              const Point2D& FootprintHalfExtentsValue)
{
    const Point2D UnitHalfExtentsValue(UnitValue.radius, UnitValue.radius);
    return DoAxisAlignedFootprintsOverlap(Point2D(UnitValue.pos), UnitHalfExtentsValue, FootprintCenterValue,
                                          FootprintHalfExtentsValue);
}

bool DoesFootprintContainFriendlyBuilding(const ObservationInterface& ObservationValue, const Tag IgnoredActorTagValue,
                                          const Point2D& FootprintCenterValue,
                                          const Point2D& FootprintHalfExtentsValue)
{
    const Units SelfUnitsValue = ObservationValue.GetUnits(Unit::Alliance::Self);
    for (const Unit* SelfUnitValue : SelfUnitsValue)
    {
        if (SelfUnitValue == nullptr || SelfUnitValue->tag == IgnoredActorTagValue || !SelfUnitValue->is_building ||
            SelfUnitValue->is_flying)
        {
            continue;
        }

        const Point2D StructureHalfExtentsValue =
            GetStructureFootprintHalfExtentsForUnitType(SelfUnitValue->unit_type.ToType());
        if (DoAxisAlignedFootprintsOverlap(Point2D(SelfUnitValue->pos), StructureHalfExtentsValue,
                                           FootprintCenterValue, FootprintHalfExtentsValue))
        {
            return true;
        }
    }

    return false;
}

bool DoesFootprintContainFriendlyMovableBlocker(const ObservationInterface& ObservationValue,
                                                const Tag IgnoredActorTagValue,
                                                const Point2D& FootprintCenterValue,
                                                const Point2D& FootprintHalfExtentsValue)
{
    const Units SelfUnitsValue = ObservationValue.GetUnits(Unit::Alliance::Self);
    for (const Unit* SelfUnitValue : SelfUnitsValue)
    {
        if (SelfUnitValue == nullptr || !IsFriendlyMovableBlockerUnit(*SelfUnitValue, IgnoredActorTagValue))
        {
            continue;
        }

        if (DoesUnitOverlapFootprint(*SelfUnitValue, FootprintCenterValue, FootprintHalfExtentsValue))
        {
            return true;
        }
    }

    return false;
}

bool DoesUnitAlreadyHaveMoveOrderNearPoint(const Unit& UnitValue, const Point2D& TargetPointValue)
{
    if (UnitValue.orders.empty())
    {
        return false;
    }

    const UnitOrder& UnitOrderValue = UnitValue.orders.front();
    switch (UnitOrderValue.ability_id.ToType())
    {
        case ABILITY_ID::GENERAL_MOVE:
        case ABILITY_ID::SMART:
        case ABILITY_ID::ATTACK_ATTACK:
            return DistanceSquared2D(UnitOrderValue.target_pos, TargetPointValue) <=
                   ExistingMoveOrderDistanceSquaredToleranceValue;
        default:
            return false;
    }
}

bool TryFindDisplacementTargetPoint(const FFrameContext& FrameValue, const Unit& BlockerUnitValue,
                                    const Point2D& FootprintCenterValue,
                                    const Point2D& FootprintHalfExtentsValue,
                                    Point2D& OutDisplacementTargetPointValue)
{
    if (FrameValue.GameInfo == nullptr || FrameValue.Query == nullptr || FrameValue.Observation == nullptr)
    {
        return false;
    }

    static const std::array<Point2D, 8U> CandidateDirectionsValue =
    {{
        Point2D(1.0f, 0.0f),
        Point2D(-1.0f, 0.0f),
        Point2D(0.0f, 1.0f),
        Point2D(0.0f, -1.0f),
        Point2D(0.70710678f, 0.70710678f),
        Point2D(0.70710678f, -0.70710678f),
        Point2D(-0.70710678f, 0.70710678f),
        Point2D(-0.70710678f, -0.70710678f),
    }};

    const Point2D BlockerHalfExtentsValue(BlockerUnitValue.radius, BlockerUnitValue.radius);
    const float CandidateDistanceValue =
        std::max(FootprintHalfExtentsValue.x, FootprintHalfExtentsValue.y) + BlockerUnitValue.radius +
        DisplacementFootprintPaddingValue;

    bool bFoundCandidateValue = false;
    float BestCandidateDistanceSquaredValue = std::numeric_limits<float>::max();
    for (const Point2D& CandidateDirectionValue : CandidateDirectionsValue)
    {
        const Point2D CandidatePointValue = ClampToPlayable(
            *FrameValue.GameInfo, FootprintCenterValue + (CandidateDirectionValue * CandidateDistanceValue));
        if (DoAxisAlignedFootprintsOverlap(CandidatePointValue, BlockerHalfExtentsValue, FootprintCenterValue,
                                           FootprintHalfExtentsValue) ||
            !FrameValue.Observation->IsPathable(CandidatePointValue))
        {
            continue;
        }

        const float PathingDistanceValue = FrameValue.Query->PathingDistance(&BlockerUnitValue, CandidatePointValue);
        if (!IsGroundPathingResultValid(BlockerUnitValue, CandidatePointValue, PathingDistanceValue))
        {
            continue;
        }

        const float UnitTravelDistanceSquaredValue =
            DistanceSquared2D(Point2D(BlockerUnitValue.pos), CandidatePointValue);
        if (bFoundCandidateValue && UnitTravelDistanceSquaredValue >= BestCandidateDistanceSquaredValue)
        {
            continue;
        }

        bFoundCandidateValue = true;
        BestCandidateDistanceSquaredValue = UnitTravelDistanceSquaredValue;
        OutDisplacementTargetPointValue = CandidatePointValue;
    }

    return bFoundCandidateValue;
}

bool TryFindPreferredBlockerReliefTargetPoint(const FFrameContext& FrameValue, const Unit& BlockerUnitValue,
                                              const Point2D& FootprintCenterValue,
                                              const Point2D& FootprintHalfExtentsValue,
                                              const Point2D& PreferredTargetPointValue,
                                              Point2D& OutDisplacementTargetPointValue)
{
    if (FrameValue.Observation == nullptr || FrameValue.Query == nullptr)
    {
        return false;
    }

    if (DoAxisAlignedFootprintsOverlap(PreferredTargetPointValue, Point2D(BlockerUnitValue.radius, BlockerUnitValue.radius),
                                       FootprintCenterValue, FootprintHalfExtentsValue) ||
        !FrameValue.Observation->IsPathable(PreferredTargetPointValue))
    {
        return false;
    }

    const float PathingDistanceValue = FrameValue.Query->PathingDistance(&BlockerUnitValue, PreferredTargetPointValue);
    if (!IsGroundPathingResultValid(BlockerUnitValue, PreferredTargetPointValue, PathingDistanceValue))
    {
        return false;
    }

    OutDisplacementTargetPointValue = PreferredTargetPointValue;
    return true;
}

uint64_t BuildFriendlyMovableBlockerFingerprint(const FFrameContext& FrameValue, const Tag IgnoredActorTagValue,
                                                const Point2D& FootprintCenterValue,
                                                const Point2D& FootprintHalfExtentsValue)
{
    if (FrameValue.Observation == nullptr)
    {
        return 0U;
    }

    uint64_t FingerprintValue = 1469598103934665603ULL;
    const Units SelfUnitsValue = FrameValue.Observation->GetUnits(Unit::Alliance::Self);
    for (const Unit* SelfUnitValue : SelfUnitsValue)
    {
        if (SelfUnitValue == nullptr || !IsFriendlyMovableBlockerUnit(*SelfUnitValue, IgnoredActorTagValue) ||
            !DoesUnitOverlapFootprint(*SelfUnitValue, FootprintCenterValue, FootprintHalfExtentsValue))
        {
            continue;
        }

        FingerprintValue ^= static_cast<uint64_t>(SelfUnitValue->tag);
        FingerprintValue *= 1099511628211ULL;
    }

    return FingerprintValue;
}

EProductionBlockerKind ClassifyAddonFootprintBlocker(const FFrameContext& FrameValue, const Tag ProducerTagValue,
                                                     const Point2D& StructureBuildPointValue)
{
    if (FrameValue.Observation == nullptr || FrameValue.GameInfo == nullptr)
    {
        return EProductionBlockerKind::Terrain;
    }

    if (!DoesAddonFootprintSupportTerrain(*FrameValue.GameInfo, StructureBuildPointValue))
    {
        return EProductionBlockerKind::Terrain;
    }

    const Point2D AddonFootprintCenterValue = GetAddonFootprintCenter(StructureBuildPointValue);
    const Point2D AddonHalfExtentsValue(1.0f, 1.0f);
    const Units AllUnitsValue = FrameValue.Observation->GetUnits();
    for (const Unit* CandidateUnitValue : AllUnitsValue)
    {
        if (CandidateUnitValue == nullptr || CandidateUnitValue->tag == ProducerTagValue)
        {
            continue;
        }

        // For buildings, use the structure footprint half extents (grid-aligned) instead of
        // the unit radius (circular collision), which is typically larger and causes false
        // positives for edge-adjacent buildings like ramp wall depots.
        const bool IsOverlappingValue = CandidateUnitValue->is_building
            ? DoAxisAlignedFootprintsOverlap(
                  Point2D(CandidateUnitValue->pos),
                  GetStructureFootprintHalfExtentsForUnitType(CandidateUnitValue->unit_type.ToType()),
                  AddonFootprintCenterValue, AddonHalfExtentsValue)
            : DoesUnitOverlapFootprint(*CandidateUnitValue, AddonFootprintCenterValue, AddonHalfExtentsValue);

        if (!IsOverlappingValue)
        {
            continue;
        }

#if _DEBUG
        if (FrameValue.GameLoop % 224U == 0U)
        {
            std::cout << "[ADDON_BLOCKER] producer_pos=(" << StructureBuildPointValue.x << ","
                      << StructureBuildPointValue.y << ") addon_center=(" << AddonFootprintCenterValue.x << ","
                      << AddonFootprintCenterValue.y << ") blocker_tag=" << CandidateUnitValue->tag
                      << " blocker_type=" << static_cast<int>(CandidateUnitValue->unit_type)
                      << " blocker_pos=(" << CandidateUnitValue->pos.x << "," << CandidateUnitValue->pos.y << ")"
                      << " is_building=" << CandidateUnitValue->is_building
                      << " alliance=" << static_cast<int>(CandidateUnitValue->alliance)
                      << " loop=" << FrameValue.GameLoop << std::endl;
        }
#endif

        switch (CandidateUnitValue->alliance)
        {
            case Unit::Alliance::Self:
                return CandidateUnitValue->is_building ? EProductionBlockerKind::FriendlyStructure
                                                       : EProductionBlockerKind::FriendlyMovableUnit;
            case Unit::Alliance::Enemy:
                return CandidateUnitValue->is_building ? EProductionBlockerKind::EnemyStructure
                                                       : EProductionBlockerKind::EnemyUnit;
            case Unit::Alliance::Neutral:
                return EProductionBlockerKind::ResourceNode;
            default:
                break;
        }
    }

    return EProductionBlockerKind::None;
}

uint32_t AddFriendlyBlockerReliefIntentsForFootprint(const FFrameContext& FrameValue, const Tag IgnoredActorTagValue,
                                                     const Point2D& FootprintCenterValue,
                                                     const Point2D& FootprintHalfExtentsValue,
                                                     const Point2D& PreferredTargetPointValue,
                                                     FIntentBuffer& IntentBufferValue)
{
    if (FrameValue.Observation == nullptr)
    {
        return 0U;
    }

    uint32_t AddedIntentCountValue = 0U;
    const Units SelfUnitsValue = FrameValue.Observation->GetUnits(Unit::Alliance::Self);
    for (const Unit* SelfUnitValue : SelfUnitsValue)
    {
        if (SelfUnitValue == nullptr)
        {
            continue;
        }

        const bool IsFriendlyMovableValue = IsFriendlyMovableBlockerUnit(*SelfUnitValue, IgnoredActorTagValue);
        const bool IsOverlappingValue = DoesUnitOverlapFootprint(*SelfUnitValue, FootprintCenterValue,
                                                                  FootprintHalfExtentsValue);
        const bool HasExistingIntentValue = IntentBufferValue.HasIntentForActor(SelfUnitValue->tag);

        if (!IsFriendlyMovableValue || !IsOverlappingValue || HasExistingIntentValue)
        {
#if _DEBUG
            if (IsOverlappingValue && IsFriendlyMovableValue && HasExistingIntentValue &&
                FrameValue.GameLoop % 224U == 0U)
            {
                std::cout << "[RELIEF_DIAG] SkippedDueToExistingIntent tag=" << SelfUnitValue->tag
                          << " type=" << static_cast<int>(SelfUnitValue->unit_type.ToType())
                          << " pos=(" << SelfUnitValue->pos.x << "," << SelfUnitValue->pos.y << ")"
                          << " loop=" << FrameValue.GameLoop << std::endl;
            }
#endif
            continue;
        }

        Point2D DisplacementTargetPointValue;
        const bool bFoundPreferredValue =
            TryFindPreferredBlockerReliefTargetPoint(FrameValue, *SelfUnitValue, FootprintCenterValue,
                                                     FootprintHalfExtentsValue, PreferredTargetPointValue,
                                                     DisplacementTargetPointValue);
        const bool bFoundDisplacementValue = !bFoundPreferredValue &&
            TryFindDisplacementTargetPoint(FrameValue, *SelfUnitValue, FootprintCenterValue,
                                           FootprintHalfExtentsValue, DisplacementTargetPointValue);
        const bool bFoundReliefTargetValue = bFoundPreferredValue || bFoundDisplacementValue;
        const bool bAlreadyMovingValue = bFoundReliefTargetValue &&
            DoesUnitAlreadyHaveMoveOrderNearPoint(*SelfUnitValue, DisplacementTargetPointValue);

#if _DEBUG
        if (FrameValue.GameLoop % 224U == 0U)
        {
            std::cout << "[RELIEF_DIAG] tag=" << SelfUnitValue->tag
                      << " type=" << static_cast<int>(SelfUnitValue->unit_type.ToType())
                      << " pos=(" << SelfUnitValue->pos.x << "," << SelfUnitValue->pos.y << ")"
                      << " FoundPreferred=" << bFoundPreferredValue
                      << " FoundDisplacement=" << bFoundDisplacementValue
                      << " AlreadyMoving=" << bAlreadyMovingValue
                      << " loop=" << FrameValue.GameLoop << std::endl;
        }
#endif

        if (!bFoundReliefTargetValue || bAlreadyMovingValue)
        {
            continue;
        }

        IntentBufferValue.Add(FUnitIntent::CreatePointTarget(
            SelfUnitValue->tag, ABILITY_ID::GENERAL_MOVE, DisplacementTargetPointValue,
            FriendlyBlockerReliefIntentPriorityValue, EIntentDomain::Recovery, true));
        ++AddedIntentCountValue;
    }

    return AddedIntentCountValue;
}

bool TryIssueFriendlyBlockerReliefForStructurePlacement(const FFrameContext& FrameValue,
                                                        const FCommandOrderRecord& EconomyOrderValue,
                                                        const Unit& WorkerUnitValue,
                                                        const FBuildPlacementSlot& BuildPlacementSlotValue,
                                                        const Point2D& PreferredTargetPointValue,
                                                        FIntentBuffer& IntentBufferValue)
{
    const Point2D StructureHalfExtentsValue =
        GetStructureFootprintHalfExtentsForAbility(EconomyOrderValue.AbilityId);
    uint32_t AddedIntentCountValue = AddFriendlyBlockerReliefIntentsForFootprint(
        FrameValue, WorkerUnitValue.tag, BuildPlacementSlotValue.BuildPoint, StructureHalfExtentsValue,
        PreferredTargetPointValue,
        IntentBufferValue);

    if (DoesStructureAbilityRequireAddonClearance(EconomyOrderValue.AbilityId))
    {
        AddedIntentCountValue += AddFriendlyBlockerReliefIntentsForFootprint(
            FrameValue, WorkerUnitValue.tag, GetAddonFootprintCenter(BuildPlacementSlotValue.BuildPoint),
            Point2D(1.0f, 1.0f), PreferredTargetPointValue, IntentBufferValue);
    }

    return AddedIntentCountValue > 0U;
}

bool DoesStructurePlacementHaveFriendlyMovableBlocker(const FFrameContext& FrameValue,
                                                      const FCommandOrderRecord& EconomyOrderValue,
                                                      const Unit& WorkerUnitValue,
                                                      const FBuildPlacementSlot& BuildPlacementSlotValue)
{
    if (FrameValue.Observation == nullptr)
    {
        return false;
    }

    const Point2D StructureHalfExtentsValue =
        GetStructureFootprintHalfExtentsForAbility(EconomyOrderValue.AbilityId);
    if (DoesFootprintContainFriendlyMovableBlocker(*FrameValue.Observation, WorkerUnitValue.tag,
                                                   BuildPlacementSlotValue.BuildPoint,
                                                   StructureHalfExtentsValue))
    {
        return true;
    }

    if (!DoesStructureAbilityRequireAddonClearance(EconomyOrderValue.AbilityId))
    {
        return false;
    }

    return DoesFootprintContainFriendlyMovableBlocker(*FrameValue.Observation, WorkerUnitValue.tag,
                                                      GetAddonFootprintCenter(BuildPlacementSlotValue.BuildPoint),
                                                      Point2D(1.0f, 1.0f));
}

bool DoesAddonFootprintHaveHardBlocker(const FFrameContext& FrameValue, const Tag ProducerTagValue,
                                       const Point2D& StructureBuildPointValue)
{
    if (FrameValue.GameInfo == nullptr || FrameValue.Observation == nullptr)
    {
        return true;
    }

    if (!DoesAddonFootprintSupportTerrain(*FrameValue.GameInfo, StructureBuildPointValue) ||
        !DoesAddonFootprintAvoidObservedStructures(*FrameValue.Observation, StructureBuildPointValue))
    {
        return true;
    }

    return DoesFootprintContainFriendlyBuilding(*FrameValue.Observation, ProducerTagValue,
                                                GetAddonFootprintCenter(StructureBuildPointValue),
                                                Point2D(1.0f, 1.0f));
}

bool DoesAddonFootprintContainFriendlyMovableBlocker(const FFrameContext& FrameValue, const Tag ProducerTagValue,
                                                     const Point2D& StructureBuildPointValue)
{
    if (FrameValue.Observation == nullptr)
    {
        return false;
    }

    return DoesFootprintContainFriendlyMovableBlocker(*FrameValue.Observation, ProducerTagValue,
                                                      GetAddonFootprintCenter(StructureBuildPointValue),
                                                      Point2D(1.0f, 1.0f));
}

bool TryIssueFriendlyBlockerReliefForAddonFootprint(const FFrameContext& FrameValue, const Unit& ProducerUnitValue,
                                                    const Point2D& PreferredTargetPointValue,
                                                    FIntentBuffer& IntentBufferValue)
{
    return AddFriendlyBlockerReliefIntentsForFootprint(
               FrameValue, ProducerUnitValue.tag, GetAddonFootprintCenter(Point2D(ProducerUnitValue.pos)),
               Point2D(1.0f, 1.0f), PreferredTargetPointValue, IntentBufferValue) > 0U;
}

bool HasPlacementSlotBeenAttempted(const std::vector<FBuildPlacementSlotId>& AttemptedPlacementSlotIdsValue,
                                   const FBuildPlacementSlotId& BuildPlacementSlotIdValue)
{
    return std::find(AttemptedPlacementSlotIdsValue.begin(), AttemptedPlacementSlotIdsValue.end(),
                     BuildPlacementSlotIdValue) != AttemptedPlacementSlotIdsValue.end();
}

bool IsUnitTrainingAbility(const ABILITY_ID AbilityIdValue)
{
    switch (AbilityIdValue)
    {
        case ABILITY_ID::TRAIN_MARINE:
        case ABILITY_ID::TRAIN_MARAUDER:
        case ABILITY_ID::TRAIN_HELLION:
        case ABILITY_ID::TRAIN_CYCLONE:
        case ABILITY_ID::TRAIN_MEDIVAC:
        case ABILITY_ID::TRAIN_LIBERATOR:
        case ABILITY_ID::TRAIN_SIEGETANK:
        case ABILITY_ID::TRAIN_WIDOWMINE:
        case ABILITY_ID::TRAIN_VIKINGFIGHTER:
            return true;
        default:
            return false;
    }
}

bool AreAllOrdersUnitTraining(const Unit& ProducerUnitValue)
{
    for (const UnitOrder& OrderValue : ProducerUnitValue.orders)
    {
        if (!IsUnitTrainingAbility(OrderValue.ability_id))
        {
            return false;
        }
    }
    return true;
}

bool TrySelectAddonProducerUnit(const FFrameContext& FrameValue,
                                const FGameStateDescriptor& GameStateDescriptorValue,
                                const IBuildPlacementService& BuildPlacementServiceValue,
                                const std::vector<Point2D>& ExpansionLocationsValue,
                                const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                const FCommandOrderRecord& EconomyOrderValue,
                                const UNIT_TYPEID ProducerUnitTypeIdValue,
                                const Point2D& BlockerReliefTargetPointValue,
                                std::unordered_map<FCommandTaskSignatureKey, FProductionBlockerResolution,
                                                   FCommandTaskSignatureKeyHash>& ProductionBlockerResolutionsValue,
                                FIntentBuffer& IntentBufferValue,
                                const Unit*& OutProducerUnitValue,
                                ECommandOrderDeferralReason& OutDeferralReasonValue)
{
    OutProducerUnitValue = nullptr;
    if (FrameValue.Observation == nullptr)
    {
        OutDeferralReasonValue = ECommandOrderDeferralReason::NoProducer;
        return false;
    }

    bool HasBusyProducerWithoutAddonValue = false;
    bool HasHardBlockedProducerValue = false;
    bool HasSoftBlockedProducerValue = false;
    FProductionBlockerResolution& ProductionBlockerResolutionValue =
        ProductionBlockerResolutionsValue[FCommandTaskSignatureKey::FromOrderRecord(EconomyOrderValue)];
    std::vector<const Unit*> CandidateProducerUnitsValue;
    if (EconomyOrderValue.PreferredProducerPlacementSlotId.IsValid())
    {
        FBuildPlacementSlot PreferredProducerPlacementSlotValue;
        const Unit* PreferredProducerUnitValue = nullptr;
        if (!TryResolvePreferredProducerUnit(FrameValue, GameStateDescriptorValue, BuildPlacementServiceValue,
                                             ExpansionLocationsValue, ProducerUnitTypeIdValue,
                                             EconomyOrderValue.PreferredProducerPlacementSlotId,
                                             PreferredProducerPlacementSlotValue, PreferredProducerUnitValue))
        {
#if _DEBUG
            if (FrameValue.GameLoop % 224U == 0U)
            {
                std::cout << "[ADDON_DIAG] PreferredProducerResolveFailed SlotType="
                          << static_cast<int>(EconomyOrderValue.PreferredProducerPlacementSlotId.SlotType)
                          << " SlotOrdinal=" << static_cast<int>(EconomyOrderValue.PreferredProducerPlacementSlotId.Ordinal)
                          << " ProducerType=" << static_cast<int>(ProducerUnitTypeIdValue)
                          << " OrderId=" << EconomyOrderValue.OrderId
                          << " loop=" << FrameValue.GameLoop << std::endl;
            }
#endif
            OutDeferralReasonValue = ECommandOrderDeferralReason::NoProducer;
            return false;
        }

        CandidateProducerUnitsValue.push_back(PreferredProducerUnitValue);
    }
    else
    {
        const Units ProducerUnitsValue =
            FrameValue.Observation->GetUnits(Unit::Alliance::Self, IsUnit(ProducerUnitTypeIdValue));
        CandidateProducerUnitsValue.reserve(ProducerUnitsValue.size());
        for (const Unit* ProducerUnitValue : ProducerUnitsValue)
        {
            CandidateProducerUnitsValue.push_back(ProducerUnitValue);
        }
    }

    for (const Unit* CandidateProducerUnitValue : CandidateProducerUnitsValue)
    {
        if (CandidateProducerUnitValue == nullptr || CandidateProducerUnitValue->add_on_tag != NullTag)
        {
            continue;
        }

        if (CandidateProducerUnitValue->build_progress < 1.0f ||
            IntentBufferValue.HasIntentForActor(CandidateProducerUnitValue->tag))
        {
#if _DEBUG
            if (FrameValue.GameLoop % 224U == 0U)
            {
                std::cout << "[ADDON_DIAG] tag=" << CandidateProducerUnitValue->tag
                          << " SKIP_PROGRESS_OR_INTENT bp=" << CandidateProducerUnitValue->build_progress
                          << " intent=" << IntentBufferValue.HasIntentForActor(CandidateProducerUnitValue->tag)
                          << " loop=" << FrameValue.GameLoop << std::endl;
            }
#endif
            HasBusyProducerWithoutAddonValue = true;
            continue;
        }

        // A barracks currently training units (marines, etc.) is still eligible for addon
        // construction. The addon build will cancel the training queue. Only skip if the
        // barracks has non-training orders (e.g. already building an addon or lifting).
        if (!CandidateProducerUnitValue->orders.empty() &&
            !AreAllOrdersUnitTraining(*CandidateProducerUnitValue))
        {
#if _DEBUG
            if (FrameValue.GameLoop % 224U == 0U)
            {
                std::cout << "[ADDON_DIAG] tag=" << CandidateProducerUnitValue->tag
                          << " SKIP_NON_TRAIN_ORDERS orders=" << CandidateProducerUnitValue->orders.size()
                          << " loop=" << FrameValue.GameLoop << std::endl;
            }
#endif
            HasBusyProducerWithoutAddonValue = true;
            continue;
        }

        // Skip if a scheduler order already targets this barracks for an addon build
        if (HasActiveSchedulerOrderForActor(CommandAuthoritySchedulingStateValue, CandidateProducerUnitValue->tag))
        {
#if _DEBUG
            if (FrameValue.GameLoop % 224U == 0U)
            {
                std::cout << "[ADDON_DIAG] tag=" << CandidateProducerUnitValue->tag
                          << " SKIP_HAS_SCHED_ORDER loop=" << FrameValue.GameLoop << std::endl;
            }
#endif
            HasBusyProducerWithoutAddonValue = true;
            continue;
        }

        const Point2D ProducerBuildPointValue = Point2D(CandidateProducerUnitValue->pos);

        // Ramp wall barracks addon positions are validated at layout initialization by
        // CreateDiscoveredRampWallDescriptor, which tests 18 shifted candidates with batch
        // placement queries. Trust that validated position and skip runtime re-validation
        // which can give inconsistent results on ramp terrain.
        const bool IsValidatedRampWallSlotValue =
            EconomyOrderValue.PreferredProducerPlacementSlotId.SlotType ==
                EBuildPlacementSlotType::MainRampBarracksWithAddon;
        if (IsValidatedRampWallSlotValue)
        {
            ProductionBlockerResolutionValue.Reset();
            OutProducerUnitValue = CandidateProducerUnitValue;
            return true;
        }

        const EProductionBlockerKind ProductionBlockerKindValue =
            ClassifyAddonFootprintBlocker(FrameValue, CandidateProducerUnitValue->tag, ProducerBuildPointValue);
#if _DEBUG
        if (FrameValue.GameLoop % 224U == 0U)
        {
            std::cout << "[ADDON_DIAG] tag=" << CandidateProducerUnitValue->tag
                      << " pos=(" << ProducerBuildPointValue.x << "," << ProducerBuildPointValue.y << ")"
                      << " BLOCKER=" << static_cast<int>(ProductionBlockerKindValue)
                      << " loop=" << FrameValue.GameLoop << std::endl;
        }
#endif
        if (ProductionBlockerKindValue == EProductionBlockerKind::None)
        {
            // ClassifyAddonFootprintBlocker verifies terrain buildability via
            // DoesAddonFootprintSupportTerrain (5-point PlacementGrid sampling) and
            // structure overlap via DoesAddonFootprintAvoidObservedStructures.
            // Both passed (BLOCKER=None). Trust these deterministic checks rather than
            // Query->Placement on the addon center, which incorrectly rejects valid
            // addon positions on ramp-adjacent terrain. SC2 addon builds are issued
            // to the producer structure, not placed independently at a point.
            ProductionBlockerResolutionValue.Reset();
            OutProducerUnitValue = CandidateProducerUnitValue;
            return true;
        }

        if (ProductionBlockerKindValue == EProductionBlockerKind::FriendlyMovableUnit)
        {
            const uint64_t BlockerFingerprintValue = BuildFriendlyMovableBlockerFingerprint(
                FrameValue, CandidateProducerUnitValue->tag, GetAddonFootprintCenter(ProducerBuildPointValue),
                Point2D(1.0f, 1.0f));
            if (ProductionBlockerResolutionValue.BlockerKind == EProductionBlockerKind::FriendlyMovableUnit &&
                ProductionBlockerResolutionValue.BlockerFingerprint == BlockerFingerprintValue &&
                FrameValue.Observation->GetGameLoop() < ProductionBlockerResolutionValue.RetryNotBeforeGameLoop)
            {
                HasSoftBlockedProducerValue = true;
                continue;
            }

            ProductionBlockerResolutionValue.BlockerKind = EProductionBlockerKind::FriendlyMovableUnit;
            ProductionBlockerResolutionValue.bCanAttemptRelief = true;
            ProductionBlockerResolutionValue.bCanRelocate = EconomyOrderValue.Origin != ECommandTaskOrigin::Opening;
            ProductionBlockerResolutionValue.RetryNotBeforeGameLoop =
                FrameValue.Observation->GetGameLoop() + BlockerReliefRetryDelayGameLoopsValue;
            ProductionBlockerResolutionValue.SoftBlockWindowCount =
                ProductionBlockerResolutionValue.BlockerFingerprint == BlockerFingerprintValue
                    ? (ProductionBlockerResolutionValue.SoftBlockWindowCount + 1U)
                    : 1U;
            ProductionBlockerResolutionValue.BlockerFingerprint = BlockerFingerprintValue;
            HasSoftBlockedProducerValue = true;
            TryIssueFriendlyBlockerReliefForAddonFootprint(FrameValue, *CandidateProducerUnitValue,
                                                           BlockerReliefTargetPointValue, IntentBufferValue);
            if (ProductionBlockerResolutionValue.SoftBlockWindowCount >= MaxSoftBlockWindowCountValue)
            {
                HasHardBlockedProducerValue = true;
            }

            continue;
        }

        ProductionBlockerResolutionValue.BlockerKind = ProductionBlockerKindValue;
        ProductionBlockerResolutionValue.bCanAttemptRelief = false;
        ProductionBlockerResolutionValue.bCanRelocate = EconomyOrderValue.Origin != ECommandTaskOrigin::Opening;
        ProductionBlockerResolutionValue.RetryNotBeforeGameLoop =
            FrameValue.Observation->GetGameLoop() + BlockerReliefRetryDelayGameLoopsValue;
        ProductionBlockerResolutionValue.SoftBlockWindowCount = MaxSoftBlockWindowCountValue;
        ProductionBlockerResolutionValue.BlockerFingerprint = 0U;
        HasHardBlockedProducerValue = true;
        continue;
    }

    if (HasHardBlockedProducerValue)
    {
        OutDeferralReasonValue = ECommandOrderDeferralReason::NoValidPlacement;
        return false;
    }

    if (HasBusyProducerWithoutAddonValue || HasSoftBlockedProducerValue)
    {
        OutDeferralReasonValue = ECommandOrderDeferralReason::ProducerBusy;
        return false;
    }

    OutDeferralReasonValue = ECommandOrderDeferralReason::NoProducer;
    return false;
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
        case ABILITY_ID::RESEARCH_TERRANINFANTRYARMORLEVEL1:
            return true;
        default:
            return false;
    }
}

bool RequiresTechLabAddonForAbility(const ABILITY_ID AbilityIdValue)
{
    switch (AbilityIdValue)
    {
        case ABILITY_ID::TRAIN_MARAUDER:
        case ABILITY_ID::TRAIN_CYCLONE:
        case ABILITY_ID::TRAIN_SIEGETANK:
            return true;
        default:
            return false;
    }
}

void PrimeQueuedAddonFootprintRelief(
    const FFrameContext& FrameValue, const FGameStateDescriptor& GameStateDescriptorValue,
    const IBuildPlacementService& BuildPlacementServiceValue, const std::vector<Point2D>& ExpansionLocationsValue,
    const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
    const std::vector<size_t>& PlanningOrderIndicesValue, const Point2D& ProductionRallyPointValue,
    const std::unordered_map<FCommandTaskSignatureKey, FProductionBlockerResolution, FCommandTaskSignatureKeyHash>&
        ProductionBlockerResolutionsValue,
    FIntentBuffer& IntentBufferValue)
{
    if (FrameValue.Observation == nullptr)
    {
        return;
    }

    for (const size_t PlanningOrderIndexValue : PlanningOrderIndicesValue)
    {
        const FCommandOrderRecord EconomyOrderValue =
            CommandAuthoritySchedulingStateValue.GetOrderRecord(PlanningOrderIndexValue);
        if (EconomyOrderValue.SourceLayer != ECommandAuthorityLayer::EconomyAndProduction ||
            EconomyOrderValue.LifecycleState != EOrderLifecycleState::Queued)
        {
            continue;
        }

        UNIT_TYPEID ProducerUnitTypeIdValue = UNIT_TYPEID::INVALID;
        switch (EconomyOrderValue.AbilityId.ToType())
        {
            case ABILITY_ID::BUILD_REACTOR_BARRACKS:
            case ABILITY_ID::BUILD_TECHLAB_BARRACKS:
                ProducerUnitTypeIdValue = UNIT_TYPEID::TERRAN_BARRACKS;
                break;
            case ABILITY_ID::BUILD_REACTOR_FACTORY:
            case ABILITY_ID::BUILD_TECHLAB_FACTORY:
                ProducerUnitTypeIdValue = UNIT_TYPEID::TERRAN_FACTORY;
                break;
            case ABILITY_ID::BUILD_REACTOR_STARPORT:
            case ABILITY_ID::BUILD_TECHLAB_STARPORT:
                ProducerUnitTypeIdValue = UNIT_TYPEID::TERRAN_STARPORT;
                break;
            default:
                continue;
        }

        std::vector<const Unit*> CandidateProducerUnitsValue;
        if (EconomyOrderValue.PreferredProducerPlacementSlotId.IsValid())
        {
            FBuildPlacementSlot PreferredProducerPlacementSlotValue;
            const Unit* PreferredProducerUnitValue = nullptr;
            if (!TryResolvePreferredProducerUnit(FrameValue, GameStateDescriptorValue, BuildPlacementServiceValue,
                                                 ExpansionLocationsValue, ProducerUnitTypeIdValue,
                                                 EconomyOrderValue.PreferredProducerPlacementSlotId,
                                                 PreferredProducerPlacementSlotValue, PreferredProducerUnitValue) ||
                PreferredProducerUnitValue == nullptr)
            {
                continue;
            }

            CandidateProducerUnitsValue.push_back(PreferredProducerUnitValue);
        }
        else
        {
            const Units ProducerUnitsValue =
                FrameValue.Observation->GetUnits(Unit::Alliance::Self, IsUnit(ProducerUnitTypeIdValue));
            CandidateProducerUnitsValue.reserve(ProducerUnitsValue.size());
            for (const Unit* ProducerUnitValue : ProducerUnitsValue)
            {
                CandidateProducerUnitsValue.push_back(ProducerUnitValue);
            }
        }

        for (const Unit* CandidateProducerUnitValue : CandidateProducerUnitsValue)
        {
            if (CandidateProducerUnitValue == nullptr || CandidateProducerUnitValue->add_on_tag != NullTag ||
                CandidateProducerUnitValue->build_progress < 1.0f || !CandidateProducerUnitValue->orders.empty() ||
                IntentBufferValue.HasIntentForActor(CandidateProducerUnitValue->tag) ||
                HasActiveSchedulerOrderForActor(CommandAuthoritySchedulingStateValue, CandidateProducerUnitValue->tag))
            {
                continue;
            }

            if (ClassifyAddonFootprintBlocker(FrameValue, CandidateProducerUnitValue->tag,
                                              Point2D(CandidateProducerUnitValue->pos)) !=
                EProductionBlockerKind::FriendlyMovableUnit)
            {
                continue;
            }

            const FCommandTaskSignatureKey CommandTaskSignatureKeyValue =
                FCommandTaskSignatureKey::FromOrderRecord(EconomyOrderValue);
            const std::unordered_map<FCommandTaskSignatureKey, FProductionBlockerResolution,
                                     FCommandTaskSignatureKeyHash>::const_iterator
                FoundResolutionIteratorValue =
                    ProductionBlockerResolutionsValue.find(CommandTaskSignatureKeyValue);
            if (FoundResolutionIteratorValue != ProductionBlockerResolutionsValue.end())
            {
                const uint64_t BlockerFingerprintValue = BuildFriendlyMovableBlockerFingerprint(
                    FrameValue, CandidateProducerUnitValue->tag,
                    GetAddonFootprintCenter(Point2D(CandidateProducerUnitValue->pos)), Point2D(1.0f, 1.0f));
                const FProductionBlockerResolution& ProductionBlockerResolutionValue =
                    FoundResolutionIteratorValue->second;
                if (ProductionBlockerResolutionValue.BlockerKind == EProductionBlockerKind::FriendlyMovableUnit &&
                    ProductionBlockerResolutionValue.BlockerFingerprint == BlockerFingerprintValue &&
                    FrameValue.Observation->GetGameLoop() < ProductionBlockerResolutionValue.RetryNotBeforeGameLoop)
                {
                    continue;
                }
            }

            TryIssueFriendlyBlockerReliefForAddonFootprint(FrameValue, *CandidateProducerUnitValue,
                                                           ProductionRallyPointValue, IntentBufferValue);
        }
    }
}

bool HasOutstandingSupplyDepotDemand(const FGameStateDescriptor& GameStateDescriptorValue)
{
    return GameStateDescriptorValue.ProductionState.GetProjectedBuildingCount(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) <
           FTerranGoalRuleLibrary::DetermineDesiredSupplyDepotCount(GameStateDescriptorValue);
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
    ChildOrderValue.Origin = SourceOrderValue.Origin;
    ChildOrderValue.CommitmentClass = SourceOrderValue.CommitmentClass;
    ChildOrderValue.ExecutionGuarantee = SourceOrderValue.ExecutionGuarantee;
    ChildOrderValue.RetentionPolicy = SourceOrderValue.RetentionPolicy;
    ChildOrderValue.BlockedTaskWakeKind = SourceOrderValue.BlockedTaskWakeKind;
    ChildOrderValue.EffectivePriorityValue = SourceOrderValue.EffectivePriorityValue;
    ChildOrderValue.PriorityTier = SourceOrderValue.PriorityTier;
    ChildOrderValue.PreferredProducerPlacementSlotId = SourceOrderValue.PreferredProducerPlacementSlotId;
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

ABILITY_ID GetStructureAbilityIdForProducerUnitType(const UNIT_TYPEID ProducerUnitTypeIdValue)
{
    switch (ProducerUnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_BARRACKS:
            return ABILITY_ID::BUILD_BARRACKS;
        case UNIT_TYPEID::TERRAN_FACTORY:
            return ABILITY_ID::BUILD_FACTORY;
        case UNIT_TYPEID::TERRAN_STARPORT:
            return ABILITY_ID::BUILD_STARPORT;
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
            return ABILITY_ID::BUILD_COMMANDCENTER;
        default:
            return ABILITY_ID::INVALID;
    }
}

bool TryResolvePreferredProducerUnit(
    const FFrameContext& FrameValue, const FGameStateDescriptor& GameStateDescriptorValue,
    const IBuildPlacementService& BuildPlacementServiceValue, const std::vector<Point2D>& ExpansionLocationsValue,
    const UNIT_TYPEID ProducerUnitTypeIdValue, const FBuildPlacementSlotId& PreferredProducerPlacementSlotIdValue,
    FBuildPlacementSlot& OutProducerPlacementSlotValue, const Unit*& OutProducerUnitValue)
{
    OutProducerUnitValue = nullptr;
    if (FrameValue.Observation == nullptr || !PreferredProducerPlacementSlotIdValue.IsValid())
    {
        return false;
    }

    const ABILITY_ID ProducerStructureAbilityIdValue = GetStructureAbilityIdForProducerUnitType(ProducerUnitTypeIdValue);
    if (ProducerStructureAbilityIdValue == ABILITY_ID::INVALID)
    {
        return false;
    }

    const Point2D BaseLocationValue = Point2D(FrameValue.Observation->GetStartLocation());
    const FBuildPlacementContext BuildPlacementContextValue = CreateBuildPlacementContext(
        BaseLocationValue, ExpansionLocationsValue, GameStateDescriptorValue.RampWallDescriptor,
        GameStateDescriptorValue.MainBaseLayoutDescriptor);
    const std::vector<FBuildPlacementSlot> ProducerPlacementSlotsValue =
        BuildPlacementServiceValue.GetStructurePlacementSlots(GameStateDescriptorValue, ProducerStructureAbilityIdValue,
                                                              BuildPlacementContextValue);
    if (!TryFindPlacementSlotById(ProducerPlacementSlotsValue, PreferredProducerPlacementSlotIdValue,
                                  OutProducerPlacementSlotValue))
    {
#if _DEBUG
        if (FrameValue.GameLoop % 224U == 0U)
        {
            std::cout << "[RESOLVE_DIAG] SlotNotFound SlotType="
                      << static_cast<int>(PreferredProducerPlacementSlotIdValue.SlotType)
                      << " Ordinal=" << static_cast<int>(PreferredProducerPlacementSlotIdValue.Ordinal)
                      << " TotalSlots=" << ProducerPlacementSlotsValue.size()
                      << " WallValid=" << GameStateDescriptorValue.RampWallDescriptor.bIsValid
                      << " loop=" << FrameValue.GameLoop << std::endl;
        }
#endif
        return false;
    }

    OutProducerUnitValue = FindObservedStructureOccupyingPlacementSlot(*FrameValue.Observation,
                                                                       ProducerStructureAbilityIdValue,
                                                                       OutProducerPlacementSlotValue);
    if (OutProducerUnitValue == nullptr || OutProducerUnitValue->unit_type.ToType() != ProducerUnitTypeIdValue)
    {
#if _DEBUG
        if (FrameValue.GameLoop % 224U == 0U)
        {
            std::cout << "[RESOLVE_DIAG] UnitNotFound SlotBuildPoint=("
                      << OutProducerPlacementSlotValue.BuildPoint.x << ","
                      << OutProducerPlacementSlotValue.BuildPoint.y << ")"
                      << " FoundUnit=" << (OutProducerUnitValue != nullptr)
                      << " FoundType=" << (OutProducerUnitValue != nullptr
                             ? static_cast<int>(OutProducerUnitValue->unit_type.ToType()) : -1)
                      << " ExpectedType=" << static_cast<int>(ProducerUnitTypeIdValue)
                      << " loop=" << FrameValue.GameLoop;
            if (OutProducerUnitValue != nullptr)
            {
                std::cout << " unit_pos=(" << OutProducerUnitValue->pos.x << ","
                          << OutProducerUnitValue->pos.y << ")"
                          << " is_flying=" << OutProducerUnitValue->is_flying;
            }
            const Units AllBarracksValue = FrameValue.Observation->GetUnits(Unit::Alliance::Self,
                IsUnit(ProducerUnitTypeIdValue));
            const Units FlyingBarracksValue = FrameValue.Observation->GetUnits(Unit::Alliance::Self,
                IsUnit(UNIT_TYPEID::TERRAN_BARRACKSFLYING));
            std::cout << " GroundBarracksCount=" << AllBarracksValue.size()
                      << " FlyingBarracksCount=" << FlyingBarracksValue.size();
            for (const Unit* BarracksUnitValue : AllBarracksValue)
            {
                if (BarracksUnitValue != nullptr)
                {
                    std::cout << " BB=(" << BarracksUnitValue->pos.x << "," << BarracksUnitValue->pos.y
                              << " bp=" << BarracksUnitValue->build_progress
                              << " fly=" << BarracksUnitValue->is_flying << ")";
                }
            }
            std::cout << std::endl;
        }
#endif
        OutProducerUnitValue = nullptr;
        return false;
    }

    return true;
}

UNIT_TYPEID GetProtectedAddonProducerUnitTypeForStructureAbility(const ABILITY_ID StructureAbilityIdValue)
{
    switch (StructureAbilityIdValue)
    {
        case ABILITY_ID::BUILD_FACTORY:
            return UNIT_TYPEID::TERRAN_BARRACKS;
        case ABILITY_ID::BUILD_STARPORT:
            return UNIT_TYPEID::TERRAN_FACTORY;
        default:
            return UNIT_TYPEID::INVALID;
    }
}

bool DoesStructurePlacementOverlapObservedProtectedAddonLane(const FFrameContext& FrameValue,
                                                             const FCommandOrderRecord& EconomyOrderValue,
                                                             const FBuildPlacementSlot& BuildPlacementSlotValue)
{
    if (FrameValue.Observation == nullptr)
    {
        return false;
    }

    const UNIT_TYPEID ProtectedProducerUnitTypeIdValue =
        GetProtectedAddonProducerUnitTypeForStructureAbility(EconomyOrderValue.AbilityId);
    if (ProtectedProducerUnitTypeIdValue == UNIT_TYPEID::INVALID)
    {
        return false;
    }

    const Point2D CandidateFootprintHalfExtentsValue =
        GetStructureFootprintHalfExtentsForAbility(EconomyOrderValue.AbilityId);
    static const Point2D AddonFootprintHalfExtentsValue(1.0f, 1.0f);
    const Units ProtectedProducerUnitsValue =
        FrameValue.Observation->GetUnits(Unit::Alliance::Self, IsUnit(ProtectedProducerUnitTypeIdValue));
    for (const Unit* ProtectedProducerUnitValue : ProtectedProducerUnitsValue)
    {
        if (ProtectedProducerUnitValue == nullptr || !ProtectedProducerUnitValue->is_building ||
            ProtectedProducerUnitValue->is_flying)
        {
            continue;
        }

        if (!DoAxisAlignedFootprintsOverlap(BuildPlacementSlotValue.BuildPoint,
                                            CandidateFootprintHalfExtentsValue,
                                            GetAddonFootprintCenter(Point2D(ProtectedProducerUnitValue->pos)),
                                            AddonFootprintHalfExtentsValue))
        {
            continue;
        }

        return true;
    }

    return false;
}

bool DoesMandatoryOpeningAddonReserveProducer(
    const FFrameContext& FrameValue, const FGameStateDescriptor& GameStateDescriptorValue,
    const IBuildPlacementService& BuildPlacementServiceValue, const std::vector<Point2D>& ExpansionLocationsValue,
    const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue, const Unit& ProducerUnitValue,
    const uint32_t IgnoredOrderIdValue)
{
    static constexpr uint32_t MaxAddonReservationDeferralCountValue = 48U;

    const auto DoesAddonTaskRequireExclusiveProducerReservationValue =
        [](const ECommandTaskOrigin TaskOriginValue, const ECommandCommitmentClass CommitmentClassValue,
           const ECommandTaskExecutionGuarantee ExecutionGuaranteeValue) -> bool
    {
        if (ExecutionGuaranteeValue == ECommandTaskExecutionGuarantee::MustExecute ||
            TaskOriginValue == ECommandTaskOrigin::Opening)
        {
            return true;
        }

        switch (CommitmentClassValue)
        {
            case ECommandCommitmentClass::MandatoryOpening:
            case ECommandCommitmentClass::MandatoryRecovery:
                return true;
            case ECommandCommitmentClass::FlexibleMacro:
            case ECommandCommitmentClass::Opportunistic:
            default:
                return false;
        }
    };

    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.OrderIds[OrderIndexValue] == IgnoredOrderIdValue ||
            IsTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]) ||
            CommandAuthoritySchedulingStateValue.TaskTypes[OrderIndexValue] != ECommandTaskType::AddOn ||
            CommandAuthoritySchedulingStateValue.PreferredProducerPlacementSlotIdTypes[OrderIndexValue] ==
                EBuildPlacementSlotType::Unknown)
        {
            continue;
        }

        if (!DoesAddonTaskRequireExclusiveProducerReservationValue(
                CommandAuthoritySchedulingStateValue.TaskOrigins[OrderIndexValue],
                CommandAuthoritySchedulingStateValue.CommitmentClasses[OrderIndexValue],
                CommandAuthoritySchedulingStateValue.ExecutionGuarantees[OrderIndexValue]))
        {
            continue;
        }

        if (CommandAuthoritySchedulingStateValue.ConsecutiveDeferralCounts[OrderIndexValue] > MaxAddonReservationDeferralCountValue &&
            CommandAuthoritySchedulingStateValue.LastDeferralReasons[OrderIndexValue] ==
                ECommandOrderDeferralReason::InsufficientResources)
        {
            continue;
        }

        FBuildPlacementSlotId PreferredProducerPlacementSlotIdValue;
        PreferredProducerPlacementSlotIdValue.SlotType =
            CommandAuthoritySchedulingStateValue.PreferredProducerPlacementSlotIdTypes[OrderIndexValue];
        PreferredProducerPlacementSlotIdValue.Ordinal =
            CommandAuthoritySchedulingStateValue.PreferredProducerPlacementSlotIdOrdinals[OrderIndexValue];
        if (!PreferredProducerPlacementSlotIdValue.IsValid())
        {
            continue;
        }

        FBuildPlacementSlot ReservedProducerPlacementSlotValue;
        const Unit* ReservedProducerUnitValue = nullptr;
        if (!TryResolvePreferredProducerUnit(FrameValue, GameStateDescriptorValue, BuildPlacementServiceValue,
                                             ExpansionLocationsValue,
                                             CommandAuthoritySchedulingStateValue.ProducerUnitTypeIds[OrderIndexValue],
                                             PreferredProducerPlacementSlotIdValue,
                                             ReservedProducerPlacementSlotValue, ReservedProducerUnitValue))
        {
            continue;
        }

        if (ReservedProducerUnitValue->tag == ProducerUnitValue.tag)
        {
            return true;
        }
    }

    const std::array<const FBlockedTaskRingBuffer*, 2U> BlockedTaskBuffersValue =
    {
        &CommandAuthoritySchedulingStateValue.BlockedStrategicTasks,
        &CommandAuthoritySchedulingStateValue.BlockedPlanningTasks,
    };
    for (const FBlockedTaskRingBuffer* BlockedTaskRingBufferPtrValue : BlockedTaskBuffersValue)
    {
        if (BlockedTaskRingBufferPtrValue == nullptr)
        {
            continue;
        }

        const size_t BlockedTaskCountValue = BlockedTaskRingBufferPtrValue->GetCount();
        for (size_t OrderedIndexValue = 0U; OrderedIndexValue < BlockedTaskCountValue; ++OrderedIndexValue)
        {
            const FBlockedTaskRecord* BlockedTaskRecordPtrValue =
                BlockedTaskRingBufferPtrValue->GetRecordAtOrderedIndex(OrderedIndexValue);
            if (BlockedTaskRecordPtrValue == nullptr || BlockedTaskRecordPtrValue->TaskType != ECommandTaskType::AddOn ||
                !BlockedTaskRecordPtrValue->PreferredProducerPlacementSlotId.IsValid())
            {
                continue;
            }

            if (!DoesAddonTaskRequireExclusiveProducerReservationValue(BlockedTaskRecordPtrValue->Origin,
                                                                       BlockedTaskRecordPtrValue->CommitmentClass,
                                                                       BlockedTaskRecordPtrValue->ExecutionGuarantee))
            {
                continue;
            }

            if (BlockedTaskRecordPtrValue->RetryCount > MaxAddonReservationDeferralCountValue &&
                BlockedTaskRecordPtrValue->BlockingReason == ECommandOrderDeferralReason::InsufficientResources)
            {
                continue;
            }

            FBuildPlacementSlot ReservedProducerPlacementSlotValue;
            const Unit* ReservedProducerUnitValue = nullptr;
            if (!TryResolvePreferredProducerUnit(FrameValue, GameStateDescriptorValue, BuildPlacementServiceValue,
                                                 ExpansionLocationsValue,
                                                 BlockedTaskRecordPtrValue->ProducerUnitTypeId,
                                                 BlockedTaskRecordPtrValue->PreferredProducerPlacementSlotId,
                                                 ReservedProducerPlacementSlotValue, ReservedProducerUnitValue))
            {
                continue;
            }

            if (ReservedProducerUnitValue->tag == ProducerUnitValue.tag)
            {
                return true;
            }
        }
    }

    return false;
}

uint32_t CountPendingAddonOrdersForProducerType(
    const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
    const UNIT_TYPEID ProducerUnitTypeIdValue)
{
    uint32_t PendingAddonOrderCountValue = 0U;

    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (IsTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        if (CommandAuthoritySchedulingStateValue.TaskTypes[OrderIndexValue] != ECommandTaskType::AddOn)
        {
            continue;
        }

        if (CommandAuthoritySchedulingStateValue.ProducerUnitTypeIds[OrderIndexValue] == ProducerUnitTypeIdValue)
        {
            ++PendingAddonOrderCountValue;
        }
    }

    const std::array<const FBlockedTaskRingBuffer*, 2U> BlockedTaskBuffersValue =
    {
        &CommandAuthoritySchedulingStateValue.BlockedStrategicTasks,
        &CommandAuthoritySchedulingStateValue.BlockedPlanningTasks,
    };
    for (const FBlockedTaskRingBuffer* BlockedTaskRingBufferPtrValue : BlockedTaskBuffersValue)
    {
        if (BlockedTaskRingBufferPtrValue == nullptr)
        {
            continue;
        }

        const size_t BlockedTaskCountValue = BlockedTaskRingBufferPtrValue->GetCount();
        for (size_t OrderedIndexValue = 0U; OrderedIndexValue < BlockedTaskCountValue; ++OrderedIndexValue)
        {
            const FBlockedTaskRecord* BlockedTaskRecordPtrValue =
                BlockedTaskRingBufferPtrValue->GetRecordAtOrderedIndex(OrderedIndexValue);
            if (BlockedTaskRecordPtrValue == nullptr ||
                BlockedTaskRecordPtrValue->TaskType != ECommandTaskType::AddOn)
            {
                continue;
            }

            if (BlockedTaskRecordPtrValue->ProducerUnitTypeId == ProducerUnitTypeIdValue)
            {
                ++PendingAddonOrderCountValue;
            }
        }
    }

    return PendingAddonOrderCountValue;
}

bool IsSupplyPressureActiveForDepotFallback(const FGameStateDescriptor& GameStateDescriptorValue)
{
    return GameStateDescriptorValue.EconomyState.ProjectedAvailableSupplyByHorizon[ShortForecastHorizonIndexValue] <=
               2U ||
           GameStateDescriptorValue.CommitmentLedger.GetProjectedDiscretionarySupply(ShortForecastHorizonIndexValue) <=
               2U;
}

float GetPlacementSlotOccupancyRadiusSquared(const FBuildPlacementSlot& BuildPlacementSlotValue)
{
    switch (BuildPlacementSlotValue.SlotId.SlotType)
    {
        case EBuildPlacementSlotType::MainRampDepotLeft:
        case EBuildPlacementSlotType::MainRampBarracksWithAddon:
        case EBuildPlacementSlotType::MainRampDepotRight:
        case EBuildPlacementSlotType::NaturalEntranceDepotLeft:
        case EBuildPlacementSlotType::NaturalEntranceDepotRight:
        case EBuildPlacementSlotType::NaturalApproachDepot:
        case EBuildPlacementSlotType::MainSupportDepot:
        case EBuildPlacementSlotType::MainPeripheralDepot:
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
                                     const Point2D& BlockerReliefTargetPointValue,
                                     FIntentBuffer& IntentBufferValue,
                                     FBuildPlacementSlot& OutBuildPlacementSlotValue,
                                     bool& OutHasFriendlyMovableBlockerValue)
{
    OutHasFriendlyMovableBlockerValue = false;
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

    if (DoesStructurePlacementOverlapObservedProtectedAddonLane(FrameValue, EconomyOrderValue,
                                                                NormalizedBuildPlacementSlotValue))
    {
        return false;
    }

    if (DoesStructurePlacementHaveFriendlyMovableBlocker(FrameValue, EconomyOrderValue, WorkerUnitValue,
                                                         NormalizedBuildPlacementSlotValue))
    {
        OutHasFriendlyMovableBlockerValue = true;
        TryIssueFriendlyBlockerReliefForStructurePlacement(FrameValue, EconomyOrderValue, WorkerUnitValue,
                                                           NormalizedBuildPlacementSlotValue,
                                                           BlockerReliefTargetPointValue,
                                                           IntentBufferValue);
        return false;
    }

    if (!FrameValue.Query->Placement(EconomyOrderValue.AbilityId, ClampedBuildPointValue, &WorkerUnitValue))
    {
        return false;
    }

    OutBuildPlacementSlotValue = NormalizedBuildPlacementSlotValue;
    return true;
}

bool TrySelectFallbackSupplyDepotPlacementSlot(const FFrameContext& FrameValue, const Unit& WorkerUnitValue,
                                               const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                               const FCommandOrderRecord& EconomyOrderValue,
                                               const FBuildPlacementContext& BuildPlacementContextValue,
                                               const Point2D& BlockerReliefTargetPointValue,
                                               FIntentBuffer& IntentBufferValue,
                                               FBuildPlacementSlot& OutBuildPlacementSlotValue)
{
    if (FrameValue.GameInfo == nullptr || FrameValue.Query == nullptr)
    {
        return false;
    }

    const Point2D ForwardDirectionValue = GetNormalizedDirection(
        BuildPlacementContextValue.HasNaturalLocation()
            ? (BuildPlacementContextValue.NaturalLocation - BuildPlacementContextValue.BaseLocation)
            : Point2D(1.0f, 0.0f));
    const Point2D LateralDirectionValue = GetCounterClockwiseLateralDirection(ForwardDirectionValue);
    const Point2D BaseLocationValue = BuildPlacementContextValue.BaseLocation;
    const Point2D NaturalLocationValue =
        BuildPlacementContextValue.HasNaturalLocation()
            ? BuildPlacementContextValue.NaturalLocation
            : Point2D(BaseLocationValue.x + (ForwardDirectionValue.x * 20.0f),
                      BaseLocationValue.y + (ForwardDirectionValue.y * 20.0f));
    const std::array<Point2D, 8U> CandidateBuildPointsValue =
    {
        Point2D(NaturalLocationValue.x + (LateralDirectionValue.x * 10.0f),
                NaturalLocationValue.y + (LateralDirectionValue.y * 10.0f)),
        Point2D(NaturalLocationValue.x - (LateralDirectionValue.x * 10.0f),
                NaturalLocationValue.y - (LateralDirectionValue.y * 10.0f)),
        Point2D(NaturalLocationValue.x + (ForwardDirectionValue.x * 6.0f) + (LateralDirectionValue.x * 14.0f),
                NaturalLocationValue.y + (ForwardDirectionValue.y * 6.0f) + (LateralDirectionValue.y * 14.0f)),
        Point2D(NaturalLocationValue.x + (ForwardDirectionValue.x * 6.0f) - (LateralDirectionValue.x * 14.0f),
                NaturalLocationValue.y + (ForwardDirectionValue.y * 6.0f) - (LateralDirectionValue.y * 14.0f)),
        Point2D(BaseLocationValue.x + (LateralDirectionValue.x * 22.0f) + (ForwardDirectionValue.x * 18.0f),
                BaseLocationValue.y + (LateralDirectionValue.y * 22.0f) + (ForwardDirectionValue.y * 18.0f)),
        Point2D(BaseLocationValue.x - (LateralDirectionValue.x * 22.0f) + (ForwardDirectionValue.x * 18.0f),
                BaseLocationValue.y - (LateralDirectionValue.y * 22.0f) + (ForwardDirectionValue.y * 18.0f)),
        Point2D(BaseLocationValue.x + (LateralDirectionValue.x * 26.0f),
                BaseLocationValue.y + (LateralDirectionValue.y * 26.0f)),
        Point2D(BaseLocationValue.x - (LateralDirectionValue.x * 26.0f),
                BaseLocationValue.y - (LateralDirectionValue.y * 26.0f)),
    };

    for (const Point2D& CandidateBuildPointValue : CandidateBuildPointsValue)
    {
        FBuildPlacementSlot FallbackPlacementSlotValue;
        FallbackPlacementSlotValue.SlotId.Reset();
        FallbackPlacementSlotValue.FootprintPolicy = EBuildPlacementFootprintPolicy::StructureOnly;
        FallbackPlacementSlotValue.BuildPoint = CandidateBuildPointValue;
        bool HasFriendlyMovableBlockerValue = false;
        if (TrySelectPlacementSlotCandidate(FrameValue, CommandAuthoritySchedulingStateValue, EconomyOrderValue,
                                            WorkerUnitValue, FallbackPlacementSlotValue,
                                            BlockerReliefTargetPointValue, IntentBufferValue,
                                            OutBuildPlacementSlotValue, HasFriendlyMovableBlockerValue))
        {
            return true;
        }
    }

    return false;
}

bool TrySelectStructurePlacementSlot(const FFrameContext& FrameValue,
                                     const FGameStateDescriptor& GameStateDescriptorValue,
                                     const IBuildPlacementService& BuildPlacementServiceValue,
                                     const std::vector<Point2D>& ExpansionLocationsValue,
                                     const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                     const FCommandOrderRecord& EconomyOrderValue, const Unit& WorkerUnitValue,
                                     const Point2D& BlockerReliefTargetPointValue,
                                     FIntentBuffer& IntentBufferValue,
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
    bool HasFriendlyMovableBlockerValue = false;

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
                                             BlockerReliefTargetPointValue,
                                             IntentBufferValue, OutBuildPlacementSlotValue,
                                             HasFriendlyMovableBlockerValue))
        {
            OutDeferralReasonValue = HasFriendlyMovableBlockerValue ? ECommandOrderDeferralReason::ProducerBusy
                                                                   : ECommandOrderDeferralReason::ReservedSlotOccupied;
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
                                             BlockerReliefTargetPointValue,
                                             IntentBufferValue, OutBuildPlacementSlotValue,
                                             HasFriendlyMovableBlockerValue))
        {
            OutDeferralReasonValue = HasFriendlyMovableBlockerValue ? ECommandOrderDeferralReason::ProducerBusy
                                                                   : ECommandOrderDeferralReason::ReservedSlotOccupied;
            return false;
        }

        return true;
    }

    bool HasIssuedFriendlyBlockerReliefValue = false;
    std::vector<FBuildPlacementSlotId> AttemptedPlacementSlotIdsValue;
    bool HasPreferredPlacementSlotCandidatesValue = false;
    const bool IsExactMustExecutePlacementValue =
        EconomyOrderValue.ExecutionGuarantee == ECommandTaskExecutionGuarantee::MustExecute &&
        (EconomyOrderValue.PreferredPlacementSlotId.IsValid() ||
         EconomyOrderValue.PreferredPlacementSlotType != EBuildPlacementSlotType::Unknown);
    if (EconomyOrderValue.PreferredPlacementSlotType != EBuildPlacementSlotType::Unknown)
    {
        for (const FBuildPlacementSlot& BuildPlacementSlotValue : BuildPlacementSlotsValue)
        {
            if (BuildPlacementSlotValue.SlotId.SlotType != EconomyOrderValue.PreferredPlacementSlotType)
            {
                continue;
            }

            HasPreferredPlacementSlotCandidatesValue = true;
            AttemptedPlacementSlotIdsValue.push_back(BuildPlacementSlotValue.SlotId);
            if (TrySelectPlacementSlotCandidate(FrameValue, CommandAuthoritySchedulingStateValue,
                                                EconomyOrderValue, WorkerUnitValue, BuildPlacementSlotValue,
                                                BlockerReliefTargetPointValue,
                                                IntentBufferValue, OutBuildPlacementSlotValue,
                                                HasFriendlyMovableBlockerValue))
            {
                return true;
            }

            HasIssuedFriendlyBlockerReliefValue =
                HasIssuedFriendlyBlockerReliefValue || HasFriendlyMovableBlockerValue;
        }
    }

    if (IsExactMustExecutePlacementValue &&
        EconomyOrderValue.PreferredPlacementSlotType != EBuildPlacementSlotType::Unknown)
    {
        if (HasIssuedFriendlyBlockerReliefValue)
        {
            OutDeferralReasonValue = ECommandOrderDeferralReason::ProducerBusy;
            return false;
        }

        OutDeferralReasonValue = HasPreferredPlacementSlotCandidatesValue
                                     ? ECommandOrderDeferralReason::ReservedSlotOccupied
                                     : ECommandOrderDeferralReason::ReservedSlotInvalidated;
        return false;
    }

    for (const FBuildPlacementSlot& BuildPlacementSlotValue : BuildPlacementSlotsValue)
    {
        if (HasPlacementSlotBeenAttempted(AttemptedPlacementSlotIdsValue, BuildPlacementSlotValue.SlotId))
        {
            continue;
        }

        if (TrySelectPlacementSlotCandidate(FrameValue, CommandAuthoritySchedulingStateValue,
                                            EconomyOrderValue, WorkerUnitValue, BuildPlacementSlotValue,
                                            BlockerReliefTargetPointValue,
                                            IntentBufferValue, OutBuildPlacementSlotValue,
                                            HasFriendlyMovableBlockerValue))
        {
            return true;
        }

        HasIssuedFriendlyBlockerReliefValue =
            HasIssuedFriendlyBlockerReliefValue || HasFriendlyMovableBlockerValue;
    }

    if (EconomyOrderValue.AbilityId == ABILITY_ID::BUILD_SUPPLYDEPOT && !HasIssuedFriendlyBlockerReliefValue &&
        IsSupplyPressureActiveForDepotFallback(GameStateDescriptorValue) &&
        TrySelectFallbackSupplyDepotPlacementSlot(FrameValue, WorkerUnitValue, CommandAuthoritySchedulingStateValue,
                                                  EconomyOrderValue, BuildPlacementContextValue,
                                                  BlockerReliefTargetPointValue, IntentBufferValue,
                                                  OutBuildPlacementSlotValue))
    {
        return true;
    }

    if (HasIssuedFriendlyBlockerReliefValue)
    {
        OutDeferralReasonValue = ECommandOrderDeferralReason::ProducerBusy;
        return false;
    }

    OutDeferralReasonValue = HasPreferredPlacementSlotCandidatesValue ? ECommandOrderDeferralReason::ReservedSlotOccupied
                                                                      : ECommandOrderDeferralReason::NoValidPlacement;
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

            {
                const bool bTerrainSupportedValue =
                    DoesAddonFootprintSupportTerrain(*FrameValue.GameInfo, BuildPlacementSlotValue.BuildPoint);
                const bool bStructureAvoidedValue =
                    DoesAddonFootprintAvoidObservedStructures(*FrameValue.Observation,
                                                              BuildPlacementSlotValue.BuildPoint);
                return bTerrainSupportedValue && bStructureAvoidedValue;
            }
        default:
            return false;
    }
}

bool TryReserveResources(FBuildPlanningState& BuildPlanningStateValue,
                         const ECommandCommitmentClass CommandCommitmentClassValue,
                         const uint32_t MineralsCostValue, const uint32_t VespeneCostValue,
                         const uint32_t SupplyCostValue)
{
    return BuildPlanningStateValue.TryReserveResources(CommandCommitmentClassValue, MineralsCostValue,
                                                       VespeneCostValue, SupplyCostValue);
}

void ReleaseReservedResources(FBuildPlanningState& BuildPlanningStateValue,
                              const ECommandCommitmentClass CommandCommitmentClassValue,
                              const uint32_t MineralsCostValue, const uint32_t VespeneCostValue,
                              const uint32_t SupplyCostValue)
{
    BuildPlanningStateValue.ReleaseReservedResources(CommandCommitmentClassValue, MineralsCostValue,
                                                     VespeneCostValue, SupplyCostValue);
}

bool TryReserveStructureCost(FBuildPlanningState& BuildPlanningStateValue,
                             const ECommandCommitmentClass CommandCommitmentClassValue,
                             const UNIT_TYPEID StructureTypeIdValue)
{
    const FBuildingCostData& BuildingCostDataValue = TERRAN_ECONOMIC_DATA.GetBuildingCostData(StructureTypeIdValue);
    return TryReserveResources(BuildPlanningStateValue, CommandCommitmentClassValue,
                               BuildingCostDataValue.CostData.Minerals,
                               BuildingCostDataValue.CostData.Vespine, 0U);
}

bool TryReserveUnitCost(FBuildPlanningState& BuildPlanningStateValue,
                        const ECommandCommitmentClass CommandCommitmentClassValue,
                        const UNIT_TYPEID UnitTypeIdValue)
{
    const FUnitCostData& UnitCostDataValue = TERRAN_ECONOMIC_DATA.GetUnitCostData(UnitTypeIdValue);
    return TryReserveResources(BuildPlanningStateValue, CommandCommitmentClassValue,
                               UnitCostDataValue.CostData.Minerals,
                               UnitCostDataValue.CostData.Vespine, UnitCostDataValue.CostData.Supply);
}

bool TryReserveUpgradeCost(FBuildPlanningState& BuildPlanningStateValue,
                           const ECommandCommitmentClass CommandCommitmentClassValue,
                           const ABILITY_ID UpgradeAbilityIdValue)
{
    const FUpgradeCostData& UpgradeCostDataValue = TERRAN_ECONOMIC_DATA.GetUpgradeCostData(UpgradeAbilityIdValue);
    return TryReserveResources(BuildPlanningStateValue, CommandCommitmentClassValue,
                               UpgradeCostDataValue.CostData.Minerals,
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
    const Point2D BaseLocationValue = Point2D(FrameValue.Observation->GetStartLocation());
    const FBuildPlacementContext BuildPlacementContextValue = CreateBuildPlacementContext(
        BaseLocationValue, ExpansionLocationsValue, GameStateDescriptorValue.RampWallDescriptor,
        GameStateDescriptorValue.MainBaseLayoutDescriptor);
    const Point2D ProductionRallyPointValue =
        BuildPlacementServiceValue.GetProductionRallyPoint(GameStateDescriptorValue, BuildPlacementContextValue);
    const Point2D ProductionClearancePointValue =
        (BuildPlacementContextValue.MainBaseLayoutDescriptor.ProductionClearanceAnchorPoint.x != 0.0f ||
         BuildPlacementContextValue.MainBaseLayoutDescriptor.ProductionClearanceAnchorPoint.y != 0.0f)
            ? BuildPlacementContextValue.MainBaseLayoutDescriptor.ProductionClearanceAnchorPoint
            : ProductionRallyPointValue;
    PrimeQueuedAddonFootprintRelief(FrameValue, GameStateDescriptorValue, BuildPlacementServiceValue,
                                    ExpansionLocationsValue, CommandAuthoritySchedulingStateValue,
                                    PlanningOrderIndicesValue, ProductionClearancePointValue,
                                    ProductionBlockerResolutions, IntentBufferValue);

    bool bBarracksDispatchedThisCycleValue = false;

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
            GameStateDescriptorValue.OpeningPlanExecutionState.ResetPlacementFailureCount(EconomyOrderValue.PlanStepId);
            CommandAuthoritySchedulingStateValue.SetOrderDeferralState(
                EconomyOrderValue.OrderId, ECommandOrderDeferralReason::TargetAlreadySatisfied, CurrentStepValue,
                CurrentGameLoopValue);
            CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(EconomyOrderValue.OrderId,
                                                                       EOrderLifecycleState::Completed);
            continue;
        }

        const bool IsUnitProductionOrderValue = IsUnitProductionAbility(EconomyOrderValue.AbilityId);

        // Expire stale morph child orders. SC2 silently ignores MORPH_ORBITALCOMMAND
        // if the CC is training when the command fires. The child order stays in
        // Dispatched state indefinitely. Detect and expire these so the parent economy
        // order can retry the morph when the CC is truly idle.
        if (EconomyOrderValue.AbilityId == ABILITY_ID::MORPH_ORBITALCOMMAND)
        {
            static constexpr uint32_t MaxMorphDispatchStaleGameLoopsValue = 224U;
            for (size_t ChildOrderIndexValue = 0U;
                 ChildOrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
                 ++ChildOrderIndexValue)
            {
                if (CommandAuthoritySchedulingStateValue.ParentOrderIds[ChildOrderIndexValue] !=
                        static_cast<int>(EconomyOrderValue.OrderId) ||
                    CommandAuthoritySchedulingStateValue.SourceLayers[ChildOrderIndexValue] !=
                        ECommandAuthorityLayer::UnitExecution ||
                    IsOrderTerminal(CommandAuthoritySchedulingStateValue.LifecycleStates[ChildOrderIndexValue]))
                {
                    continue;
                }

                if (CommandAuthoritySchedulingStateValue.LifecycleStates[ChildOrderIndexValue] ==
                        EOrderLifecycleState::Dispatched &&
                    CommandAuthoritySchedulingStateValue.DispatchGameLoops[ChildOrderIndexValue] > 0U &&
                    CurrentGameLoopValue - CommandAuthoritySchedulingStateValue.DispatchGameLoops[ChildOrderIndexValue] >
                        MaxMorphDispatchStaleGameLoopsValue)
                {
                    CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(
                        CommandAuthoritySchedulingStateValue.OrderIds[ChildOrderIndexValue],
                        EOrderLifecycleState::Completed);
                }
            }
        }

        const uint32_t ActiveUnitExecutionChildCountValue =
            CountActiveUnitExecutionChildrenForParent(CommandAuthoritySchedulingStateValue, EconomyOrderValue.OrderId);
        if (!IsUnitProductionOrderValue &&
            HasActiveUnitExecutionChild(CommandAuthoritySchedulingStateValue, EconomyOrderValue.OrderId))
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
                    ApplyOpeningWallFallbackForPlacementDeferral(GameStateDescriptorValue, EconomyOrderValue,
                                                                 ECommandOrderDeferralReason::ReservedSlotInvalidated);
                    CommandAuthoritySchedulingStateValue.SetOrderDeferralState(
                        EconomyOrderValue.OrderId, ECommandOrderDeferralReason::ReservedSlotInvalidated,
                        CurrentStepValue, CurrentGameLoopValue);
                    break;
                case EReservedPlacementSlotState::Occupied:
                    ApplyOpeningWallFallbackForPlacementDeferral(GameStateDescriptorValue, EconomyOrderValue,
                                                                 ECommandOrderDeferralReason::ReservedSlotOccupied);
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

        const bool IsUnitProductionProjectedCheckValue = IsUnitProductionAbility(EconomyOrderValue.AbilityId) &&
            EconomyOrderValue.ResultUnitTypeId != UNIT_TYPEID::INVALID;
        const uint32_t ProjectedCountValue = IsUnitProductionProjectedCheckValue
            ? GameStateDescriptorValue.ProductionState.GetObservedAndInProgressUnitCount(
                  EconomyOrderValue.ResultUnitTypeId) +
                  CountPendingIntentsForAbility(IntentBufferValue, EconomyOrderValue.AbilityId)
            : GetProjectedCountForOrderExcludingSelf(GameStateDescriptorValue, CommandAuthoritySchedulingStateValue,
                                                     EconomyOrderValue) +
                  CountPendingIntentsForAbility(IntentBufferValue, EconomyOrderValue.AbilityId);
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
                                                     *WorkerUnitValue, ProductionClearancePointValue, IntentBufferValue,
                                                     SelectedBuildPlacementSlotValue,
                                                     DeferralReasonValue))
                {
                    break;
                }

                if (!TryReserveStructureCost(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue.CommitmentClass,
                                             EconomyOrderValue.ResultUnitTypeId))
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
                if (EconomyOrderValue.AbilityId == ABILITY_ID::BUILD_BARRACKS)
                {
                    bBarracksDispatchedThisCycleValue = true;
                }
                break;
            }
            case ABILITY_ID::BUILD_REFINERY:
            {
                if (FrameValue.Query == nullptr)
                {
                    DeferralReasonValue = ECommandOrderDeferralReason::NoValidPlacement;
                    break;
                }

                // Guard: Do not build a refinery until at least one barracks exists, is under construction,
                // or was dispatched in this economy cycle. The cycle-local flag avoids a timing gap where
                // the barracks SCV is en route but the building does not yet exist as a game unit.
                {
                    const Units ExistingBarracksUnitsValue =
                        FrameValue.Observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_BARRACKS));
                    if (ExistingBarracksUnitsValue.empty() && !bBarracksDispatchedThisCycleValue)
                    {
                        DeferralReasonValue = ECommandOrderDeferralReason::AwaitingObservedCompletion;
                        break;
                    }
                }

                if (!TryReserveStructureCost(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue.CommitmentClass,
                                             UNIT_TYPEID::TERRAN_REFINERY))
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
                    ReleaseReservedResources(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue.CommitmentClass,
                                             RefineryCostDataValue.CostData.Minerals,
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

                if (!TryReserveStructureCost(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue.CommitmentClass,
                                             UNIT_TYPEID::TERRAN_COMMANDCENTER))
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
                if (!TryReserveResources(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue.CommitmentClass, 150U,
                                         0U, 0U))
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
                        HasActiveSchedulerOrderForActorExcludingOrder(CommandAuthoritySchedulingStateValue,
                                                                      CandidateTownHallUnitValue->tag,
                                                                      EconomyOrderValue.OrderId))
                    {
                        HasBusyCommandCenterValue = true;
                        continue;
                    }

                    TownHallUnitValue = CandidateTownHallUnitValue;
                    break;
                }

                if (TownHallUnitValue == nullptr)
                {
                    ReleaseReservedResources(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue.CommitmentClass,
                                             150U, 0U, 0U);
                    DeferralReasonValue = HasBusyCommandCenterValue ? ECommandOrderDeferralReason::ProducerBusy
                                                                    : ECommandOrderDeferralReason::NoProducer;
                    break;
                }

                // Dispatch orbital morph directly as an intent. Unlike structure
                // builds, morphs do not require worker dispatch or placement — they
                // are immediate ability commands on the CC. Using a direct intent
                // instead of a UnitExecution order prevents the race where SCV
                // training orders compete with the morph within the same frame.
                IntentBufferValue.Add(FUnitIntent::CreateNoTarget(
                    TownHallUnitValue->tag, ABILITY_ID::MORPH_ORBITALCOMMAND,
                    EconomyOrderValue.BasePriorityValue, EIntentDomain::StructureBuild));

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
                if (!TryReserveResources(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue.CommitmentClass,
                                         MineralsCostValue, VespeneCostValue, 0U))
                {
                    DeferralReasonValue = ECommandOrderDeferralReason::InsufficientResources;
                    break;
                }

                const Unit* BarracksUnitValue = nullptr;
                if (!TrySelectAddonProducerUnit(FrameValue, GameStateDescriptorValue, BuildPlacementServiceValue,
                                                ExpansionLocationsValue, CommandAuthoritySchedulingStateValue,
                                                EconomyOrderValue, UNIT_TYPEID::TERRAN_BARRACKS,
                                                ProductionClearancePointValue, ProductionBlockerResolutions,
                                                IntentBufferValue, BarracksUnitValue, DeferralReasonValue))
                {
                    ReleaseReservedResources(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue.CommitmentClass,
                                             MineralsCostValue,
                                             VespeneCostValue, 0U);
                    break;
                }

                // Addon builds require an idle producer. SC2 silently ignores addon
                // commands sent to a barracks that is training. Defer until idle.
                // The mandatory opening reservation prevents new marines from being
                // dispatched to this barracks, so the in-progress marine will finish
                // and the barracks will become idle.
                if (!BarracksUnitValue->orders.empty())
                {
                    ReleaseReservedResources(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue.CommitmentClass,
                                             MineralsCostValue, VespeneCostValue, 0U);
                    DeferralReasonValue = ECommandOrderDeferralReason::ProducerBusy;
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
                if (!TryReserveResources(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue.CommitmentClass,
                                         MineralsCostValue, VespeneCostValue, 0U))
                {
                    DeferralReasonValue = ECommandOrderDeferralReason::InsufficientResources;
                    break;
                }

                const Unit* FactoryUnitValue = nullptr;
                if (!TrySelectAddonProducerUnit(FrameValue, GameStateDescriptorValue, BuildPlacementServiceValue,
                                                ExpansionLocationsValue, CommandAuthoritySchedulingStateValue,
                                                EconomyOrderValue, UNIT_TYPEID::TERRAN_FACTORY,
                                                ProductionClearancePointValue, ProductionBlockerResolutions,
                                                IntentBufferValue, FactoryUnitValue, DeferralReasonValue))
                {
                    ReleaseReservedResources(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue.CommitmentClass,
                                             MineralsCostValue,
                                             VespeneCostValue, 0U);
                    break;
                }

                if (!FactoryUnitValue->orders.empty())
                {
                    ReleaseReservedResources(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue.CommitmentClass,
                                             MineralsCostValue, VespeneCostValue, 0U);
                    DeferralReasonValue = ECommandOrderDeferralReason::ProducerBusy;
                    break;
                }

                {
                    UnitExecutionOrderValue = FCommandOrderRecord::CreateNoTarget(
                        ECommandAuthorityLayer::UnitExecution, FactoryUnitValue->tag, EconomyOrderValue.AbilityId,
                        EconomyOrderValue.BasePriorityValue, EIntentDomain::StructureBuild,
                        GameStateDescriptorValue.CurrentGameLoop, 0U, EconomyOrderValue.OrderId);
                    CopyTaskMetadataToChildOrder(EconomyOrderValue, UnitExecutionOrderValue);
                    CreatedOrderValue = true;
                }
                break;
            }
            case ABILITY_ID::BUILD_REACTOR_STARPORT:
            case ABILITY_ID::BUILD_TECHLAB_STARPORT:
            {
                const uint32_t MineralsCostValue = 50U;
                const uint32_t VespeneCostValue =
                    EconomyOrderValue.AbilityId == ABILITY_ID::BUILD_REACTOR_STARPORT ? 50U : 25U;
                if (!TryReserveResources(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue.CommitmentClass,
                                         MineralsCostValue, VespeneCostValue, 0U))
                {
                    DeferralReasonValue = ECommandOrderDeferralReason::InsufficientResources;
                    break;
                }

                const Unit* StarportUnitValue = nullptr;
                if (!TrySelectAddonProducerUnit(FrameValue, GameStateDescriptorValue, BuildPlacementServiceValue,
                                                ExpansionLocationsValue, CommandAuthoritySchedulingStateValue,
                                                EconomyOrderValue, UNIT_TYPEID::TERRAN_STARPORT,
                                                ProductionClearancePointValue, ProductionBlockerResolutions,
                                                IntentBufferValue, StarportUnitValue, DeferralReasonValue))
                {
                    ReleaseReservedResources(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue.CommitmentClass,
                                             MineralsCostValue,
                                             VespeneCostValue, 0U);
                    break;
                }

                if (!StarportUnitValue->orders.empty())
                {
                    ReleaseReservedResources(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue.CommitmentClass,
                                             MineralsCostValue, VespeneCostValue, 0U);
                    DeferralReasonValue = ECommandOrderDeferralReason::ProducerBusy;
                    break;
                }

                {
                    UnitExecutionOrderValue = FCommandOrderRecord::CreateNoTarget(
                        ECommandAuthorityLayer::UnitExecution, StarportUnitValue->tag, EconomyOrderValue.AbilityId,
                        EconomyOrderValue.BasePriorityValue, EIntentDomain::StructureBuild,
                        GameStateDescriptorValue.CurrentGameLoop, 0U, EconomyOrderValue.OrderId);
                    CopyTaskMetadataToChildOrder(EconomyOrderValue, UnitExecutionOrderValue);
                    CreatedOrderValue = true;
                }
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
            case ABILITY_ID::RESEARCH_TERRANINFANTRYARMORLEVEL1:
            {
                const bool IsResearchUpgradeValue = EconomyOrderValue.UpgradeId.ToType() != UPGRADE_ID::INVALID;
                if (EconomyOrderValue.AbilityId == ABILITY_ID::TRAIN_SCV &&
                    GameStateDescriptorValue.EconomyState.ProjectedAvailableSupplyByHorizon[
                        ShortForecastHorizonIndexValue] <= 1U &&
                    HasOutstandingSupplyDepotDemand(GameStateDescriptorValue) &&
                    GameStateDescriptorValue.EconomyState.ProjectedAvailableMineralsByHorizon[
                        ShortForecastHorizonIndexValue] < 150U)
                {
                    DeferralReasonValue = ECommandOrderDeferralReason::InsufficientResources;
                    break;
                }

                const Units ProducerUnitsValue = EconomyOrderValue.AbilityId == ABILITY_ID::TRAIN_SCV
                                                     ? GetEligibleWorkerProductionStructures(*FrameValue.Observation)
                                                     : FrameValue.Observation->GetUnits(Unit::Alliance::Self,
                                                                                        IsUnit(EconomyOrderValue.ProducerUnitTypeId));

                if (EconomyOrderValue.AbilityId == ABILITY_ID::TRAIN_SCV)
                {
                    const uint32_t RequestedQueueCountValue = std::max<uint32_t>(1U, EconomyOrderValue.RequestedQueueCount);
                    const uint32_t AdditionalChildOrderBudgetValue =
                        RequestedQueueCountValue > ActiveUnitExecutionChildCountValue
                            ? RequestedQueueCountValue - ActiveUnitExecutionChildCountValue
                            : 0U;
                    if (AdditionalChildOrderBudgetValue == 0U)
                    {
                        CommandAuthoritySchedulingStateValue.SetOrderDeferralState(
                            EconomyOrderValue.OrderId, ECommandOrderDeferralReason::AwaitingObservedCompletion,
                            CurrentStepValue, CurrentGameLoopValue);
                        continue;
                    }

                    bool HasBusyProducerValue = false;
                    uint32_t CreatedChildCountValue = 0U;
                    for (const Unit* ProducerUnitValue : ProducerUnitsValue)
                    {
                        if (ProducerUnitValue == nullptr)
                        {
                            continue;
                        }

                        if (ProducerUnitValue->build_progress < 1.0f)
                        {
                            HasBusyProducerValue = true;
                            continue;
                        }

                        // Reserve THIS specific CC for orbital morph if a pending morph
                        // order targets it. Block ALL new SCV production on this CC so
                        // in-progress SCVs finish naturally and the CC idles for morph.
                        // Other CCs continue SCV production normally.
                        if (ProducerUnitValue->unit_type.ToType() == UNIT_TYPEID::TERRAN_COMMANDCENTER &&
                            HasPendingMorphOrderForActor(CommandAuthoritySchedulingStateValue,
                                                         ProducerUnitValue->tag,
                                                         ABILITY_ID::MORPH_ORBITALCOMMAND))
                        {
                            HasBusyProducerValue = true;
                            continue;
                        }

                        {
                            const bool IsReservedForAddonValue = DoesMandatoryOpeningAddonReserveProducer(
                                FrameValue, GameStateDescriptorValue, BuildPlacementServiceValue,
                                ExpansionLocationsValue, CommandAuthoritySchedulingStateValue, *ProducerUnitValue,
                                EconomyOrderValue.OrderId);
                            if (IsReservedForAddonValue && ProducerUnitValue->orders.size() <= 1U)
                            {
                                HasBusyProducerValue = true;
                                continue;
                            }
                        }

                        if (IntentBufferValue.HasIntentForActor(ProducerUnitValue->tag) ||
                            HasConflictingSchedulerOrderForProductionActor(CommandAuthoritySchedulingStateValue,
                                                                           ProducerUnitValue->tag))
                        {
                            HasBusyProducerValue = true;
                            continue;
                        }

                        if (GetCommittedProductionOrderCount(CommandAuthoritySchedulingStateValue, *ProducerUnitValue) >=
                            GetProductionQueueCapacity(*FrameValue.Observation, *ProducerUnitValue))
                        {
                            HasBusyProducerValue = true;
                            continue;
                        }

                        if (!TryReserveUnitCost(GameStateDescriptorValue.BuildPlanning,
                                                EconomyOrderValue.CommitmentClass,
                                                EconomyOrderValue.ResultUnitTypeId))
                        {
                            DeferralReasonValue = ECommandOrderDeferralReason::InsufficientResources;
                            break;
                        }

                        FCommandOrderRecord ScvUnitExecutionOrderValue = FCommandOrderRecord::CreateNoTarget(
                            ECommandAuthorityLayer::UnitExecution, ProducerUnitValue->tag, EconomyOrderValue.AbilityId,
                            EconomyOrderValue.BasePriorityValue, EIntentDomain::UnitProduction,
                            GameStateDescriptorValue.CurrentGameLoop, 0U, EconomyOrderValue.OrderId);
                        CopyTaskMetadataToChildOrder(EconomyOrderValue, ScvUnitExecutionOrderValue);
                        ScvUnitExecutionOrderValue.Queued = !ProducerUnitValue->orders.empty();
                        ScvUnitExecutionOrderValue.LifecycleState = EOrderLifecycleState::Ready;
                        ScvUnitExecutionOrderValue.PlanStepId = EconomyOrderValue.PlanStepId;
                        ScvUnitExecutionOrderValue.TargetCount = EconomyOrderValue.TargetCount;
                        ScvUnitExecutionOrderValue.RequestedQueueCount = EconomyOrderValue.RequestedQueueCount;
                        ScvUnitExecutionOrderValue.ProducerUnitTypeId = ProducerUnitValue->unit_type.ToType();
                        ScvUnitExecutionOrderValue.ResultUnitTypeId = EconomyOrderValue.ResultUnitTypeId;
                        ScvUnitExecutionOrderValue.UpgradeId = EconomyOrderValue.UpgradeId;
                        CommandAuthoritySchedulingStateValue.EnqueueOrder(ScvUnitExecutionOrderValue);
                        ++CreatedChildCountValue;

                        if (CreatedChildCountValue >= AdditionalChildOrderBudgetValue)
                        {
                            break;
                        }
                    }

                    if (CreatedChildCountValue > 0U)
                    {
                        CommandAuthoritySchedulingStateValue.SetOrderDeferralState(
                            EconomyOrderValue.OrderId, ECommandOrderDeferralReason::AwaitingObservedCompletion,
                            CurrentStepValue, CurrentGameLoopValue);
                        continue;
                    }

                    if (DeferralReasonValue == ECommandOrderDeferralReason::None)
                    {
                        DeferralReasonValue = HasBusyProducerValue ? ECommandOrderDeferralReason::ProducerBusy
                                                                   : ECommandOrderDeferralReason::NoProducer;
                    }

                    CommandAuthoritySchedulingStateValue.SetOrderDeferralState(EconomyOrderValue.OrderId,
                                                                               DeferralReasonValue, CurrentStepValue,
                                                                               CurrentGameLoopValue);
                    continue;
                }

                if (!IsResearchUpgradeValue)
                {
                    bool HasBusyProducerValue = false;
                    uint32_t CreatedChildCountValue = 0U;
                    for (const Unit* ProducerUnitValue : ProducerUnitsValue)
                    {
                        if (ProducerUnitValue == nullptr)
                        {
                            continue;
                        }

                        if (ProducerUnitValue->build_progress < 1.0f)
                        {
                            HasBusyProducerValue = true;
                            continue;
                        }

                        const bool IsReservedForAddonValue = DoesMandatoryOpeningAddonReserveProducer(
                            FrameValue, GameStateDescriptorValue, BuildPlacementServiceValue,
                            ExpansionLocationsValue, CommandAuthoritySchedulingStateValue, *ProducerUnitValue,
                            EconomyOrderValue.OrderId);
                        if (IsReservedForAddonValue && ProducerUnitValue->orders.size() <= 1U)
                        {
                            HasBusyProducerValue = true;
                            continue;
                        }

                        const bool HasIntentValue = IntentBufferValue.HasIntentForActor(ProducerUnitValue->tag);
                        const bool HasConflictingOrderValue = HasConflictingSchedulerOrderForProductionActor(
                            CommandAuthoritySchedulingStateValue, ProducerUnitValue->tag);
                        if (HasIntentValue || HasConflictingOrderValue)
                        {
                            HasBusyProducerValue = true;
                            continue;
                        }

                        if (RequiresTechLabAddonForAbility(EconomyOrderValue.AbilityId) &&
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

                        if (!TryReserveUnitCost(GameStateDescriptorValue.BuildPlanning,
                                                EconomyOrderValue.CommitmentClass,
                                                EconomyOrderValue.ResultUnitTypeId))
                        {
                            DeferralReasonValue = ECommandOrderDeferralReason::InsufficientResources;
                            break;
                        }

                        FCommandOrderRecord ProducedUnitExecutionOrderValue = FCommandOrderRecord::CreateNoTarget(
                            ECommandAuthorityLayer::UnitExecution, ProducerUnitValue->tag, EconomyOrderValue.AbilityId,
                            EconomyOrderValue.BasePriorityValue, EIntentDomain::UnitProduction,
                            GameStateDescriptorValue.CurrentGameLoop, 0U, EconomyOrderValue.OrderId);
                        CopyTaskMetadataToChildOrder(EconomyOrderValue, ProducedUnitExecutionOrderValue);
                        ProducedUnitExecutionOrderValue.Queued = !ProducerUnitValue->orders.empty();
                        ProducedUnitExecutionOrderValue.LifecycleState = EOrderLifecycleState::Ready;
                        ProducedUnitExecutionOrderValue.PlanStepId = EconomyOrderValue.PlanStepId;
                        ProducedUnitExecutionOrderValue.TargetCount = EconomyOrderValue.TargetCount;
                        ProducedUnitExecutionOrderValue.RequestedQueueCount = EconomyOrderValue.RequestedQueueCount;
                        ProducedUnitExecutionOrderValue.ProducerUnitTypeId = ProducerUnitValue->unit_type.ToType();
                        ProducedUnitExecutionOrderValue.ResultUnitTypeId = EconomyOrderValue.ResultUnitTypeId;
                        ProducedUnitExecutionOrderValue.UpgradeId = EconomyOrderValue.UpgradeId;
                        CommandAuthoritySchedulingStateValue.EnqueueOrder(ProducedUnitExecutionOrderValue);
                        ++CreatedChildCountValue;
                    }

                    if (CreatedChildCountValue > 0U)
                    {
                        CommandAuthoritySchedulingStateValue.SetOrderDeferralState(
                            EconomyOrderValue.OrderId, ECommandOrderDeferralReason::AwaitingObservedCompletion,
                            CurrentStepValue, CurrentGameLoopValue);
                        continue;
                    }

                    if (DeferralReasonValue == ECommandOrderDeferralReason::None)
                    {
                        DeferralReasonValue = HasBusyProducerValue ? ECommandOrderDeferralReason::ProducerBusy
                                                                   : ECommandOrderDeferralReason::NoProducer;
                    }

                    CommandAuthoritySchedulingStateValue.SetOrderDeferralState(EconomyOrderValue.OrderId,
                                                                               DeferralReasonValue, CurrentStepValue,
                                                                               CurrentGameLoopValue);
                    continue;
                }

                bool HasBusyProducerValue = false;
                for (const Unit* ProducerUnitValue : ProducerUnitsValue)
                {
                    if (ProducerUnitValue == nullptr)
                    {
                        continue;
                    }

                    if (DoesMandatoryOpeningAddonReserveProducer(
                            FrameValue, GameStateDescriptorValue, BuildPlacementServiceValue,
                            ExpansionLocationsValue, CommandAuthoritySchedulingStateValue, *ProducerUnitValue,
                            EconomyOrderValue.OrderId))
                    {
                        HasBusyProducerValue = true;
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

                    if (GetCommittedProductionOrderCount(CommandAuthoritySchedulingStateValue, *ProducerUnitValue) >=
                        GetProductionQueueCapacity(*FrameValue.Observation, *ProducerUnitValue))
                    {
                        HasBusyProducerValue = true;
                        continue;
                    }

                    const bool ReservedCostValue =
                        TryReserveUpgradeCost(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue.CommitmentClass,
                                              EconomyOrderValue.AbilityId);
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
                ApplyOpeningWallFallbackForPlacementDeferral(GameStateDescriptorValue, EconomyOrderValue,
                                                             DeferralReasonValue);
                CommandAuthoritySchedulingStateValue.SetOrderDeferralState(EconomyOrderValue.OrderId,
                                                                           DeferralReasonValue, CurrentStepValue,
                                                                           CurrentGameLoopValue);
            }
            continue;
        }

        GameStateDescriptorValue.OpeningPlanExecutionState.ResetPlacementFailureCount(EconomyOrderValue.PlanStepId);
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
