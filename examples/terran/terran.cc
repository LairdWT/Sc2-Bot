#include "terran.h"

#include <algorithm>
#include <unordered_map>

#include "sc2lib/sc2_search.h"

namespace sc2
{
namespace
{

constexpr float WallStructureMatchRadiusSquaredValue = 6.25f;
constexpr int GasHarvestIntentPriorityValue = 320;
constexpr int GasReliefIntentPriorityValue = 321;

EExecutionConditionState GetExecutionConditionState(const bool ConditionValue)
{
    return ConditionValue ? EExecutionConditionState::Active : EExecutionConditionState::Inactive;
}

bool IsProductionRallyStructureType(const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_BARRACKS:
        case UNIT_TYPEID::TERRAN_FACTORY:
        case UNIT_TYPEID::TERRAN_STARPORT:
            return true;
        default:
            return false;
    }
}

AbilityID GetProductionStructureRallyAbility(const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_BARRACKS:
        case UNIT_TYPEID::TERRAN_FACTORY:
        case UNIT_TYPEID::TERRAN_STARPORT:
            return ABILITY_ID::RALLY_UNITS;
        default:
            return ABILITY_ID::INVALID;
    }
}

bool IsWallDepotUnitType(const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOT:
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED:
            return true;
        default:
            return false;
    }
}

void PrintMainLayoutSlotFamily(const char* LabelPtrValue,
                               const std::vector<FBuildPlacementSlot>& BuildPlacementSlotsValue)
{
    std::cout << " | " << LabelPtrValue << " ";
    if (BuildPlacementSlotsValue.empty())
    {
        std::cout << "None";
        return;
    }

    for (size_t SlotIndexValue = 0U; SlotIndexValue < BuildPlacementSlotsValue.size(); ++SlotIndexValue)
    {
        if (SlotIndexValue > 0U)
        {
            std::cout << ",";
        }

        const FBuildPlacementSlot& BuildPlacementSlotValue = BuildPlacementSlotsValue[SlotIndexValue];
        std::cout << SlotIndexValue << ":(" << BuildPlacementSlotValue.BuildPoint.x << ", "
                  << BuildPlacementSlotValue.BuildPoint.y << ")";
    }
}

void PrintGoalList(const char* LabelPtrValue, const std::vector<FGoalDescriptor>& GoalDescriptorsValue)
{
    std::cout << LabelPtrValue << ": ";
    if (GoalDescriptorsValue.empty())
    {
        std::cout << "None";
        return;
    }

    for (size_t GoalIndexValue = 0U; GoalIndexValue < GoalDescriptorsValue.size(); ++GoalIndexValue)
    {
        if (GoalIndexValue > 0U)
        {
            std::cout << " | ";
        }

        const FGoalDescriptor& GoalDescriptorValue = GoalDescriptorsValue[GoalIndexValue];
        std::cout << GoalDescriptorValue.GoalId << ":" << ToString(GoalDescriptorValue.GoalType)
                  << "=" << GoalDescriptorValue.TargetCount;
    }
}

void PrintPriorityTierQueueSummary(const char* LabelPtrValue,
                                   const std::array<std::vector<size_t>, CommandPriorityTierCountValue>& QueueGroupValue)
{
    std::cout << LabelPtrValue << ": ";
    for (size_t PriorityTierIndexValue = 0U; PriorityTierIndexValue < CommandPriorityTierCountValue;
         ++PriorityTierIndexValue)
    {
        if (PriorityTierIndexValue > 0U)
        {
            std::cout << " | ";
        }

        const ECommandPriorityTier CommandPriorityTierValue = static_cast<ECommandPriorityTier>(PriorityTierIndexValue);
        std::cout << ToString(CommandPriorityTierValue) << " " << QueueGroupValue[PriorityTierIndexValue].size();
    }
}

void PrintReadyIntentQueueSummary(
    const std::array<std::array<std::vector<size_t>, IntentDomainCountValue>, CommandPriorityTierCountValue>&
        QueueGroupValue)
{
    std::cout << "ReadyIntents: ";
    for (size_t PriorityTierIndexValue = 0U; PriorityTierIndexValue < CommandPriorityTierCountValue;
         ++PriorityTierIndexValue)
    {
        if (PriorityTierIndexValue > 0U)
        {
            std::cout << " | ";
        }

        uint32_t TierIntentCountValue = 0U;
        for (size_t IntentDomainIndexValue = 0U; IntentDomainIndexValue < IntentDomainCountValue; ++IntentDomainIndexValue)
        {
            TierIntentCountValue +=
                static_cast<uint32_t>(QueueGroupValue[PriorityTierIndexValue][IntentDomainIndexValue].size());
        }

        const ECommandPriorityTier CommandPriorityTierValue = static_cast<ECommandPriorityTier>(PriorityTierIndexValue);
        std::cout << ToString(CommandPriorityTierValue) << " " << TierIntentCountValue;
    }
}

bool IsProductionRailStructureType(const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_BARRACKS:
        case UNIT_TYPEID::TERRAN_FACTORY:
        case UNIT_TYPEID::TERRAN_STARPORT:
            return true;
        default:
            return false;
    }
}

const Unit* FindProductionRailStructureForSlot(const Units& SelfUnitsValue,
                                               const FBuildPlacementSlot& BuildPlacementSlotValue)
{
    const Unit* BestUnitValue = nullptr;
    float BestDistanceSquaredValue = std::numeric_limits<float>::max();

    for (const Unit* SelfUnitValue : SelfUnitsValue)
    {
        if (SelfUnitValue == nullptr || !SelfUnitValue->is_building || SelfUnitValue->is_flying ||
            !IsProductionRailStructureType(SelfUnitValue->unit_type.ToType()))
        {
            continue;
        }

        const float DistanceSquaredValue =
            DistanceSquared2D(Point2D(SelfUnitValue->pos), BuildPlacementSlotValue.BuildPoint);
        if (DistanceSquaredValue > WallStructureMatchRadiusSquaredValue || DistanceSquaredValue >= BestDistanceSquaredValue)
        {
            continue;
        }

        BestDistanceSquaredValue = DistanceSquaredValue;
        BestUnitValue = SelfUnitValue;
    }

    return BestUnitValue;
}

const char* GetProductionRailOccupancyLabel(const Unit* OccupyingUnitValue)
{
    if (OccupyingUnitValue == nullptr)
    {
        return "Empty";
    }

    switch (OccupyingUnitValue->unit_type.ToType())
    {
        case UNIT_TYPEID::TERRAN_BARRACKS:
            return OccupyingUnitValue->build_progress >= 1.0f ? "Barracks" : "BarracksInProgress";
        case UNIT_TYPEID::TERRAN_FACTORY:
            return OccupyingUnitValue->build_progress >= 1.0f ? "Factory" : "FactoryInProgress";
        case UNIT_TYPEID::TERRAN_STARPORT:
            return OccupyingUnitValue->build_progress >= 1.0f ? "Starport" : "StarportInProgress";
        default:
            return OccupyingUnitValue->build_progress >= 1.0f ? "Occupied" : "OccupiedInProgress";
    }
}

void PrintProductionRailSlots(const std::vector<FBuildPlacementSlot>& BuildPlacementSlotsValue,
                              const Units& SelfUnitsValue)
{
    std::cout << " | ProductionRail ";
    if (BuildPlacementSlotsValue.empty())
    {
        std::cout << "None";
        return;
    }

    for (size_t SlotIndexValue = 0U; SlotIndexValue < BuildPlacementSlotsValue.size(); ++SlotIndexValue)
    {
        if (SlotIndexValue > 0U)
        {
            std::cout << ",";
        }

        const FBuildPlacementSlot& BuildPlacementSlotValue = BuildPlacementSlotsValue[SlotIndexValue];
        const Unit* OccupyingUnitValue = FindProductionRailStructureForSlot(SelfUnitsValue, BuildPlacementSlotValue);
        std::cout << static_cast<uint32_t>(BuildPlacementSlotValue.SlotId.Ordinal)
                  << ":" << GetProductionRailOccupancyLabel(OccupyingUnitValue)
                  << "@(" << BuildPlacementSlotValue.BuildPoint.x << ", "
                  << BuildPlacementSlotValue.BuildPoint.y << ")";
    }
}

bool IsRefineryUnitType(const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_REFINERY:
        case UNIT_TYPEID::TERRAN_REFINERYRICH:
            return true;
        default:
            return false;
    }
}

bool IsHarvestGatherAbility(const ABILITY_ID AbilityIdValue)
{
    switch (AbilityIdValue)
    {
        case ABILITY_ID::HARVEST_GATHER:
        case ABILITY_ID::HARVEST_GATHER_SCV:
            return true;
        default:
            return false;
    }
}

