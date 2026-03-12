#include "terran.h"

namespace sc2
{

void TerranAgent::OnGameStart()
{
    CurrentStep = 0;
    ObservationPtr = Observation();
    if (!ObservationPtr)
    {
        SCLOG(LoggingVerbosity::error, "ERROR in TerranAgent::OnGameStart() - Observation() is null");
        return;
    }

    GameStateDescriptor.Reset();
    GameStateDescriptor.CurrentStep = CurrentStep;

    const Point2D BaseLocationValue = Point2D(ObservationPtr->GetStartLocation());
    const Point2D PrimaryStructureAnchorValue =
        BuildPlacementService ? BuildPlacementService->GetPrimaryStructureAnchor(GameStateDescriptor, BaseLocationValue)
                              : BaseLocationValue;
    BarracksRally = Point2D(PrimaryStructureAnchorValue.x + 6.0f, PrimaryStructureAnchorValue.y + 6.0f);

    const FFrameContext Frame = FFrameContext::Create(ObservationPtr, Query(), CurrentStep);
    UpdateAgentState(Frame);
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
    GameStateDescriptor.CurrentStep = CurrentStep;

    UpdateAgentState(Frame);

    IntentBuffer.Reset();
    ProduceRecoveryIntents(Frame);
    ProduceStructureBuildIntents(Frame);
    ProduceUnitProductionIntents(Frame);
    ProduceArmyIntents(Frame);

    ResolvedIntents = IntentArbiter.Resolve(Frame, AgentState.UnitContainer, IntentBuffer);
    ExecuteResolvedIntents(Frame, ResolvedIntents);

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

void TerranAgent::PrintAgentState()
{
    AgentState.PrintStatus();
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
                                                        GetEnemyTargetLocation(), 300,
                                                        EIntentDomain::Recovery, true));
    }
}

void TerranAgent::ProduceStructureBuildIntents(const FFrameContext& Frame)
{
    (void)Frame;
    TryBuildSupplyDepot();
    TryBuildBarracks();
}

