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

    IntentBuffer.Reset();
    ProduceSchedulerOpeningIntents(Frame);
    ProduceRecoveryIntents(Frame);
    ProduceArmyIntents(Frame);
    UpdateExecutionTelemetry(Frame);

    ResolvedIntents = IntentArbiter.Resolve(Frame, AgentState.UnitContainer, IntentBuffer);
    ExecuteResolvedIntents(Frame, ResolvedIntents);
    CompleteDispatchedSchedulerOrders();

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
    (void)UnitPtr;
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

    const Point2D BaseLocationValue = Point2D(ObservationPtr->GetStartLocation());
    const Point2D PrimaryStructureAnchorValue =
        BuildPlacementService ? BuildPlacementService->GetPrimaryStructureAnchor(GameStateDescriptor, BaseLocationValue)
                              : BaseLocationValue;
    BarracksRally = Point2D(PrimaryStructureAnchorValue.x + 6.0f, PrimaryStructureAnchorValue.y + 6.0f);
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
              << " | IdleProduction " << ExecutionTelemetry.TotalIdleProductionConflictCount << "\n";
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
            if (ExecutionEventRecordValue.AbilityId != ABILITY_ID::INVALID)
            {
                std::cout << " Ability " << static_cast<uint32_t>(ExecutionEventRecordValue.AbilityId);
            }
            if (ExecutionEventRecordValue.UnitTypeId != UNIT_TYPEID::INVALID)
            {
                std::cout << " Unit " << static_cast<uint32_t>(ExecutionEventRecordValue.UnitTypeId);
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

void TerranAgent::ProduceArmyIntents(const FFrameContext& Frame)
{
    (void)Frame;
    if (ShouldLaunchMarineAttack())
    {
        AllMarinesAttack();
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

void TerranAgent::CompleteDispatchedSchedulerOrders()
{
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

        CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(
            CommandAuthoritySchedulingStateValue.OrderIds[OrderIndexValue], EOrderLifecycleState::Completed);
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