bool IsHarvestReturnAbility(const ABILITY_ID AbilityIdValue)
{
    switch (AbilityIdValue)
    {
        case ABILITY_ID::HARVEST_RETURN:
        case ABILITY_ID::HARVEST_RETURN_SCV:
            return true;
        default:
            return false;
    }
}

bool IsWorkerCommittedToConstruction(const Unit& WorkerUnitValue)
{
    if (WorkerUnitValue.orders.empty())
    {
        return false;
    }

    switch (WorkerUnitValue.orders.front().ability_id.ToType())
    {
        case ABILITY_ID::BUILD_SUPPLYDEPOT:
        case ABILITY_ID::BUILD_BARRACKS:
        case ABILITY_ID::BUILD_FACTORY:
        case ABILITY_ID::BUILD_STARPORT:
        case ABILITY_ID::BUILD_REFINERY:
        case ABILITY_ID::BUILD_COMMANDCENTER:
            return true;
        default:
            return false;
    }
}

bool HasActiveSchedulerOrderForActorTag(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                        const Tag ActorTagValue)
{
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.ActorTags[OrderIndexValue] != ActorTagValue ||
            IsTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        return true;
    }

    return false;
}

Tag GetCommittedRefineryTag(const ObservationInterface& ObservationValue, const Unit& WorkerUnitValue)
{
    if (WorkerUnitValue.orders.empty())
    {
        return NullTag;
    }

    const UnitOrder& WorkerOrderValue = WorkerUnitValue.orders.front();
    if (!IsHarvestGatherAbility(WorkerOrderValue.ability_id) && !IsHarvestReturnAbility(WorkerOrderValue.ability_id))
    {
        return NullTag;
    }

    if (WorkerOrderValue.target_unit_tag == NullTag)
    {
        return NullTag;
    }

    const Unit* TargetUnitValue = ObservationValue.GetUnit(WorkerOrderValue.target_unit_tag);
    if (TargetUnitValue == nullptr || !IsRefineryUnitType(TargetUnitValue->unit_type.ToType()))
    {
        return NullTag;
    }

    return TargetUnitValue->tag;
}

bool IsWorkerCommittedToRefinery(const ObservationInterface& ObservationValue, const Unit& WorkerUnitValue)
{
    if (IsCarryingVespene(WorkerUnitValue))
    {
        return true;
    }

    return GetCommittedRefineryTag(ObservationValue, WorkerUnitValue) != NullTag;
}

bool IsWorkerCommittedToMinerals(const ObservationInterface& ObservationValue, const Unit& WorkerUnitValue)
{
    if (IsCarryingMinerals(WorkerUnitValue))
    {
        return true;
    }

    if (WorkerUnitValue.orders.empty())
    {
        return false;
    }

    const UnitOrder& WorkerOrderValue = WorkerUnitValue.orders.front();
    if (!IsHarvestGatherAbility(WorkerOrderValue.ability_id) && !IsHarvestReturnAbility(WorkerOrderValue.ability_id))
    {
        return false;
    }

    if (WorkerOrderValue.target_unit_tag == NullTag)
    {
        return false;
    }

    const Unit* TargetUnitValue = ObservationValue.GetUnit(WorkerOrderValue.target_unit_tag);
    if (TargetUnitValue == nullptr)
    {
        return false;
    }

    const IsMineralPatch MineralPatchFilterValue;
    return MineralPatchFilterValue(*TargetUnitValue);
}

bool DoesUnitMatchWallSlot(const Unit& SelfUnitValue, const FBuildPlacementSlot& BuildPlacementSlotValue)
{
    if (!SelfUnitValue.is_building || SelfUnitValue.is_flying)
    {
        return false;
    }

    switch (BuildPlacementSlotValue.SlotId.SlotType)
    {
        case EBuildPlacementSlotType::MainRampDepotLeft:
        case EBuildPlacementSlotType::MainRampDepotRight:
            return IsWallDepotUnitType(SelfUnitValue.unit_type.ToType());
        case EBuildPlacementSlotType::MainRampBarracksWithAddon:
            return SelfUnitValue.unit_type.ToType() == UNIT_TYPEID::TERRAN_BARRACKS;
        case EBuildPlacementSlotType::Unknown:
        case EBuildPlacementSlotType::NaturalApproachDepot:
        case EBuildPlacementSlotType::MainSupportDepot:
        case EBuildPlacementSlotType::MainBarracksWithAddon:
        case EBuildPlacementSlotType::MainFactoryWithAddon:
        case EBuildPlacementSlotType::MainStarportWithAddon:
        case EBuildPlacementSlotType::MainProductionWithAddon:
        case EBuildPlacementSlotType::MainSupportStructure:
        default:
            return false;
    }
}

const Unit* FindWallStructureForSlot(const Units& SelfUnitsValue, const FBuildPlacementSlot& BuildPlacementSlotValue)
{
    const Unit* BestUnitValue = nullptr;
    float BestDistanceSquaredValue = std::numeric_limits<float>::max();

    for (const Unit* SelfUnitValue : SelfUnitsValue)
    {
        if (SelfUnitValue == nullptr || !DoesUnitMatchWallSlot(*SelfUnitValue, BuildPlacementSlotValue))
        {
            continue;
        }

        const float DistanceSquaredValue =
            DistanceSquared2D(Point2D(SelfUnitValue->pos), BuildPlacementSlotValue.BuildPoint);
        if (DistanceSquaredValue > WallStructureMatchRadiusSquaredValue || DistanceSquaredValue >= BestDistanceSquaredValue)
        {
            continue;
        }

        BestDistanceSquaredValue = DistanceSquaredValue;
        BestUnitValue = SelfUnitValue;
    }

    return BestUnitValue;
}

const char* GetWallSlotOccupancyLabel(const Unit* OccupyingUnitValue)
{
    if (OccupyingUnitValue == nullptr)
    {
        return "Empty";
    }

    return OccupyingUnitValue->build_progress >= 1.0f ? "Filled" : "InProgress";
}

const Unit* SelectWorkerForRefinery(const ObservationInterface& ObservationValue, const FAgentState& AgentStateValue,
                                    const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                    const FIntentBuffer& IntentBufferValue, const Unit& RefineryUnitValue,
                                    const std::unordered_set<Tag>& ReservedWorkerTagsValue)
{
    const Unit* BestIdleWorkerValue = nullptr;
    float BestIdleWorkerDistanceSquaredValue = std::numeric_limits<float>::max();
    const Unit* BestMineralWorkerValue = nullptr;
    float BestMineralWorkerDistanceSquaredValue = std::numeric_limits<float>::max();

    for (const Unit* WorkerUnitValue : AgentStateValue.UnitContainer.ControlledUnits)
    {
        if (WorkerUnitValue == nullptr || WorkerUnitValue->unit_type.ToType() != UNIT_TYPEID::TERRAN_SCV ||
            WorkerUnitValue->build_progress < 1.0f ||
            ReservedWorkerTagsValue.find(WorkerUnitValue->tag) != ReservedWorkerTagsValue.end() ||
            IntentBufferValue.HasIntentForActor(WorkerUnitValue->tag) ||
            HasActiveSchedulerOrderForActorTag(CommandAuthoritySchedulingStateValue, WorkerUnitValue->tag) ||
            IsWorkerCommittedToConstruction(*WorkerUnitValue) ||
            IsWorkerCommittedToRefinery(ObservationValue, *WorkerUnitValue))
        {
            continue;
        }

        const float WorkerDistanceSquaredValue =
            DistanceSquared2D(Point2D(WorkerUnitValue->pos), Point2D(RefineryUnitValue.pos));
        if (WorkerUnitValue->orders.empty())
        {
            if (BestIdleWorkerValue == nullptr ||
                WorkerDistanceSquaredValue < BestIdleWorkerDistanceSquaredValue)
            {
                BestIdleWorkerValue = WorkerUnitValue;
                BestIdleWorkerDistanceSquaredValue = WorkerDistanceSquaredValue;
            }
            continue;
        }

        if (!IsWorkerCommittedToMinerals(ObservationValue, *WorkerUnitValue))
        {
            continue;
        }

        if (BestMineralWorkerValue == nullptr ||
            WorkerDistanceSquaredValue < BestMineralWorkerDistanceSquaredValue)
        {
            BestMineralWorkerValue = WorkerUnitValue;
            BestMineralWorkerDistanceSquaredValue = WorkerDistanceSquaredValue;
        }
    }

    return BestIdleWorkerValue != nullptr ? BestIdleWorkerValue : BestMineralWorkerValue;
}