void TerranAgent::ProduceUnitProductionIntents(const FFrameContext& Frame)
{
    (void)Frame;
    TryBuildSCV();
    TryBuildMarine();
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

const Unit* TerranAgent::SelectBuildWorker(const UNIT_TYPEID WorkerTypeId, const Point2D& BuildAnchorValue)
{
    const Unit* IdleWorker = nullptr;
    const Unit* GatheringWorker = nullptr;
    const Unit* FallbackWorker = nullptr;
    float GatheringDistance = std::numeric_limits<float>::max();
    float FallbackDistance = std::numeric_limits<float>::max();

    for (const Unit* Worker : AgentState.UnitContainer.GetWorkers())
    {
        if (!Worker || Worker->unit_type.ToType() != WorkerTypeId)
        {
            continue;
        }
        if (IntentBuffer.HasIntentForActorInDomain(Worker->tag, EIntentDomain::StructureBuild))
        {
            continue;
        }

        if (Worker->orders.empty())
        {
            IdleWorker = Worker;
            break;
        }

        const float DistanceValue = DistanceSquared2D(Worker->pos, BuildAnchorValue);
        if (Worker->orders.front().ability_id == ABILITY_ID::HARVEST_GATHER)
        {
            if (!GatheringWorker || DistanceValue < GatheringDistance)
            {
                GatheringWorker = Worker;
                GatheringDistance = DistanceValue;
            }
            continue;
        }

        if (!FallbackWorker || DistanceValue < FallbackDistance)
        {
            FallbackWorker = Worker;
            FallbackDistance = DistanceValue;
        }
    }

    return IdleWorker ? IdleWorker : (GatheringWorker ? GatheringWorker : FallbackWorker);
}

bool TerranAgent::TryGetStructureBuildPoint(const ABILITY_ID StructureAbilityId, const Unit& WorkerUnitValue,
                                            Point2D& OutBuildPointValue)
{
    if (!ObservationPtr || !BuildPlacementService)
    {
        return false;
    }

    QueryInterface* QueryValue = Query();
    if (!QueryValue)
    {
        return false;
    }

    const Point2D BaseLocationValue = Point2D(ObservationPtr->GetStartLocation());
    const std::vector<Point2D> CandidateValues =
        BuildPlacementService->GetStructurePlacementCandidates(GameStateDescriptor, StructureAbilityId, BaseLocationValue);
    const GameInfo& GameInfoValue = ObservationPtr->GetGameInfo();

    for (const Point2D& CandidateValue : CandidateValues)
    {
        const Point2D ClampedCandidateValue = ClampToPlayable(GameInfoValue, CandidateValue);
        if (QueryValue->Placement(StructureAbilityId, ClampedCandidateValue, &WorkerUnitValue))
        {
            OutBuildPointValue = ClampedCandidateValue;
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

bool TerranAgent::TryBuildStructure(const ABILITY_ID StructureAbilityId, const UNIT_TYPEID WorkerTypeId, const int Priority)
{
    if (!ObservationPtr || !BuildPlacementService)
    {
        return false;
    }

    const Point2D BaseLocationValue = Point2D(ObservationPtr->GetStartLocation());
    const Point2D BuildAnchorValue = BuildPlacementService->GetPrimaryStructureAnchor(GameStateDescriptor, BaseLocationValue);
    const Unit* WorkerToBuild = SelectBuildWorker(WorkerTypeId, BuildAnchorValue);
    if (!WorkerToBuild)
    {
        return false;
    }

    Point2D BuildPointValue;
    if (!TryGetStructureBuildPoint(StructureAbilityId, *WorkerToBuild, BuildPointValue))
    {
        return false;
    }

    IntentBuffer.Add(FUnitIntent::CreatePointTarget(WorkerToBuild->tag, StructureAbilityId, BuildPointValue, Priority,
                                                    EIntentDomain::StructureBuild, false, true));
    return true;
}

bool TerranAgent::TryBuildSupplyDepot()
{
    if (AgentState.Economy.SupplyCap >= 200)
    {
        return false;
    }

    if (AgentState.Economy.Supply <= (AgentState.Economy.SupplyCap - (AgentState.Economy.SupplyCap / 6)))
    {
        return false;
    }

    if (AgentState.Buildings.GetBarracksCount() < 1 && AgentState.Buildings.GetSupplyDepotCount() > 0)
    {
        return false;
    }

    const int MaxSupplyDepotsInProgress = 2;
    if (AgentState.Buildings.GetCurrentlyInConstruction(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) >=
        MaxSupplyDepotsInProgress)
    {
        return false;
    }

    return TryBuildStructure(ABILITY_ID::BUILD_SUPPLYDEPOT, UNIT_TYPEID::TERRAN_SCV, 220);
}

bool TerranAgent::TryBuildBarracks()
{
    if (AgentState.Buildings.GetSupplyDepotCount() < 1 || AgentState.Buildings.GetTownHallCount() < 1 ||
        AgentState.Units.GetWorkerCount() < 8)
    {
        return false;
    }

    if (AgentState.Buildings.GetSupplyDepotCount() < 2 && AgentState.Buildings.GetBarracksCount() > 0)
    {
        return false;
    }

    if (AgentState.Buildings.GetBarracksCount() > 4 && AgentState.Economy.Minerals < 600)
    {
        return false;
    }

    if (AgentState.Economy.SupplyAvailable < 2)
    {
        return false;
    }

    const int MaxBarracksInProgress = 2;
    if (AgentState.Buildings.GetCurrentlyInConstruction(UNIT_TYPEID::TERRAN_BARRACKS) >= MaxBarracksInProgress)
    {
        return false;
    }

    return TryBuildStructure(ABILITY_ID::BUILD_BARRACKS, UNIT_TYPEID::TERRAN_SCV, 180);
}

bool TerranAgent::ShouldBuildSCV() const
{
    if (AgentState.Economy.Minerals < 50)
    {
        return false;
    }

    if (AgentState.Economy.SupplyAvailable < 1)
    {
        return false;
    }

    const uint16_t TownHallCount = AgentState.Buildings.GetTownHallCount();
    if (TownHallCount < 1)
    {
        return false;
    }

    const uint16_t TargetWorkerCount = static_cast<uint16_t>(TownHallCount * 26);
    return AgentState.Units.GetWorkerCount() + AgentState.Units.GetUnitsInConstruction(UNIT_TYPEID::TERRAN_SCV) <
           TargetWorkerCount;
}

bool TerranAgent::TryBuildSCV()
{
    if (!ShouldBuildSCV())
    {
        return false;
    }

    const Unit* TownHall = AgentState.UnitContainer.GetFirstIdleTownHall();
    if (!TownHall || IntentBuffer.HasIntentForActorInDomain(TownHall->tag, EIntentDomain::UnitProduction))
    {
        return false;
    }

    IntentBuffer.Add(FUnitIntent::CreateNoTarget(TownHall->tag, ABILITY_ID::TRAIN_SCV, 140,
                                                 EIntentDomain::UnitProduction));
    return true;
}

bool TerranAgent::ShouldBuildMarine() const
{
    if (AgentState.Economy.Minerals < 50)
    {
        return false;
    }

    if (AgentState.Economy.SupplyAvailable < 1)
    {
        return false;
    }

    if (AgentState.Buildings.GetBarracksCount() < 1)
    {
        return false;
    }

    return true;
}

bool TerranAgent::TryBuildMarine()
{
    if (!ShouldBuildMarine())
    {
        return false;
    }

    const Unit* Barracks = AgentState.UnitContainer.GetFirstIdleBarracks();
    if (!Barracks || IntentBuffer.HasIntentForActorInDomain(Barracks->tag, EIntentDomain::UnitProduction))
    {
        return false;
    }

    IntentBuffer.Add(FUnitIntent::CreateNoTarget(Barracks->tag, ABILITY_ID::TRAIN_MARINE, 120,
                                                 EIntentDomain::UnitProduction));
    return true;
}

bool TerranAgent::ShouldLaunchMarineAttack() const
{
    if (AgentState.Units.GetUnitCount(UNIT_TYPEID::TERRAN_MARINE) < 24)
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
