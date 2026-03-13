#include "terran.h"

#include "sc2lib/sc2_search.h"

namespace sc2
{
namespace
{

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

bool DoesArmyPostureUseAssemblyPoint(const EArmyPosture ArmyPostureValue)
{
    switch (ArmyPostureValue)
    {
        case EArmyPosture::Assemble:
        case EArmyPosture::Hold:
        case EArmyPosture::Regroup:
            return true;
        default:
            return false;
    }
}

bool IsAssemblyCombatUnitType(const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_MARINE:
        case UNIT_TYPEID::TERRAN_MARAUDER:
        case UNIT_TYPEID::TERRAN_HELLION:
        case UNIT_TYPEID::TERRAN_HELLIONTANK:
        case UNIT_TYPEID::TERRAN_CYCLONE:
        case UNIT_TYPEID::TERRAN_SIEGETANK:
        case UNIT_TYPEID::TERRAN_WIDOWMINE:
        case UNIT_TYPEID::TERRAN_MEDIVAC:
        case UNIT_TYPEID::TERRAN_LIBERATOR:
        case UNIT_TYPEID::TERRAN_VIKINGFIGHTER:
        case UNIT_TYPEID::TERRAN_VIKINGASSAULT:
            return true;
        default:
            return false;
    }
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

    const FFrameContext Frame = FFrameContext::Create(ObservationPtr, Query(), CurrentStep);
    UpdateAgentState(Frame);
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
    ProduceSchedulerOpeningIntents(Frame);
    ProduceRecoveryIntents(Frame);
    ProduceArmyIntents(Frame);
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

    BuildPlacementContextValue.BaseLocation = Point2D(ObservationPtr->GetStartLocation());

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
              << " | Desired Armies: " << GameStateDescriptor.MacroState.DesiredArmyCount << "\n";
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

void TerranAgent::ProduceSchedulerOpeningIntents(const FFrameContext& Frame)
{
    CommandAuthorityProcessor.ProcessSchedulerStep(GameStateDescriptor);
    if (EconomyProductionOrderExpander != nullptr && BuildPlacementService != nullptr)
    {
        EconomyProductionOrderExpander->ExpandEconomyAndProductionOrders(
            Frame, AgentState, GameStateDescriptor, IntentBuffer, *BuildPlacementService, ExpansionLocations);
    }

    IntentSchedulingService.DrainReadyIntents(GameStateDescriptor.CommandAuthoritySchedulingState, IntentBuffer,
                                              GameStateDescriptor.CommandAuthoritySchedulingState.MaxUnitIntentsPerStep);
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

void TerranAgent::ProduceArmyIntents(const FFrameContext& Frame)
{
    (void)Frame;

    ProduceProductionRallyIntents();
    if (ShouldLaunchMarineAttack())
    {
        AllMarinesAttack();
        return;
    }

    AssembleCombatUnitsAtRallyPoint();
}

void TerranAgent::AssembleCombatUnitsAtRallyPoint()
{
    if (GameStateDescriptor.ArmyState.ArmyPostures.empty())
    {
        return;
    }

    const EArmyPosture PrimaryArmyPostureValue = GameStateDescriptor.ArmyState.ArmyPostures.front();
    if (!DoesArmyPostureUseAssemblyPoint(PrimaryArmyPostureValue))
    {
        return;
    }

    constexpr float AssemblyRadiusSquaredValue = 36.0f;

    for (const Unit* ControlledUnitValue : AgentState.UnitContainer.ControlledUnits)
    {
        if (ControlledUnitValue == nullptr || ControlledUnitValue->build_progress < 1.0f)
        {
            continue;
        }

        if (!IsAssemblyCombatUnitType(ControlledUnitValue->unit_type.ToType()) ||
            IntentBuffer.HasIntentForActor(ControlledUnitValue->tag))
        {
            continue;
        }

        if (DistanceSquared2D(Point2D(ControlledUnitValue->pos), BarracksRally) <= AssemblyRadiusSquaredValue)
        {
            continue;
        }

        if (!ShouldRefreshArmyOrder(*ControlledUnitValue, ABILITY_ID::MOVE_MOVE, BarracksRally))
        {
            continue;
        }

        IntentBuffer.Add(FUnitIntent::CreatePointTarget(ControlledUnitValue->tag, ABILITY_ID::MOVE_MOVE,
                                                        BarracksRally, 35, EIntentDomain::ArmyCombat, true));
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

void TerranAgent::AllMarinesAttack()
{
    if (!ObservationPtr)
    {
        return;
    }

    const Units Marines = ObservationPtr->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_MARINE));
    const bool HasEnemyContact = AgentState.SpatialMetrics.Map.HasEnemy || AgentState.SpatialMetrics.Minimap.HasEnemy;
    const Point2D EnemyTarget = GetEnemyTargetLocation();

    for (const Unit* Marine : Marines)
    {
        if (!Marine || Marine->build_progress < 1.0f)
        {
            continue;
        }

        const bool UseDirectAttack = HasEnemyContact || (CurrentStep % 3 != 1);
        const Point2D Target = UseDirectAttack ? GetRandomPointNear(EnemyTarget, 15.0f, 15.0f)
                                               : GetRandomPointNear(EnemyTarget, 20.0f, 5.0f);
        const AbilityID Ability = UseDirectAttack ? ABILITY_ID::ATTACK_ATTACK : ABILITY_ID::MOVE_MOVE;
        if (!ShouldRefreshArmyOrder(*Marine, Ability, Target))
        {
            continue;
        }

        IntentBuffer.Add(FUnitIntent::CreatePointTarget(Marine->tag, Ability, Target, 50,
                                                        EIntentDomain::ArmyCombat, true));
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

bool TerranAgent::ShouldRefreshArmyOrder(const Unit& UnitValue, const ABILITY_ID AbilityValue,
                                         const Point2D& TargetPointValue) const
{
    if (UnitValue.orders.empty())
    {
        return true;
    }

    const UnitOrder& CurrentOrderValue = UnitValue.orders.front();
    if (CurrentOrderValue.ability_id != AbilityValue)
    {
        return true;
    }

    constexpr float OrderRefreshDistanceSquared = 64.0f;
    return DistanceSquared2D(CurrentOrderValue.target_pos, TargetPointValue) > OrderRefreshDistanceSquared;
}

bool TerranAgent::ShouldLaunchMarineAttack() const
{
    if (GameStateDescriptor.ArmyState.ArmyPostures.empty())
    {
        return false;
    }

    const EArmyPosture PrimaryArmyPostureValue = GameStateDescriptor.ArmyState.ArmyPostures.front();
    if (PrimaryArmyPostureValue != EArmyPosture::Advance && PrimaryArmyPostureValue != EArmyPosture::Engage)
    {
        return false;
    }

    const bool HasEnemyContact = AgentState.SpatialMetrics.Map.HasEnemy || AgentState.SpatialMetrics.Minimap.HasEnemy;
    const uint64_t Cadence = HasEnemyContact ? 6 : 12;
    return Cadence > 0 && (CurrentStep % Cadence) == 0;
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

Point2D TerranAgent::GetRandomPointNear(const Point2D& Origin, float XRadius, float YRadius) const
{
    const float SignedXOffset = ((GetRandomScalar() * 2.0f) - 1.0f) * XRadius;
    const float SignedYOffset = ((GetRandomScalar() * 2.0f) - 1.0f) * YRadius;
    return Point2D(Origin.x + SignedXOffset, Origin.y + SignedYOffset);
}

}  // namespace sc2