const Unit* SelectWorkerForGasRelief(const ObservationInterface& ObservationValue, const FAgentState& AgentStateValue,
                                     const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                     const FIntentBuffer& IntentBufferValue, const Unit& RefineryUnitValue,
                                     const std::unordered_set<Tag>& ReservedWorkerTagsValue)
{
    const Unit* BestRefineryWorkerValue = nullptr;
    float BestRefineryWorkerDistanceSquaredValue = std::numeric_limits<float>::max();

    for (const Unit* WorkerUnitValue : AgentStateValue.UnitContainer.ControlledUnits)
    {
        if (WorkerUnitValue == nullptr || WorkerUnitValue->unit_type.ToType() != UNIT_TYPEID::TERRAN_SCV ||
            WorkerUnitValue->build_progress < 1.0f ||
            ReservedWorkerTagsValue.find(WorkerUnitValue->tag) != ReservedWorkerTagsValue.end() ||
            IntentBufferValue.HasIntentForActor(WorkerUnitValue->tag) ||
            HasActiveSchedulerOrderForActorTag(CommandAuthoritySchedulingStateValue, WorkerUnitValue->tag) ||
            IsWorkerCommittedToConstruction(*WorkerUnitValue) ||
            GetCommittedRefineryTag(ObservationValue, *WorkerUnitValue) != RefineryUnitValue.tag)
        {
            continue;
        }

        const float WorkerDistanceSquaredValue =
            DistanceSquared2D(Point2D(WorkerUnitValue->pos), Point2D(RefineryUnitValue.pos));
        if (BestRefineryWorkerValue == nullptr ||
            WorkerDistanceSquaredValue < BestRefineryWorkerDistanceSquaredValue)
        {
            BestRefineryWorkerValue = WorkerUnitValue;
            BestRefineryWorkerDistanceSquaredValue = WorkerDistanceSquaredValue;
        }
    }

    return BestRefineryWorkerValue;
}

int GetCommittedHarvesterCountForRefinery(const ObservationInterface& ObservationValue,
                                          const FAgentState& AgentStateValue,
                                          const Tag RefineryTagValue)
{
    int CommittedHarvesterCountValue = 0;

    for (const Unit* WorkerUnitValue : AgentStateValue.UnitContainer.ControlledUnits)
    {
        if (WorkerUnitValue == nullptr || WorkerUnitValue->unit_type.ToType() != UNIT_TYPEID::TERRAN_SCV ||
            WorkerUnitValue->build_progress < 1.0f)
        {
            continue;
        }

        if (GetCommittedRefineryTag(ObservationValue, *WorkerUnitValue) != RefineryTagValue)
        {
            continue;
        }

        ++CommittedHarvesterCountValue;
    }

    return CommittedHarvesterCountValue;
}

int GetPlannedHarvesterDeltaForRefinery(const std::unordered_map<Tag, int>& PlannedFillCountsByRefineryTagValue,
                                        const std::unordered_map<Tag, int>& PlannedReliefCountsByRefineryTagValue,
                                        const Tag RefineryTagValue)
{
    int PlannedHarvesterDeltaValue = 0;

    const std::unordered_map<Tag, int>::const_iterator PlannedFillIteratorValue =
        PlannedFillCountsByRefineryTagValue.find(RefineryTagValue);
    if (PlannedFillIteratorValue != PlannedFillCountsByRefineryTagValue.end())
    {
        PlannedHarvesterDeltaValue += PlannedFillIteratorValue->second;
    }

    const std::unordered_map<Tag, int>::const_iterator PlannedReliefIteratorValue =
        PlannedReliefCountsByRefineryTagValue.find(RefineryTagValue);
    if (PlannedReliefIteratorValue != PlannedReliefCountsByRefineryTagValue.end())
    {
        PlannedHarvesterDeltaValue -= PlannedReliefIteratorValue->second;
    }

    return PlannedHarvesterDeltaValue;
}

}  // namespace

void TerranAgent::OnGameStart()
{
    CurrentStep = 0;
    ObservationPtr = Observation();
    if (!ObservationPtr)
    {
        SCLOG(LoggingVerbosity::error, "ERROR in TerranAgent::OnGameStart() - Observation() is null");
        return;
    }

    ExpansionLocations.clear();
    QueryInterface* QueryValue = Query();
    if (QueryValue)
    {
        const std::vector<Point3D> ExpansionLocationValues =
            search::CalculateExpansionLocations(ObservationPtr, QueryValue);
        ExpansionLocations.reserve(ExpansionLocationValues.size());
        for (const Point3D& ExpansionLocationValue : ExpansionLocationValues)
        {
            ExpansionLocations.push_back(Point2D(ExpansionLocationValue.x, ExpansionLocationValue.y));
        }
    }

    GameStateDescriptor.Reset();
    ExecutionTelemetry.Reset();
    PendingProductionRallyStructureTags.clear();
    CurrentWallGateState = EWallGateState::Unavailable;
    LastArmyExecutionOrderCount = 0U;

    const FFrameContext Frame = FFrameContext::Create(ObservationPtr, Query(), CurrentStep);
    UpdateAgentState(Frame);
    InitializeRampWallDescriptor(Frame);
    InitializeMainBaseLayoutDescriptor(Frame);
    RebuildGameStateDescriptor(Frame);
    PrintAgentState();
}

void TerranAgent::OnStep()
{
    ++CurrentStep;

    ObservationPtr = Observation();
    if (!ObservationPtr)
    {
        SCLOG(LoggingVerbosity::error, "ERROR in TerranAgent::OnStep() - Observation() is null");
        return;
    }

    const FFrameContext Frame = FFrameContext::Create(ObservationPtr, Query(), CurrentStep);

    UpdateAgentState(Frame);
    RebuildGameStateDescriptor(Frame);
    UpdateDispatchedSchedulerOrders(Frame);

    IntentBuffer.Reset();
    ProduceSchedulerIntents(Frame);
    ProduceProductionRallyIntents();
    ProduceWallGateIntents(Frame);
    ProduceWorkerHarvestIntents(Frame);
    ProduceRecoveryIntents(Frame);
    UpdateExecutionTelemetry(Frame);

    ResolvedIntents = IntentArbiter.Resolve(Frame, AgentState.UnitContainer, IntentBuffer);
    ExecuteResolvedIntents(Frame, ResolvedIntents);
    CaptureNewlyDispatchedSchedulerOrders(Frame);

    if (CurrentStep % 120 == 0)
    {
        PrintAgentState();
    }
}

void TerranAgent::OnGameEnd()
{
    sc2::renderer::Shutdown();
}

void TerranAgent::OnUnitIdle(const Unit* UnitPtr)
{
    if (!UnitPtr)
    {
        return;
    }

    if (UnitPtr->unit_type.ToType() == UNIT_TYPEID::TERRAN_SCV)
    {
        PendingRecoveryWorkers.insert(UnitPtr->tag);
    }
}

void TerranAgent::OnUnitCreated(const Unit* UnitPtr)
{
    if (UnitPtr == nullptr)
    {
        return;
    }

    if (IsProductionRallyStructureType(UnitPtr->unit_type.ToType()))
    {
        PendingProductionRallyStructureTags.insert(UnitPtr->tag);
    }
}

void TerranAgent::UpdateAgentState(const FFrameContext& Frame)
{
    if (!Frame.Observation)
    {
        SCLOG(LoggingVerbosity::error, "ERROR in TerranAgent::UpdateAgentState() - observation is null");
        return;
    }

    NeutralUnits = Frame.Observation->GetUnits(Unit::Alliance::Neutral);
    AgentState.Update(Frame);
}

void TerranAgent::InitializeRampWallDescriptor(const FFrameContext& Frame)
{
    GameStateDescriptor.RampWallDescriptor.Reset();
    GameStateDescriptor.MainBaseLayoutDescriptor.Reset();
    if (BuildPlacementService == nullptr || ObservationPtr == nullptr)
    {
        return;
    }

    FBuildPlacementContext BuildPlacementContextValue = CreateBuildPlacementContext();
    BuildPlacementContextValue.RampWallDescriptor.Reset();
    GameStateDescriptor.RampWallDescriptor =
        BuildPlacementService->GetRampWallDescriptor(Frame, BuildPlacementContextValue);

    if (!GameStateDescriptor.RampWallDescriptor.bIsValid)
    {
        ExecutionTelemetry.RecordWallDescriptorInvalid(CurrentStep, Frame.GameLoop);
    }
}

void TerranAgent::InitializeMainBaseLayoutDescriptor(const FFrameContext& Frame)
{
    GameStateDescriptor.MainBaseLayoutDescriptor.Reset();
    if (BuildPlacementService == nullptr || ObservationPtr == nullptr)
    {
        return;
    }

    FBuildPlacementContext BuildPlacementContextValue = CreateBuildPlacementContext();
    BuildPlacementContextValue.MainBaseLayoutDescriptor.Reset();
    GameStateDescriptor.MainBaseLayoutDescriptor =
        BuildPlacementService->GetMainBaseLayoutDescriptor(Frame, BuildPlacementContextValue);
}

