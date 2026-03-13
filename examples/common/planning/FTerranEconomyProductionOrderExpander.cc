#include "common/planning/FTerranEconomyProductionOrderExpander.h"

#include <limits>

#include "common/bot_status_models.h"
#include "common/economic_models.h"
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

uint32_t GetObservedCountForOrder(const FBuildPlanningState& BuildPlanningStateValue,
                                  const FCommandOrderRecord& CommandOrderRecordValue)
{
    if (IsTerranBuilding(CommandOrderRecordValue.ResultUnitTypeId) ||
        CommandOrderRecordValue.ResultUnitTypeId == UNIT_TYPEID::TERRAN_COMMANDCENTER)
    {
        return GetObservedBuildingCount(BuildPlanningStateValue, CommandOrderRecordValue.ResultUnitTypeId);
    }

    return GetObservedUnitCount(BuildPlanningStateValue, CommandOrderRecordValue.ResultUnitTypeId);
}

uint32_t GetObservedInConstructionCountForOrder(const FBuildPlanningState& BuildPlanningStateValue,
                                                const FCommandOrderRecord& CommandOrderRecordValue)
{
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
           static_cast<int>(CountActiveSchedulerOrdersForActor(CommandAuthoritySchedulingStateValue,
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
                               const ABILITY_ID StructureAbilityIdValue, const Unit& WorkerUnitValue,
                               Point2D& OutBuildPointValue)
{
    if (FrameValue.Observation == nullptr || FrameValue.Query == nullptr)
    {
        return false;
    }

    const Point2D BaseLocationValue = Point2D(FrameValue.Observation->GetStartLocation());
    const std::vector<Point2D> CandidateValues = BuildPlacementServiceValue.GetStructurePlacementCandidates(
        GameStateDescriptorValue, StructureAbilityIdValue, BaseLocationValue);
    const GameInfo& GameInfoValue = FrameValue.Observation->GetGameInfo();

    for (const Point2D& CandidateValue : CandidateValues)
    {
        const Point2D ClampedCandidateValue = ClampToPlayable(GameInfoValue, CandidateValue);
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
    const std::vector<size_t> PlanningOrderIndicesValue = CommandAuthoritySchedulingStateValue.PlanningProcessIndices;

    for (const size_t PlanningOrderIndexValue : PlanningOrderIndicesValue)
    {
        const FCommandOrderRecord EconomyOrderValue =
            CommandAuthoritySchedulingStateValue.GetOrderRecord(PlanningOrderIndexValue);
        if (EconomyOrderValue.SourceLayer != ECommandAuthorityLayer::EconomyAndProduction ||
            EconomyOrderValue.LifecycleState != EOrderLifecycleState::Queued)
        {
            continue;
        }

        const uint32_t ObservedCountValue =
            GetObservedCountForOrder(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue);
        if (ObservedCountValue >= EconomyOrderValue.TargetCount)
        {
            CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(EconomyOrderValue.OrderId,
                                                                       EOrderLifecycleState::Completed);
            continue;
        }

        if (HasActiveUnitExecutionChild(CommandAuthoritySchedulingStateValue, EconomyOrderValue.OrderId))
        {
            continue;
        }

        const uint32_t PendingOrderCountValue =
            CountCurrentOrdersForAbility(AgentStateValue, EconomyOrderValue.AbilityId) +
            CountPendingSchedulerOrdersForAbility(CommandAuthoritySchedulingStateValue, EconomyOrderValue.AbilityId) +
            CountPendingIntentsForAbility(IntentBufferValue, EconomyOrderValue.AbilityId);
        const uint32_t ProjectedCountValue =
            ObservedCountValue +
            GetObservedInConstructionCountForOrder(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue) +
            PendingOrderCountValue;
        if (ProjectedCountValue >= EconomyOrderValue.TargetCount)
        {
            continue;
        }

        FCommandOrderRecord UnitExecutionOrderValue;
        bool CreatedOrderValue = false;

        switch (EconomyOrderValue.AbilityId.ToType())
        {
            case ABILITY_ID::BUILD_SUPPLYDEPOT:
            case ABILITY_ID::BUILD_BARRACKS:
            case ABILITY_ID::BUILD_FACTORY:
            case ABILITY_ID::BUILD_STARPORT:
            {
                const Point2D BaseLocationValue = Point2D(FrameValue.Observation->GetStartLocation());
                const Point2D BuildAnchorValue =
                    BuildPlacementServiceValue.GetPrimaryStructureAnchor(GameStateDescriptorValue, BaseLocationValue);
                const Unit* WorkerUnitValue = SelectBuildWorker(AgentStateValue, IntentBufferValue,
                                                                CommandAuthoritySchedulingStateValue,
                                                                UNIT_TYPEID::TERRAN_SCV, BuildAnchorValue);
                if (WorkerUnitValue == nullptr)
                {
                    break;
                }

                Point2D BuildPointValue;
                if (!TryGetStructureBuildPoint(FrameValue, GameStateDescriptorValue, BuildPlacementServiceValue,
                                               EconomyOrderValue.AbilityId, *WorkerUnitValue, BuildPointValue))
                {
                    break;
                }

                if (!TryReserveStructureCost(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue.ResultUnitTypeId))
                {
                    break;
                }

                UnitExecutionOrderValue = FCommandOrderRecord::CreatePointTarget(
                    ECommandAuthorityLayer::UnitExecution, WorkerUnitValue->tag, EconomyOrderValue.AbilityId,
                    BuildPointValue, EconomyOrderValue.PriorityValue, EIntentDomain::StructureBuild,
                    GameStateDescriptorValue.CurrentGameLoop, 0U, EconomyOrderValue.OrderId, -1, -1, false, true,
                    false);
                CreatedOrderValue = true;
                break;
            }
            case ABILITY_ID::BUILD_REFINERY:
            {
                if (FrameValue.Query == nullptr ||
                    !TryReserveStructureCost(GameStateDescriptorValue.BuildPlanning, UNIT_TYPEID::TERRAN_REFINERY))
                {
                    break;
                }

                const Units TownHallUnitsValue = FrameValue.Observation->GetUnits(Unit::Alliance::Self, IsTownHall());
                const Units GeyserUnitsValue = FrameValue.Observation->GetUnits(Unit::Alliance::Neutral, IsGeyser());

                for (const Unit* TownHallUnitValue : TownHallUnitsValue)
                {
                    if (TownHallUnitValue == nullptr || TownHallUnitValue->build_progress < 1.0f)
                    {
                        continue;
                    }

                    for (const Unit* GeyserUnitValue : GeyserUnitsValue)
                    {
                        if (GeyserUnitValue == nullptr || Distance2D(TownHallUnitValue->pos, GeyserUnitValue->pos) > 15.0f ||
                            !FrameValue.Query->Placement(ABILITY_ID::BUILD_REFINERY, GeyserUnitValue->pos))
                        {
                            continue;
                        }

                        const Unit* WorkerUnitValue = SelectBuildWorker(AgentStateValue, IntentBufferValue,
                                                                        CommandAuthoritySchedulingStateValue,
                                                                        UNIT_TYPEID::TERRAN_SCV, GeyserUnitValue->pos);
                        if (WorkerUnitValue == nullptr)
                        {
                            continue;
                        }

                        UnitExecutionOrderValue = FCommandOrderRecord::CreateUnitTarget(
                            ECommandAuthorityLayer::UnitExecution, WorkerUnitValue->tag, EconomyOrderValue.AbilityId,
                            GeyserUnitValue->tag, EconomyOrderValue.PriorityValue, EIntentDomain::StructureBuild,
                            GameStateDescriptorValue.CurrentGameLoop, 0U, EconomyOrderValue.OrderId);
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
                }
                break;
            }
            case ABILITY_ID::BUILD_COMMANDCENTER:
            {
                const Point2D ExpansionLocationValue = GetNextExpansionLocation(FrameValue, ExpansionLocationsValue);
                if (!IsPointFinite(ExpansionLocationValue))
                {
                    break;
                }

                const Unit* WorkerUnitValue = SelectBuildWorker(AgentStateValue, IntentBufferValue,
                                                                CommandAuthoritySchedulingStateValue,
                                                                UNIT_TYPEID::TERRAN_SCV, ExpansionLocationValue);
                if (WorkerUnitValue == nullptr || FrameValue.Query == nullptr ||
                    !FrameValue.Query->Placement(ABILITY_ID::BUILD_COMMANDCENTER, ExpansionLocationValue, WorkerUnitValue))
                {
                    break;
                }

                if (!TryReserveStructureCost(GameStateDescriptorValue.BuildPlanning, UNIT_TYPEID::TERRAN_COMMANDCENTER))
                {
                    break;
                }

                UnitExecutionOrderValue = FCommandOrderRecord::CreatePointTarget(
                    ECommandAuthorityLayer::UnitExecution, WorkerUnitValue->tag, EconomyOrderValue.AbilityId,
                    ExpansionLocationValue, EconomyOrderValue.PriorityValue, EIntentDomain::StructureBuild,
                    GameStateDescriptorValue.CurrentGameLoop, 0U, EconomyOrderValue.OrderId, -1, -1, false, true,
                    false);
                CreatedOrderValue = true;
                break;
            }
            case ABILITY_ID::MORPH_ORBITALCOMMAND:
            {
                if (!TryReserveResources(GameStateDescriptorValue.BuildPlanning, 150U, 0U, 0U))
                {
                    break;
                }

                const Units TownHallUnitsValue = FrameValue.Observation->GetUnits(Unit::Alliance::Self, IsTownHall());
                const Unit* TownHallUnitValue = nullptr;
                for (const Unit* CandidateTownHallUnitValue : TownHallUnitsValue)
                {
                    if (CandidateTownHallUnitValue == nullptr ||
                        CandidateTownHallUnitValue->unit_type.ToType() != UNIT_TYPEID::TERRAN_COMMANDCENTER ||
                        CandidateTownHallUnitValue->build_progress < 1.0f || !CandidateTownHallUnitValue->orders.empty() ||
                        IntentBufferValue.HasIntentForActor(CandidateTownHallUnitValue->tag) ||
                        HasActiveSchedulerOrderForActor(CommandAuthoritySchedulingStateValue,
                                                        CandidateTownHallUnitValue->tag))
                    {
                        continue;
                    }

                    TownHallUnitValue = CandidateTownHallUnitValue;
                    break;
                }

                if (TownHallUnitValue == nullptr)
                {
                    ReleaseReservedResources(GameStateDescriptorValue.BuildPlanning, 150U, 0U, 0U);
                    break;
                }

                UnitExecutionOrderValue = FCommandOrderRecord::CreateNoTarget(
                    ECommandAuthorityLayer::UnitExecution, TownHallUnitValue->tag, EconomyOrderValue.AbilityId,
                    EconomyOrderValue.PriorityValue, EIntentDomain::StructureBuild,
                    GameStateDescriptorValue.CurrentGameLoop, 0U, EconomyOrderValue.OrderId);
                CreatedOrderValue = true;
                break;
            }
            case ABILITY_ID::BUILD_REACTOR_BARRACKS:
            {
                if (!TryReserveResources(GameStateDescriptorValue.BuildPlanning, 50U, 50U, 0U))
                {
                    break;
                }

                const Units BarracksUnitsValue =
                    FrameValue.Observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_BARRACKS));
                const Unit* BarracksUnitValue = nullptr;
                for (const Unit* CandidateBarracksUnitValue : BarracksUnitsValue)
                {
                    if (CandidateBarracksUnitValue == nullptr || CandidateBarracksUnitValue->build_progress < 1.0f ||
                        !CandidateBarracksUnitValue->orders.empty() || CandidateBarracksUnitValue->add_on_tag != NullTag ||
                        IntentBufferValue.HasIntentForActor(CandidateBarracksUnitValue->tag) ||
                        HasActiveSchedulerOrderForActor(CommandAuthoritySchedulingStateValue,
                                                        CandidateBarracksUnitValue->tag))
                    {
                        continue;
                    }

                    BarracksUnitValue = CandidateBarracksUnitValue;
                    break;
                }

                if (BarracksUnitValue == nullptr)
                {
                    ReleaseReservedResources(GameStateDescriptorValue.BuildPlanning, 50U, 50U, 0U);
                    break;
                }

                UnitExecutionOrderValue = FCommandOrderRecord::CreateNoTarget(
                    ECommandAuthorityLayer::UnitExecution, BarracksUnitValue->tag, EconomyOrderValue.AbilityId,
                    EconomyOrderValue.PriorityValue, EIntentDomain::StructureBuild,
                    GameStateDescriptorValue.CurrentGameLoop, 0U, EconomyOrderValue.OrderId);
                CreatedOrderValue = true;
                break;
            }
            case ABILITY_ID::BUILD_TECHLAB_FACTORY:
            {
                if (!TryReserveResources(GameStateDescriptorValue.BuildPlanning, 50U, 25U, 0U))
                {
                    break;
                }

                const Units FactoryUnitsValue =
                    FrameValue.Observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_FACTORY));
                const Unit* FactoryUnitValue = nullptr;
                for (const Unit* CandidateFactoryUnitValue : FactoryUnitsValue)
                {
                    if (CandidateFactoryUnitValue == nullptr || CandidateFactoryUnitValue->build_progress < 1.0f ||
                        !CandidateFactoryUnitValue->orders.empty() || CandidateFactoryUnitValue->add_on_tag != NullTag ||
                        IntentBufferValue.HasIntentForActor(CandidateFactoryUnitValue->tag) ||
                        HasActiveSchedulerOrderForActor(CommandAuthoritySchedulingStateValue,
                                                        CandidateFactoryUnitValue->tag))
                    {
                        continue;
                    }

                    FactoryUnitValue = CandidateFactoryUnitValue;
                    break;
                }

                if (FactoryUnitValue == nullptr)
                {
                    ReleaseReservedResources(GameStateDescriptorValue.BuildPlanning, 50U, 25U, 0U);
                    break;
                }

                UnitExecutionOrderValue = FCommandOrderRecord::CreateNoTarget(
                    ECommandAuthorityLayer::UnitExecution, FactoryUnitValue->tag, EconomyOrderValue.AbilityId,
                    EconomyOrderValue.PriorityValue, EIntentDomain::StructureBuild,
                    GameStateDescriptorValue.CurrentGameLoop, 0U, EconomyOrderValue.OrderId);
                CreatedOrderValue = true;
                break;
            }
            case ABILITY_ID::TRAIN_SCV:
            case ABILITY_ID::TRAIN_MARINE:
            case ABILITY_ID::TRAIN_HELLION:
            case ABILITY_ID::TRAIN_CYCLONE:
            case ABILITY_ID::TRAIN_MEDIVAC:
            case ABILITY_ID::TRAIN_LIBERATOR:
            case ABILITY_ID::TRAIN_SIEGETANK:
            {
                const Units ProducerUnitsValue =
                    FrameValue.Observation->GetUnits(Unit::Alliance::Self, IsUnit(EconomyOrderValue.ProducerUnitTypeId));
                for (const Unit* ProducerUnitValue : ProducerUnitsValue)
                {
                    if (ProducerUnitValue == nullptr || ProducerUnitValue->build_progress < 1.0f ||
                        IntentBufferValue.HasIntentForActor(ProducerUnitValue->tag) ||
                        HasActiveSchedulerOrderForActor(CommandAuthoritySchedulingStateValue, ProducerUnitValue->tag))
                    {
                        continue;
                    }

                    if ((EconomyOrderValue.AbilityId == ABILITY_ID::TRAIN_CYCLONE ||
                         EconomyOrderValue.AbilityId == ABILITY_ID::TRAIN_SIEGETANK) &&
                        !HasTechLabAddon(*FrameValue.Observation, *ProducerUnitValue))
                    {
                        continue;
                    }

                    if (GetCommittedProductionOrderCount(CommandAuthoritySchedulingStateValue, *ProducerUnitValue) >=
                        GetProductionQueueCapacity(*FrameValue.Observation, *ProducerUnitValue))
                    {
                        continue;
                    }

                    if (!TryReserveUnitCost(GameStateDescriptorValue.BuildPlanning, EconomyOrderValue.ResultUnitTypeId))
                    {
                        break;
                    }

                    UnitExecutionOrderValue = FCommandOrderRecord::CreateNoTarget(
                        ECommandAuthorityLayer::UnitExecution, ProducerUnitValue->tag, EconomyOrderValue.AbilityId,
                        EconomyOrderValue.PriorityValue, EIntentDomain::UnitProduction,
                        GameStateDescriptorValue.CurrentGameLoop, 0U, EconomyOrderValue.OrderId);
                    UnitExecutionOrderValue.Queued = !ProducerUnitValue->orders.empty();
                    CreatedOrderValue = true;
                    break;
                }
                break;
            }
            default:
                break;
        }

        if (!CreatedOrderValue)
        {
            continue;
        }

        UnitExecutionOrderValue.LifecycleState = EOrderLifecycleState::Ready;
        UnitExecutionOrderValue.PlanStepId = EconomyOrderValue.PlanStepId;
        UnitExecutionOrderValue.TargetCount = EconomyOrderValue.TargetCount;
        UnitExecutionOrderValue.ProducerUnitTypeId = EconomyOrderValue.ProducerUnitTypeId;
        UnitExecutionOrderValue.ResultUnitTypeId = EconomyOrderValue.ResultUnitTypeId;
        UnitExecutionOrderValue.UpgradeId = EconomyOrderValue.UpgradeId;
        CommandAuthoritySchedulingStateValue.EnqueueOrder(UnitExecutionOrderValue);
    }
}

}  // namespace sc2