void TerranAgent::RebuildGameStateDescriptor(const FFrameContext& Frame)
{
    (void)Frame;

    if (GameStateDescriptorBuilder)
    {
        GameStateDescriptorBuilder->RebuildGameStateDescriptor(CurrentStep, Frame.GameLoop, AgentState,
                                                               GameStateDescriptor);
    }
    else
    {
        GameStateDescriptor.CurrentStep = CurrentStep;
        GameStateDescriptor.CurrentGameLoop = Frame.GameLoop;
    }

    if (StrategicDirector)
    {
        StrategicDirector->UpdateGameStateDescriptor(GameStateDescriptor);
    }
    if (BuildPlanner)
    {
        BuildPlanner->ProduceBuildPlan(GameStateDescriptor, GameStateDescriptor.BuildPlanning);
    }
    if (ArmyPlanner)
    {
        ArmyPlanner->ProduceArmyPlan(GameStateDescriptor, GameStateDescriptor.ArmyState);
    }

    UpdateRallyAnchor();
}

void TerranAgent::UpdateRallyAnchor()
{
    if (!ObservationPtr)
    {
        return;
    }

    const FBuildPlacementContext BuildPlacementContextValue = CreateBuildPlacementContext();
    const Point2D BaseLocationValue = BuildPlacementContextValue.BaseLocation;
    BarracksRally = BuildPlacementService
                        ? BuildPlacementService->GetArmyAssemblyPoint(GameStateDescriptor, BuildPlacementContextValue)
                        : BaseLocationValue;
}

FBuildPlacementContext TerranAgent::CreateBuildPlacementContext() const
{
    FBuildPlacementContext BuildPlacementContextValue;
    if (!ObservationPtr)
    {
        return BuildPlacementContextValue;
    }

    const GameInfo& GameInfoValue = ObservationPtr->GetGameInfo();
    BuildPlacementContextValue.MapName = GameInfoValue.map_name;
    BuildPlacementContextValue.BaseLocation = Point2D(ObservationPtr->GetStartLocation());
    BuildPlacementContextValue.PlayableMin = GameInfoValue.playable_min;
    BuildPlacementContextValue.PlayableMax = GameInfoValue.playable_max;

    float BestDistanceSquaredValue = std::numeric_limits<float>::max();
    for (const Point2D& ExpansionLocationValue : ExpansionLocations)
    {
        const float DistanceSquaredValue =
            DistanceSquared2D(BuildPlacementContextValue.BaseLocation, ExpansionLocationValue);
        if (DistanceSquaredValue < 16.0f || DistanceSquaredValue >= BestDistanceSquaredValue)
        {
            continue;
        }

        BestDistanceSquaredValue = DistanceSquaredValue;
        BuildPlacementContextValue.NaturalLocation = ExpansionLocationValue;
    }

    BuildPlacementContextValue.RampWallDescriptor = GameStateDescriptor.RampWallDescriptor;
    BuildPlacementContextValue.MainBaseLayoutDescriptor = GameStateDescriptor.MainBaseLayoutDescriptor;

    return BuildPlacementContextValue;
}

void TerranAgent::PrintAgentState()
{
    AgentState.PrintStatus();

    std::cout << "Game Descriptor:\n";
    std::cout << "Step: " << GameStateDescriptor.CurrentStep
              << " | GameLoop: " << GameStateDescriptor.CurrentGameLoop << "\n";
    std::cout << "Plan: " << ToString(GameStateDescriptor.MacroState.ActiveGamePlan)
              << " | Phase: " << ToString(GameStateDescriptor.MacroState.ActiveMacroPhase)
              << " | Bases: " << GameStateDescriptor.MacroState.ActiveBaseCount << "/"
              << GameStateDescriptor.MacroState.DesiredBaseCount
              << " | Desired Armies: " << GameStateDescriptor.MacroState.DesiredArmyCount
              << " | Focus: " << ToString(GameStateDescriptor.MacroState.PrimaryProductionFocus) << "\n";
    std::cout << "Build Targets: "
              << "Workers " << GameStateDescriptor.BuildPlanning.DesiredWorkerCount
              << " | Orbitals " << GameStateDescriptor.BuildPlanning.DesiredOrbitalCommandCount
              << " | Refineries " << GameStateDescriptor.BuildPlanning.DesiredRefineryCount
              << " | Depots " << GameStateDescriptor.BuildPlanning.DesiredSupplyDepotCount
              << " | Barracks " << GameStateDescriptor.BuildPlanning.DesiredBarracksCount
              << " | Factory " << GameStateDescriptor.BuildPlanning.DesiredFactoryCount
              << " | Starport " << GameStateDescriptor.BuildPlanning.DesiredStarportCount
              << " | Marines " << GameStateDescriptor.BuildPlanning.DesiredMarineCount
              << " | Needs " << GameStateDescriptor.BuildPlanning.ActiveNeedCount << "\n";
    PrintGoalList("Immediate Goals", GameStateDescriptor.GoalSet.ImmediateGoals);
    std::cout << std::endl;
    PrintGoalList("Near Goals", GameStateDescriptor.GoalSet.NearTermGoals);
    std::cout << std::endl;
    PrintGoalList("Strategic Goals", GameStateDescriptor.GoalSet.StrategicGoals);
    std::cout << std::endl;
    std::cout << "Army Goals: ";
    if (GameStateDescriptor.ArmyState.ArmyGoals.empty())
    {
        std::cout << "None";
    }
    else
    {
        for (size_t ArmyIndexValue = 0U; ArmyIndexValue < GameStateDescriptor.ArmyState.ArmyGoals.size();
             ++ArmyIndexValue)
        {
            if (ArmyIndexValue > 0U)
            {
                std::cout << ", ";
            }

            std::cout << ToString(GameStateDescriptor.ArmyState.ArmyGoals[ArmyIndexValue]);
        }
    }
    std::cout << std::endl;
    std::cout << "Army Postures: ";
    if (GameStateDescriptor.ArmyState.ArmyPostures.empty())
    {
        std::cout << "None";
    }
    else
    {
        for (size_t ArmyIndexValue = 0U; ArmyIndexValue < GameStateDescriptor.ArmyState.ArmyPostures.size();
             ++ArmyIndexValue)
        {
            if (ArmyIndexValue > 0U)
            {
                std::cout << ", ";
            }

            std::cout << ToString(GameStateDescriptor.ArmyState.ArmyPostures[ArmyIndexValue]);
        }
    }
    std::cout << std::endl;
    std::cout << "Army Mission: ";
    if (GameStateDescriptor.ArmyState.ArmyMissions.empty())
    {
        std::cout << "None";
    }
    else
    {
        const FArmyMissionDescriptor& ArmyMissionDescriptorValue = GameStateDescriptor.ArmyState.ArmyMissions.front();
        std::cout << ToString(ArmyMissionDescriptorValue.MissionType)
                  << " | Goal " << ArmyMissionDescriptorValue.SourceGoalId
                  << " | Objective (" << ArmyMissionDescriptorValue.ObjectivePoint.x
                  << ", " << ArmyMissionDescriptorValue.ObjectivePoint.y << ")"
                  << " | Search " << ArmyMissionDescriptorValue.SearchExpansionOrdinal
                  << " | OrdersThisStep " << LastArmyExecutionOrderCount;
    }
    std::cout << std::endl;
    PrintPriorityTierQueueSummary("StrategicQueues", GameStateDescriptor.CommandAuthoritySchedulingState.StrategicQueues);
    std::cout << std::endl;
    PrintPriorityTierQueueSummary("PlanningQueues", GameStateDescriptor.CommandAuthoritySchedulingState.PlanningQueues);
    std::cout << std::endl;
    PrintPriorityTierQueueSummary("ArmyQueues", GameStateDescriptor.CommandAuthoritySchedulingState.ArmyQueues);
    std::cout << std::endl;
    PrintPriorityTierQueueSummary("SquadQueues", GameStateDescriptor.CommandAuthoritySchedulingState.SquadQueues);
    std::cout << std::endl;
    PrintReadyIntentQueueSummary(GameStateDescriptor.CommandAuthoritySchedulingState.ReadyIntentQueues);
    std::cout << std::endl;
    PrintWallState();
    const FMainBaseLayoutDescriptor& MainBaseLayoutDescriptorValue = GameStateDescriptor.MainBaseLayoutDescriptor;
    std::cout << "Main Layout: " << (MainBaseLayoutDescriptorValue.bIsValid ? "Valid" : "Invalid");
    if (MainBaseLayoutDescriptorValue.bIsValid)
    {
        std::cout << " | Anchor (" << MainBaseLayoutDescriptorValue.LayoutAnchorPoint.x
                  << ", " << MainBaseLayoutDescriptorValue.LayoutAnchorPoint.y << ")";
        PrintProductionRailSlots(MainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots,
                                 AgentState.UnitContainer.ControlledUnits);
        PrintMainLayoutSlotFamily("Barracks", MainBaseLayoutDescriptorValue.BarracksWithAddonSlots);
        PrintMainLayoutSlotFamily("Factory", MainBaseLayoutDescriptorValue.FactoryWithAddonSlots);
        PrintMainLayoutSlotFamily("Starport", MainBaseLayoutDescriptorValue.StarportWithAddonSlots);
    }
    std::cout << std::endl;
    std::cout << "Execution Telemetry: "
              << "SupplyBlock " << ToString(ExecutionTelemetry.SupplyBlockState)
              << " (" << ExecutionTelemetry.GetCurrentSupplyBlockDurationGameLoops(GameStateDescriptor.CurrentGameLoop)
              << " loops)"
              << " | MineralBank " << ToString(ExecutionTelemetry.MineralBankState)
              << " (" << ExecutionTelemetry.GetCurrentMineralBankDurationGameLoops(GameStateDescriptor.CurrentGameLoop)
              << " loops)"
              << " | Conflicts " << ExecutionTelemetry.TotalActorIntentConflictCount
              << " | IdleProduction " << ExecutionTelemetry.TotalIdleProductionConflictCount
              << " | Deferrals " << ExecutionTelemetry.TotalSchedulerOrderDeferralCount << "\n";
    std::cout << "Recent Execution Events: ";
    if (ExecutionTelemetry.RecentEvents.empty())
    {
        std::cout << "None";
    }
    else
    {
        for (size_t EventIndexValue = 0U; EventIndexValue < ExecutionTelemetry.RecentEvents.size(); ++EventIndexValue)
        {
            const FExecutionEventRecord& ExecutionEventRecordValue =
                ExecutionTelemetry.RecentEvents[EventIndexValue];
            if (EventIndexValue > 0U)
            {
                std::cout << " | ";
            }

            std::cout << ToString(ExecutionEventRecordValue.EventType)
                      << "@GL" << ExecutionEventRecordValue.GameLoop;
            if (ExecutionEventRecordValue.ActorTag != NullTag)
            {
                std::cout << " Actor " << ExecutionEventRecordValue.ActorTag;
            }
            if (ExecutionEventRecordValue.OrderId != 0U)
            {
                std::cout << " Order " << ExecutionEventRecordValue.OrderId;
            }
            if (ExecutionEventRecordValue.PlanStepId != 0U)
            {
                std::cout << " PlanStep " << ExecutionEventRecordValue.PlanStepId;
            }
            if (ExecutionEventRecordValue.AbilityId != ABILITY_ID::INVALID)
            {
                std::cout << " Ability " << static_cast<uint32_t>(ExecutionEventRecordValue.AbilityId);
            }
            if (ExecutionEventRecordValue.UnitTypeId != UNIT_TYPEID::INVALID)
            {
                std::cout << " Unit " << static_cast<uint32_t>(ExecutionEventRecordValue.UnitTypeId);
            }
            if (ExecutionEventRecordValue.DeferralReason != ECommandOrderDeferralReason::None)
            {
                std::cout << " Reason " << ToString(ExecutionEventRecordValue.DeferralReason);
            }
            if (ExecutionEventRecordValue.MetricValue > 0U)
            {
                std::cout << " Metric " << ExecutionEventRecordValue.MetricValue;
            }
        }
    }
    std::cout << std::endl;
}

void TerranAgent::PrintWallState() const
{
    if (ObservationPtr == nullptr)
    {
        return;
    }

    const FRampWallDescriptor& RampWallDescriptorValue = GameStateDescriptor.RampWallDescriptor;
    std::cout << "Wall: "
              << (RampWallDescriptorValue.bIsValid ? "Valid" : "Invalid")
              << " | Gate " << ToString(CurrentWallGateState);
    if (!RampWallDescriptorValue.bIsValid)
    {
        std::cout << std::endl;
        return;
    }

    const Units SelfUnitsValue = ObservationPtr->GetUnits(Unit::Alliance::Self);
    const Unit* LeftWallUnitValue = FindWallStructureForSlot(SelfUnitsValue, RampWallDescriptorValue.LeftDepotSlot);
    const Unit* CenterWallUnitValue = FindWallStructureForSlot(SelfUnitsValue, RampWallDescriptorValue.BarracksSlot);
    const Unit* RightWallUnitValue = FindWallStructureForSlot(SelfUnitsValue, RampWallDescriptorValue.RightDepotSlot);

    std::cout << " | Left " << GetWallSlotOccupancyLabel(LeftWallUnitValue)
              << " | Center " << GetWallSlotOccupancyLabel(CenterWallUnitValue)
              << " | Right " << GetWallSlotOccupancyLabel(RightWallUnitValue)
              << std::endl;
}

void TerranAgent::UpdateExecutionTelemetry(const FFrameContext& Frame)
{
    const bool IsSupplyBlockedValue = ObservationPtr != nullptr && ObservationPtr->GetFoodCap() < 200U &&
                                      GameStateDescriptor.BuildPlanning.AvailableSupply == 0U;
    ExecutionTelemetry.UpdateSupplyBlockState(GetExecutionConditionState(IsSupplyBlockedValue), CurrentStep,
                                              Frame.GameLoop);

    const bool IsBankingMineralsValue = GameStateDescriptor.BuildPlanning.AvailableMinerals >= 400U;
    ExecutionTelemetry.UpdateMineralBankState(GetExecutionConditionState(IsBankingMineralsValue), CurrentStep,
                                              Frame.GameLoop, GameStateDescriptor.BuildPlanning.AvailableMinerals);

    std::unordered_map<Tag, FUnitIntent> FirstIntentByActor;
    for (const FUnitIntent& IntentValue : IntentBuffer.Intents)
    {
        if (IntentValue.ActorTag == NullTag)
        {
            continue;
        }

        const std::unordered_map<Tag, FUnitIntent>::const_iterator FoundIntentValue =
            FirstIntentByActor.find(IntentValue.ActorTag);
        if (FoundIntentValue == FirstIntentByActor.end())
        {
            FirstIntentByActor.emplace(IntentValue.ActorTag, IntentValue);
            continue;
        }

        if (!FoundIntentValue->second.Matches(IntentValue))
        {
            ExecutionTelemetry.RecordActorIntentConflict(CurrentStep, Frame.GameLoop, IntentValue.ActorTag,
                                                         IntentValue.Ability, IntentValue.Domain);
        }
    }

    const uint32_t PlannedMarineCountValue = AgentState.Units.GetUnitCount(UNIT_TYPEID::TERRAN_MARINE) +
                                             AgentState.Units.GetUnitsInConstruction(UNIT_TYPEID::TERRAN_MARINE) +
                                             CountOrdersAndIntentsForAbility(ABILITY_ID::TRAIN_MARINE);
    const bool HasMarineDemandValue =
        PlannedMarineCountValue < GameStateDescriptor.BuildPlanning.DesiredMarineCount;
    if (ObservationPtr != nullptr && HasMarineDemandValue)
    {
        const Units BarracksUnitsValue =
            ObservationPtr->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_BARRACKS));
        for (const Unit* BarracksUnitValue : BarracksUnitsValue)
        {
            if (BarracksUnitValue == nullptr || BarracksUnitValue->build_progress < 1.0f ||
                !BarracksUnitValue->orders.empty() || IntentBuffer.HasIntentForActor(BarracksUnitValue->tag))
            {
                continue;
            }

            ExecutionTelemetry.RecordIdleProductionConflict(CurrentStep, Frame.GameLoop, BarracksUnitValue->tag,
                                                            UNIT_TYPEID::TERRAN_BARRACKS,
                                                            ABILITY_ID::TRAIN_MARINE);
        }
    }

    const uint32_t PlannedSCVCountValue = AgentState.Units.GetWorkerCount() +
                                          AgentState.Units.GetUnitsInConstruction(UNIT_TYPEID::TERRAN_SCV) +
                                          CountOrdersAndIntentsForAbility(ABILITY_ID::TRAIN_SCV);
    const bool HasWorkerDemandValue =
        PlannedSCVCountValue < std::max<uint32_t>(GameStateDescriptor.BuildPlanning.DesiredWorkerCount,
                                                  static_cast<uint32_t>(AgentState.Buildings.GetTownHallCount() * 20U));
    if (HasWorkerDemandValue)
    {
        const Unit* TownHallUnitValue = AgentState.UnitContainer.GetFirstIdleTownHall();
        if (TownHallUnitValue != nullptr && !IntentBuffer.HasIntentForActor(TownHallUnitValue->tag))
        {
            ExecutionTelemetry.RecordIdleProductionConflict(CurrentStep, Frame.GameLoop, TownHallUnitValue->tag,
                                                            TownHallUnitValue->unit_type.ToType(),
                                                            ABILITY_ID::TRAIN_SCV);
        }
    }

    const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptor.CommandAuthoritySchedulingState;
    const size_t OrderCountValue = CommandAuthoritySchedulingStateValue.OrderIds.size();
    for (size_t OrderIndexValue = 0U; OrderIndexValue < OrderCountValue; ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.LastDeferralSteps[OrderIndexValue] != CurrentStep ||
            CommandAuthoritySchedulingStateValue.LastDeferralReasons[OrderIndexValue] ==
                ECommandOrderDeferralReason::None)
        {
            continue;
        }

        ExecutionTelemetry.RecordSchedulerOrderDeferred(
            CurrentStep, Frame.GameLoop, CommandAuthoritySchedulingStateValue.OrderIds[OrderIndexValue],
            CommandAuthoritySchedulingStateValue.PlanStepIds[OrderIndexValue],
            CommandAuthoritySchedulingStateValue.ActorTags[OrderIndexValue],
            CommandAuthoritySchedulingStateValue.AbilityIds[OrderIndexValue],
            CommandAuthoritySchedulingStateValue.IntentDomains[OrderIndexValue],
            CommandAuthoritySchedulingStateValue.LastDeferralReasons[OrderIndexValue]);
    }
}

void TerranAgent::ProduceRecoveryIntents(const FFrameContext& Frame)
{
    (void)Frame;

    std::unordered_set<Tag> RecoveryCandidates = std::move(PendingRecoveryWorkers);
    PendingRecoveryWorkers.clear();

    for (const Unit* Worker : AgentState.UnitContainer.GetWorkers())
    {
        if (Worker && Worker->orders.empty())
        {
            RecoveryCandidates.insert(Worker->tag);
        }
    }

    for (Tag WorkerTag : RecoveryCandidates)
    {
        if (IntentBuffer.HasIntentForActor(WorkerTag))
        {
            continue;
        }

        const Unit* Worker = AgentState.UnitContainer.GetUnitByTag(WorkerTag);
        if (!Worker || !Worker->orders.empty())
        {
            continue;
        }

        const Unit* NearestMineralPatch = FindNearestMineralPatch(Worker->pos);
        if (NearestMineralPatch)
        {
            IntentBuffer.Add(FUnitIntent::CreateUnitTarget(Worker->tag, ABILITY_ID::SMART, NearestMineralPatch->tag,
                                                           300, EIntentDomain::Recovery));
            continue;
        }

        IntentBuffer.Add(FUnitIntent::CreatePointTarget(Worker->tag, ABILITY_ID::ATTACK_ATTACK,
                                                        GetEnemyTargetLocation(), 300, EIntentDomain::Recovery,
                                                        true));
    }
}

void TerranAgent::ProduceSchedulerIntents(const FFrameContext& Frame)
{
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptor.CommandAuthoritySchedulingState;

    CommandAuthorityProcessor.ProcessSchedulerStep(GameStateDescriptor);
    if (CommandTaskPriorityService != nullptr)
    {
        CommandTaskPriorityService->UpdateTaskPriorities(GameStateDescriptor);
    }

    if (EconomyProductionOrderExpander != nullptr && BuildPlacementService != nullptr)
    {
        CommandAuthoritySchedulingStateValue.BeginMutationBatch();
        EconomyProductionOrderExpander->ExpandEconomyAndProductionOrders(
            Frame, AgentState, GameStateDescriptor, IntentBuffer, *BuildPlacementService, ExpansionLocations);
        if (CommandTaskPriorityService != nullptr)
        {
            CommandTaskPriorityService->UpdateTaskPriorities(GameStateDescriptor);
        }
        CommandAuthoritySchedulingStateValue.EndMutationBatch();
    }

    if (ArmyOrderExpander != nullptr)
    {
        CommandAuthoritySchedulingStateValue.BeginMutationBatch();
        ArmyOrderExpander->ExpandArmyOrders(Frame, AgentState, GameStateDescriptor, ExpansionLocations, BarracksRally,
                                            GameStateDescriptor.CommandAuthoritySchedulingState);
        if (ArmyPlanner != nullptr)
        {
            ArmyPlanner->ProduceArmyPlan(GameStateDescriptor, GameStateDescriptor.ArmyState);
        }
        if (CommandTaskPriorityService != nullptr)
        {
            CommandTaskPriorityService->UpdateTaskPriorities(GameStateDescriptor);
        }
        CommandAuthoritySchedulingStateValue.EndMutationBatch();
    }
    else if (ArmyPlanner != nullptr)
    {
        ArmyPlanner->ProduceArmyPlan(GameStateDescriptor, GameStateDescriptor.ArmyState);
    }

    if (SquadOrderExpander != nullptr)
    {
        CommandAuthoritySchedulingStateValue.BeginMutationBatch();
        SquadOrderExpander->ExpandSquadOrders(Frame, AgentState, GameStateDescriptor, BarracksRally,
                                              GameStateDescriptor.CommandAuthoritySchedulingState);
        if (CommandTaskPriorityService != nullptr)
        {
            CommandTaskPriorityService->UpdateTaskPriorities(GameStateDescriptor);
        }
        CommandAuthoritySchedulingStateValue.EndMutationBatch();
    }

    if (UnitExecutionPlanner != nullptr)
    {
        CommandAuthoritySchedulingStateValue.BeginMutationBatch();
        LastArmyExecutionOrderCount = UnitExecutionPlanner->ExpandUnitExecutionOrders(
            Frame, AgentState, GameStateDescriptor, BarracksRally, GameStateDescriptor.CommandAuthoritySchedulingState);
        if (CommandTaskPriorityService != nullptr)
        {
            CommandTaskPriorityService->UpdateTaskPriorities(GameStateDescriptor);
        }
        CommandAuthoritySchedulingStateValue.EndMutationBatch();
    }
    else
    {
        LastArmyExecutionOrderCount = 0U;
    }

    IntentSchedulingService.DrainReadyIntents(GameStateDescriptor.CommandAuthoritySchedulingState, IntentBuffer,
                                              GameStateDescriptor.CommandAuthoritySchedulingState.MaxUnitIntentsPerStep);
}

void TerranAgent::ProduceWallGateIntents(const FFrameContext& Frame)
{
    if (ObservationPtr == nullptr || WallGateController == nullptr)
    {
        return;
    }

    const Units SelfUnitsValue = ObservationPtr->GetUnits(Unit::Alliance::Self);
    const Units EnemyUnitsValue = ObservationPtr->GetUnits(Unit::Alliance::Enemy);
    const EWallGateState DesiredWallGateStateValue = WallGateController->EvaluateDesiredWallGateState(
        SelfUnitsValue, EnemyUnitsValue, GameStateDescriptor.RampWallDescriptor);
    if (DesiredWallGateStateValue == EWallGateState::Closed && CurrentWallGateState != EWallGateState::Closed)
    {
        ExecutionTelemetry.RecordWallThreatDetected(CurrentStep, Frame.GameLoop);
    }

    if (DesiredWallGateStateValue != CurrentWallGateState)
    {
        WallGateController->ProduceWallGateIntents(SelfUnitsValue, GameStateDescriptor.RampWallDescriptor,
                                                   DesiredWallGateStateValue, IntentBuffer);
        switch (DesiredWallGateStateValue)
        {
            case EWallGateState::Open:
                ExecutionTelemetry.RecordWallOpened(CurrentStep, Frame.GameLoop);
                break;
            case EWallGateState::Closed:
                ExecutionTelemetry.RecordWallClosed(CurrentStep, Frame.GameLoop);
                break;
            case EWallGateState::Unavailable:
            default:
                break;
        }
    }

    CurrentWallGateState = DesiredWallGateStateValue;
}

void TerranAgent::ProduceWorkerHarvestIntents(const FFrameContext& Frame)
{
    if (Frame.Observation == nullptr)
    {
        return;
    }

    const Units RefineryUnitsValue =
        Frame.Observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_REFINERY));
    std::unordered_set<Tag> ReservedWorkerTagsValue;
    std::unordered_map<Tag, int> PlannedFillCountsByRefineryTagValue;
    std::unordered_map<Tag, int> PlannedReliefCountsByRefineryTagValue;

    for (const Unit* RefineryUnitValue : RefineryUnitsValue)
    {
        if (RefineryUnitValue == nullptr || RefineryUnitValue->build_progress < 1.0f ||
            RefineryUnitValue->ideal_harvesters <= 0 || RefineryUnitValue->vespene_contents <= 0)
        {
            continue;
        }

        const int CommittedHarvesterCountValue =
            GetCommittedHarvesterCountForRefinery(*Frame.Observation, AgentState, RefineryUnitValue->tag);
        const int EffectiveHarvesterCountValue =
            std::max(RefineryUnitValue->assigned_harvesters, CommittedHarvesterCountValue) +
            GetPlannedHarvesterDeltaForRefinery(PlannedFillCountsByRefineryTagValue,
                                                PlannedReliefCountsByRefineryTagValue, RefineryUnitValue->tag);
        const int MissingHarvesterCountValue =
            std::max(0, RefineryUnitValue->ideal_harvesters - EffectiveHarvesterCountValue);
        for (int MissingHarvesterIndexValue = 0; MissingHarvesterIndexValue < MissingHarvesterCountValue;
             ++MissingHarvesterIndexValue)
        {
            const Unit* WorkerUnitValue = SelectWorkerForRefinery(
                *Frame.Observation, AgentState, GameStateDescriptor.CommandAuthoritySchedulingState, IntentBuffer,
                *RefineryUnitValue, ReservedWorkerTagsValue);
            if (WorkerUnitValue == nullptr)
            {
                break;
            }

            IntentBuffer.Add(FUnitIntent::CreateUnitTarget(WorkerUnitValue->tag, ABILITY_ID::HARVEST_GATHER,
                                                           RefineryUnitValue->tag, GasHarvestIntentPriorityValue,
                                                           EIntentDomain::Recovery));
            ReservedWorkerTagsValue.insert(WorkerUnitValue->tag);
            ++PlannedFillCountsByRefineryTagValue[RefineryUnitValue->tag];
        }
    }

    for (const Unit* RefineryUnitValue : RefineryUnitsValue)
    {
        if (RefineryUnitValue == nullptr || RefineryUnitValue->build_progress < 1.0f ||
            RefineryUnitValue->ideal_harvesters <= 0 || RefineryUnitValue->vespene_contents <= 0)
        {
            continue;
        }

        const int CommittedHarvesterCountValue =
            GetCommittedHarvesterCountForRefinery(*Frame.Observation, AgentState, RefineryUnitValue->tag);
        const int EffectiveHarvesterCountValue =
            std::max(RefineryUnitValue->assigned_harvesters, CommittedHarvesterCountValue) +
            GetPlannedHarvesterDeltaForRefinery(PlannedFillCountsByRefineryTagValue,
                                                PlannedReliefCountsByRefineryTagValue, RefineryUnitValue->tag);
        const int ExcessHarvesterCountValue =
            std::max(0, EffectiveHarvesterCountValue - RefineryUnitValue->ideal_harvesters);
        for (int ExcessHarvesterIndexValue = 0; ExcessHarvesterIndexValue < ExcessHarvesterCountValue;
             ++ExcessHarvesterIndexValue)
        {
            const Unit* WorkerUnitValue = SelectWorkerForGasRelief(
                *Frame.Observation, AgentState, GameStateDescriptor.CommandAuthoritySchedulingState, IntentBuffer,
                *RefineryUnitValue, ReservedWorkerTagsValue);
            if (WorkerUnitValue == nullptr)
            {
                break;
            }

            const Unit* MineralPatchValue = FindNearestMineralPatch(WorkerUnitValue->pos);
            if (MineralPatchValue == nullptr)
            {
                break;
            }

            IntentBuffer.Add(FUnitIntent::CreateUnitTarget(WorkerUnitValue->tag, ABILITY_ID::SMART,
                                                           MineralPatchValue->tag, GasReliefIntentPriorityValue,
                                                           EIntentDomain::Recovery));
            ReservedWorkerTagsValue.insert(WorkerUnitValue->tag);
            ++PlannedReliefCountsByRefineryTagValue[RefineryUnitValue->tag];
        }
    }
}

void TerranAgent::ProduceProductionRallyIntents()
{
    constexpr uint64_t FullRefreshCadenceStepCount = 120U;
    const bool ShouldRunFullRefreshValue = (CurrentStep % FullRefreshCadenceStepCount) == 0U;
    if (!ShouldRunFullRefreshValue && PendingProductionRallyStructureTags.empty())
    {
        return;
    }

    for (const Unit* ControlledUnitValue : AgentState.UnitContainer.ControlledUnits)
    {
        if (ControlledUnitValue == nullptr || ControlledUnitValue->build_progress < 1.0f)
        {
            continue;
        }

        const UNIT_TYPEID UnitTypeIdValue = ControlledUnitValue->unit_type.ToType();
        if (!IsProductionRallyStructureType(UnitTypeIdValue))
        {
            continue;
        }

        const std::unordered_set<Tag>::const_iterator PendingStructureIteratorValue =
            PendingProductionRallyStructureTags.find(ControlledUnitValue->tag);
        const bool HasPendingRefreshValue = PendingStructureIteratorValue != PendingProductionRallyStructureTags.end();
        if (!ShouldRunFullRefreshValue && !HasPendingRefreshValue)
        {
            continue;
        }

        if (IntentBuffer.HasIntentForActor(ControlledUnitValue->tag))
        {
            continue;
        }

        const AbilityID RallyAbilityValue = GetProductionStructureRallyAbility(UnitTypeIdValue);
        if (RallyAbilityValue == ABILITY_ID::INVALID)
        {
            continue;
        }

        IntentBuffer.Add(FUnitIntent::CreatePointTarget(ControlledUnitValue->tag, RallyAbilityValue, BarracksRally, 5,
                                                        EIntentDomain::UnitProduction));
        if (HasPendingRefreshValue)
        {
            PendingProductionRallyStructureTags.erase(ControlledUnitValue->tag);
        }
    }
}

void TerranAgent::ExecuteResolvedIntents(const FFrameContext& Frame, const std::vector<FUnitIntent>& Intents)
{
    (void)Frame;

    for (const FUnitIntent& Intent : Intents)
    {
        switch (Intent.TargetKind)
        {
            case EIntentTargetKind::None:
                Actions()->UnitCommand(Intent.ActorTag, Intent.Ability, Intent.Queued);
                break;
            case EIntentTargetKind::Point:
                Actions()->UnitCommand(Intent.ActorTag, Intent.Ability, Intent.TargetPoint, Intent.Queued);
                break;
            case EIntentTargetKind::Unit:
                Actions()->UnitCommand(Intent.ActorTag, Intent.Ability, Intent.TargetUnitTag, Intent.Queued);
                break;
            default:
                break;
        }
    }
}

void TerranAgent::UpdateDispatchedSchedulerOrders(const FFrameContext& Frame)
{
    (void)Frame;

    constexpr uint64_t DispatchConfirmationTimeoutGameLoopsValue = 96U;
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptor.CommandAuthoritySchedulingState;
    CommandAuthoritySchedulingStateValue.BeginMutationBatch();
    const size_t OrderCountValue = CommandAuthoritySchedulingStateValue.OrderIds.size();
    for (size_t OrderIndexValue = 0U; OrderIndexValue < OrderCountValue; ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] != ECommandAuthorityLayer::UnitExecution ||
            CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue] != EOrderLifecycleState::Dispatched)
        {
            continue;
        }

        const FCommandOrderRecord CommandOrderRecordValue =
            CommandAuthoritySchedulingStateValue.GetOrderRecord(OrderIndexValue);
        const uint32_t CurrentObservedCountValue = GetObservedCountForOrder(CommandOrderRecordValue);
        const uint32_t CurrentObservedInConstructionCountValue =
            GetObservedInConstructionCountForOrder(CommandOrderRecordValue);
        if (CurrentObservedCountValue > CommandOrderRecordValue.ObservedCountAtDispatch ||
            CurrentObservedInConstructionCountValue >
                CommandOrderRecordValue.ObservedInConstructionCountAtDispatch)
        {
            CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(CommandOrderRecordValue.OrderId,
                                                                       EOrderLifecycleState::Completed);
            continue;
        }

        const Unit* ActorUnitValue = AgentState.UnitContainer.GetUnitByTag(CommandOrderRecordValue.ActorTag);
        if (ActorUnitValue == nullptr && CommandOrderRecordValue.DispatchGameLoop > 0U &&
            GameStateDescriptor.CurrentGameLoop > CommandOrderRecordValue.DispatchGameLoop)
        {
            CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(CommandOrderRecordValue.OrderId,
                                                                       EOrderLifecycleState::Aborted);
            continue;
        }

        if (HasProducerConfirmedDispatchedOrder(CommandOrderRecordValue, ActorUnitValue))
        {
            CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(CommandOrderRecordValue.OrderId,
                                                                       EOrderLifecycleState::Completed);
            continue;
        }

        if (CommandOrderRecordValue.DispatchGameLoop == 0U ||
            GameStateDescriptor.CurrentGameLoop <
                (CommandOrderRecordValue.DispatchGameLoop + DispatchConfirmationTimeoutGameLoopsValue))
        {
            continue;
        }

        CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(CommandOrderRecordValue.OrderId,
                                                                   EOrderLifecycleState::Aborted);
    }
    CommandAuthoritySchedulingStateValue.CompactTerminalOrders();
    CommandAuthoritySchedulingStateValue.EndMutationBatch();
}

void TerranAgent::CaptureNewlyDispatchedSchedulerOrders(const FFrameContext& Frame)
{
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptor.CommandAuthoritySchedulingState;
    const size_t OrderCountValue = CommandAuthoritySchedulingStateValue.OrderIds.size();
    for (size_t OrderIndexValue = 0U; OrderIndexValue < OrderCountValue; ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] != ECommandAuthorityLayer::UnitExecution ||
            CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue] != EOrderLifecycleState::Dispatched ||
            CommandAuthoritySchedulingStateValue.DispatchAttemptCounts[OrderIndexValue] > 0U)
        {
            continue;
        }

        const FCommandOrderRecord CommandOrderRecordValue =
            CommandAuthoritySchedulingStateValue.GetOrderRecord(OrderIndexValue);
        CommandAuthoritySchedulingStateValue.SetOrderDispatchState(
            CommandOrderRecordValue.OrderId, CurrentStep, Frame.GameLoop, GetObservedCountForOrder(CommandOrderRecordValue),
            GetObservedInConstructionCountForOrder(CommandOrderRecordValue));
    }
}

uint32_t TerranAgent::CountOrdersAndIntentsForAbility(const ABILITY_ID AbilityIdValue) const
{
    uint32_t OrderCountValue = 0U;

    for (const Unit* UnitValue : AgentState.UnitContainer.ControlledUnits)
    {
        if (!UnitValue)
        {
            continue;
        }

        for (const UnitOrder& OrderValue : UnitValue->orders)
        {
            if (OrderValue.ability_id == AbilityIdValue)
            {
                ++OrderCountValue;
            }
        }
    }

    for (const FUnitIntent& IntentValue : IntentBuffer.Intents)
    {
        if (IntentValue.Ability == AbilityIdValue)
        {
            ++OrderCountValue;
        }
    }

    return OrderCountValue;
}

uint32_t TerranAgent::GetObservedCountForOrder(const FCommandOrderRecord& CommandOrderRecordValue) const
{
    switch (CommandOrderRecordValue.ResultUnitTypeId)
    {
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
            return GameStateDescriptor.BuildPlanning.ObservedTownHallCount;
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
            return GameStateDescriptor.BuildPlanning.ObservedOrbitalCommandCount;
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOT:
            return static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_SUPPLYDEPOT)]) +
                   static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED)]);
        case UNIT_TYPEID::TERRAN_BARRACKS:
            return static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_BARRACKS)]) +
                   static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_BARRACKSFLYING)]);
        case UNIT_TYPEID::TERRAN_FACTORY:
            return static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_FACTORY)]) +
                   static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_FACTORYFLYING)]);
        case UNIT_TYPEID::TERRAN_STARPORT:
            return static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_STARPORT)]) +
                   static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_STARPORTFLYING)]);
        case UNIT_TYPEID::TERRAN_REFINERY:
            return static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_REFINERY)]) +
                   static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_REFINERYRICH)]);
        case UNIT_TYPEID::TERRAN_HELLION:
            return static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedUnitCounts[GetTerranUnitTypeIndex(
                       UNIT_TYPEID::TERRAN_HELLION)]) +
                   static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedUnitCounts[GetTerranUnitTypeIndex(
                       UNIT_TYPEID::TERRAN_HELLIONTANK)]);
        case UNIT_TYPEID::TERRAN_LIBERATOR:
            return static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedUnitCounts[GetTerranUnitTypeIndex(
                       UNIT_TYPEID::TERRAN_LIBERATOR)]) +
                   static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedUnitCounts[GetTerranUnitTypeIndex(
                       UNIT_TYPEID::TERRAN_LIBERATORAG)]);
        case UNIT_TYPEID::TERRAN_SIEGETANK:
            return static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedUnitCounts[GetTerranUnitTypeIndex(
                       UNIT_TYPEID::TERRAN_SIEGETANK)]) +
                   static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedUnitCounts[GetTerranUnitTypeIndex(
                       UNIT_TYPEID::TERRAN_SIEGETANKSIEGED)]);
        default:
            break;
    }

    if (IsTerranBuilding(CommandOrderRecordValue.ResultUnitTypeId))
    {
        const size_t BuildingTypeIndexValue = GetTerranBuildingTypeIndex(CommandOrderRecordValue.ResultUnitTypeId);
        return IsTerranBuildingTypeIndexValid(BuildingTypeIndexValue)
                   ? static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingCounts[BuildingTypeIndexValue])
                   : 0U;
    }

    const size_t UnitTypeIndexValue = GetTerranUnitTypeIndex(CommandOrderRecordValue.ResultUnitTypeId);
    return IsTerranUnitTypeIndexValid(UnitTypeIndexValue)
               ? static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedUnitCounts[UnitTypeIndexValue])
               : 0U;
}

uint32_t TerranAgent::GetObservedInConstructionCountForOrder(const FCommandOrderRecord& CommandOrderRecordValue) const
{
    if (CommandOrderRecordValue.ResultUnitTypeId == UNIT_TYPEID::TERRAN_COMMANDCENTER)
    {
        return static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingsInConstruction[
            GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_COMMANDCENTER)]);
    }

    if (IsTerranBuilding(CommandOrderRecordValue.ResultUnitTypeId))
    {
        const size_t BuildingTypeIndexValue = GetTerranBuildingTypeIndex(CommandOrderRecordValue.ResultUnitTypeId);
        return IsTerranBuildingTypeIndexValid(BuildingTypeIndexValue)
                   ? static_cast<uint32_t>(
                         GameStateDescriptor.BuildPlanning.ObservedBuildingsInConstruction[BuildingTypeIndexValue])
                   : 0U;
    }

    const size_t UnitTypeIndexValue = GetTerranUnitTypeIndex(CommandOrderRecordValue.ResultUnitTypeId);
    return IsTerranUnitTypeIndexValid(UnitTypeIndexValue)
               ? static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedUnitsInConstruction[UnitTypeIndexValue])
               : 0U;
}

bool TerranAgent::HasProducerConfirmedDispatchedOrder(const FCommandOrderRecord& CommandOrderRecordValue,
                                                      const Unit* ActorUnitValue) const
{
    if (ActorUnitValue == nullptr)
    {
        return false;
    }

    if (CommandOrderRecordValue.AbilityId == ABILITY_ID::MORPH_ORBITALCOMMAND &&
        ActorUnitValue->unit_type.ToType() == UNIT_TYPEID::TERRAN_ORBITALCOMMAND)
    {
        return true;
    }

    if ((CommandOrderRecordValue.AbilityId == ABILITY_ID::BUILD_REACTOR_BARRACKS ||
         CommandOrderRecordValue.AbilityId == ABILITY_ID::BUILD_TECHLAB_FACTORY) &&
        ActorUnitValue->add_on_tag != NullTag)
    {
        return true;
    }

    for (const UnitOrder& UnitOrderValue : ActorUnitValue->orders)
    {
        if (UnitOrderValue.ability_id == CommandOrderRecordValue.AbilityId)
        {
            return true;
        }
    }

    return false;
}

const Unit* TerranAgent::FindNearestMineralPatch(const Point2D& Origin)
{
    float NearestDistance = std::numeric_limits<float>::max();
    const Unit* Target = nullptr;
    const IsMineralPatch MineralFilter;

    for (const Unit* NeutralUnit : NeutralUnits)
    {
        if (!NeutralUnit || !MineralFilter(*NeutralUnit))
        {
            continue;
        }

        const Point2D Diff = Point2D(NeutralUnit->pos) - Origin;
        const float DistanceValue = Dot2D(Diff, Diff);
        if (DistanceValue <= NearestDistance)
        {
            NearestDistance = DistanceValue;
            Target = NeutralUnit;
        }
    }

    return Target;
}

Point2D TerranAgent::GetEnemyTargetLocation() const
{
    if (ObservationPtr && !ObservationPtr->GetGameInfo().enemy_start_locations.empty())
    {
        return ObservationPtr->GetGameInfo().enemy_start_locations.front();
    }

    if (ObservationPtr)
    {
        return Point2D(ObservationPtr->GetStartLocation());
    }

    return Point2D();
}

}  // namespace sc2
