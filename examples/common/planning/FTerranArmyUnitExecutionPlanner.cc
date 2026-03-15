#include "common/planning/FTerranArmyUnitExecutionPlanner.h"

#include <array>
#include <limits>
#include <unordered_set>

#include "common/armies/EArmyMissionType.h"
#include "common/armies/FArmyMissionDescriptor.h"
#include "common/bot_status_models.h"
#include "common/planning/FTacticalBehaviorScore.h"

namespace sc2
{
namespace
{

constexpr float LocalThreatDistanceSquaredValue = 100.0f;
constexpr float RetreatThreatDistanceSquaredValue = 64.0f;
constexpr float ObjectiveReachedDistanceSquaredValue = 25.0f;
constexpr float OrderRefreshDistanceSquaredValue = 36.0f;

uint64_t BuildMissionRevisionValue(const FArmyMissionDescriptor& MissionDescriptorValue)
{
    uint64_t MissionRevisionValue = static_cast<uint64_t>(MissionDescriptorValue.MissionType);
    MissionRevisionValue = (MissionRevisionValue * 1315423911ULL) ^ MissionDescriptorValue.SourceGoalId;
    MissionRevisionValue =
        (MissionRevisionValue * 1315423911ULL) ^ static_cast<uint64_t>(MissionDescriptorValue.ObjectivePoint.x * 100.0f);
    MissionRevisionValue =
        (MissionRevisionValue * 1315423911ULL) ^ static_cast<uint64_t>(MissionDescriptorValue.ObjectivePoint.y * 100.0f);
    MissionRevisionValue =
        (MissionRevisionValue * 1315423911ULL) ^ static_cast<uint64_t>(MissionDescriptorValue.SearchExpansionOrdinal);
    return MissionRevisionValue;
}

bool IsWorkerUnitType(const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_SCV:
        case UNIT_TYPEID::PROTOSS_PROBE:
        case UNIT_TYPEID::ZERG_DRONE:
        case UNIT_TYPEID::ZERG_DRONEBURROWED:
            return true;
        default:
            return false;
    }
}

bool IsTerranCombatUnitType(const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_MARINE:
        case UNIT_TYPEID::TERRAN_MARAUDER:
        case UNIT_TYPEID::TERRAN_HELLION:
        case UNIT_TYPEID::TERRAN_HELLIONTANK:
        case UNIT_TYPEID::TERRAN_CYCLONE:
        case UNIT_TYPEID::TERRAN_SIEGETANK:
        case UNIT_TYPEID::TERRAN_SIEGETANKSIEGED:
        case UNIT_TYPEID::TERRAN_WIDOWMINE:
        case UNIT_TYPEID::TERRAN_WIDOWMINEBURROWED:
        case UNIT_TYPEID::TERRAN_MEDIVAC:
        case UNIT_TYPEID::TERRAN_LIBERATOR:
        case UNIT_TYPEID::TERRAN_LIBERATORAG:
        case UNIT_TYPEID::TERRAN_VIKINGFIGHTER:
        case UNIT_TYPEID::TERRAN_VIKINGASSAULT:
            return true;
        default:
            return false;
    }
}

bool IsSupportUnitType(const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_MEDIVAC:
            return true;
        default:
            return false;
    }
}

bool IsEnemyCombatThreat(const Unit& EnemyUnitValue)
{
    return EnemyUnitValue.build_progress >= 1.0f && !EnemyUnitValue.is_building &&
           !IsWorkerUnitType(EnemyUnitValue.unit_type.ToType());
}

const FArmyMissionDescriptor* GetMissionDescriptorForArmyIndex(const FGameStateDescriptor& GameStateDescriptorValue,
                                                               const int32_t OwningArmyIndexValue)
{
    if (OwningArmyIndexValue < 0)
    {
        return nullptr;
    }

    const size_t ArmyIndexValue = static_cast<size_t>(OwningArmyIndexValue);
    if (ArmyIndexValue >= GameStateDescriptorValue.ArmyState.ArmyMissions.size())
    {
        return nullptr;
    }

    return &GameStateDescriptorValue.ArmyState.ArmyMissions[ArmyIndexValue];
}

const Unit* FindNearestEnemyThreat(const Point2D& OriginPointValue, const Units& EnemyUnitsValue)
{
    const Unit* BestEnemyUnitValue = nullptr;
    float BestDistanceSquaredValue = std::numeric_limits<float>::max();

    for (const Unit* EnemyUnitValue : EnemyUnitsValue)
    {
        if (EnemyUnitValue == nullptr || !IsEnemyCombatThreat(*EnemyUnitValue))
        {
            continue;
        }

        const float DistanceSquaredValue = DistanceSquared2D(OriginPointValue, Point2D(EnemyUnitValue->pos));
        if (DistanceSquaredValue >= BestDistanceSquaredValue)
        {
            continue;
        }

        BestDistanceSquaredValue = DistanceSquaredValue;
        BestEnemyUnitValue = EnemyUnitValue;
    }

    return BestEnemyUnitValue;
}

const Unit* FindNearestEnemyStructure(const Point2D& OriginPointValue, const Units& EnemyUnitsValue)
{
    const Unit* BestEnemyUnitValue = nullptr;
    float BestDistanceSquaredValue = std::numeric_limits<float>::max();

    for (const Unit* EnemyUnitValue : EnemyUnitsValue)
    {
        if (EnemyUnitValue == nullptr || EnemyUnitValue->build_progress < 1.0f || !EnemyUnitValue->is_building)
        {
            continue;
        }

        const float DistanceSquaredValue = DistanceSquared2D(OriginPointValue, Point2D(EnemyUnitValue->pos));
        if (DistanceSquaredValue >= BestDistanceSquaredValue)
        {
            continue;
        }

        BestDistanceSquaredValue = DistanceSquaredValue;
        BestEnemyUnitValue = EnemyUnitValue;
    }

    return BestEnemyUnitValue;
}

bool ShouldRefreshUnitOrder(const Unit& ActorUnitValue, const ABILITY_ID AbilityIdValue, const Point2D& TargetPointValue)
{
    if (ActorUnitValue.orders.empty())
    {
        return true;
    }

    const UnitOrder& CurrentOrderValue = ActorUnitValue.orders.front();
    if (CurrentOrderValue.ability_id != AbilityIdValue)
    {
        return true;
    }

    return DistanceSquared2D(CurrentOrderValue.target_pos, TargetPointValue) > OrderRefreshDistanceSquaredValue;
}

bool ShouldRefreshUnitOrder(const Unit& ActorUnitValue, const ABILITY_ID AbilityIdValue, const Tag TargetUnitTagValue)
{
    if (ActorUnitValue.orders.empty())
    {
        return true;
    }

    const UnitOrder& CurrentOrderValue = ActorUnitValue.orders.front();
    return CurrentOrderValue.ability_id != AbilityIdValue || CurrentOrderValue.target_unit_tag != TargetUnitTagValue;
}

bool ShouldRefreshHoldPosition(const Unit& ActorUnitValue)
{
    if (ActorUnitValue.orders.empty())
    {
        return true;
    }

    return ActorUnitValue.orders.front().ability_id != ABILITY_ID::GENERAL_HOLDPOSITION;
}

bool IsActiveUnitExecutionOrder(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                const size_t OrderIndexValue)
{
    return CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] == ECommandAuthorityLayer::UnitExecution &&
           !IsTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]);
}

bool DoesExecutionOrderMatchDesiredOrder(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                         const size_t OrderIndexValue,
                                         const FCommandOrderRecord& DesiredExecutionOrderValue)
{
    if (!IsActiveUnitExecutionOrder(CommandAuthoritySchedulingStateValue, OrderIndexValue) ||
        CommandAuthoritySchedulingStateValue.ActorTags[OrderIndexValue] != DesiredExecutionOrderValue.ActorTag ||
        CommandAuthoritySchedulingStateValue.AbilityIds[OrderIndexValue] != DesiredExecutionOrderValue.AbilityId ||
        CommandAuthoritySchedulingStateValue.TargetKinds[OrderIndexValue] != DesiredExecutionOrderValue.TargetKind)
    {
        return false;
    }

    switch (DesiredExecutionOrderValue.TargetKind)
    {
        case EIntentTargetKind::None:
            return true;
        case EIntentTargetKind::Point:
            return DistanceSquared2D(CommandAuthoritySchedulingStateValue.TargetPoints[OrderIndexValue],
                                     DesiredExecutionOrderValue.TargetPoint) <= OrderRefreshDistanceSquaredValue;
        case EIntentTargetKind::Unit:
            return CommandAuthoritySchedulingStateValue.TargetUnitTags[OrderIndexValue] ==
                   DesiredExecutionOrderValue.TargetUnitTag;
        default:
            return false;
    }
}

bool HasMatchingActiveExecutionOrderForActor(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                             const FCommandOrderRecord& DesiredExecutionOrderValue)
{
    size_t ExistingExecutionOrderIndexValue = 0U;
    return CommandAuthoritySchedulingStateValue.TryGetActiveExecutionOrderIndexForActor(
               DesiredExecutionOrderValue.ActorTag, ExistingExecutionOrderIndexValue) &&
           DoesExecutionOrderMatchDesiredOrder(CommandAuthoritySchedulingStateValue, ExistingExecutionOrderIndexValue,
                                               DesiredExecutionOrderValue);
}

uint32_t ExpireMismatchedExecutionOrdersForActor(FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                                 const FCommandOrderRecord& DesiredExecutionOrderValue)
{
    size_t ExistingExecutionOrderIndexValue = 0U;
    if (!CommandAuthoritySchedulingStateValue.TryGetActiveExecutionOrderIndexForActor(
            DesiredExecutionOrderValue.ActorTag, ExistingExecutionOrderIndexValue) ||
        DoesExecutionOrderMatchDesiredOrder(CommandAuthoritySchedulingStateValue, ExistingExecutionOrderIndexValue,
                                            DesiredExecutionOrderValue))
    {
        return 0U;
    }

    CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(
        CommandAuthoritySchedulingStateValue.OrderIds[ExistingExecutionOrderIndexValue], EOrderLifecycleState::Expired);
    return 1U;
}

uint32_t CountActiveUnitExecutionOrders(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue)
{
    return static_cast<uint32_t>(
        CommandAuthoritySchedulingStateValue.GetActiveOrderCountForLayer(ECommandAuthorityLayer::UnitExecution));
}

bool IsBehaviorTargetEquivalent(const FTacticalBehaviorScore& TacticalBehaviorScoreValue,
                                const FUnitExecutionCacheEntry& UnitExecutionCacheEntryValue)
{
    switch (UnitExecutionCacheEntryValue.LastTargetKind)
    {
        case EIntentTargetKind::Point:
            return DistanceSquared2D(TacticalBehaviorScoreValue.TargetPoint,
                                     UnitExecutionCacheEntryValue.LastTargetPoint) <=
                   OrderRefreshDistanceSquaredValue;
        case EIntentTargetKind::Unit:
            return TacticalBehaviorScoreValue.TargetUnitTag == UnitExecutionCacheEntryValue.LastTargetUnitTag;
        case EIntentTargetKind::None:
        default:
            return TacticalBehaviorScoreValue.TargetUnitTag == NullTag;
    }
}

bool DoesCacheEntryMatchDesiredBehavior(const FUnitExecutionCacheEntry& UnitExecutionCacheEntryValue,
                                        const uint64_t MissionRevisionValue,
                                        const FTacticalBehaviorScore& TacticalBehaviorScoreValue,
                                        const bool bInsideAssemblyRadiusValue)
{
    return UnitExecutionCacheEntryValue.LastMissionRevision == MissionRevisionValue &&
           UnitExecutionCacheEntryValue.LastTacticalBehavior == TacticalBehaviorScoreValue.Behavior &&
           UnitExecutionCacheEntryValue.bInsideAssemblyRadius == bInsideAssemblyRadiusValue &&
           IsBehaviorTargetEquivalent(TacticalBehaviorScoreValue, UnitExecutionCacheEntryValue);
}

void UpdateUnitExecutionCacheEntry(FUnitExecutionCacheEntry& UnitExecutionCacheEntryValue,
                                   const uint64_t MissionRevisionValue,
                                   const FTacticalBehaviorScore& TacticalBehaviorScoreValue,
                                   const EIntentTargetKind IntentTargetKindValue,
                                   const uint64_t CurrentGameLoopValue,
                                   const bool bInsideAssemblyRadiusValue)
{
    UnitExecutionCacheEntryValue.LastMissionRevision = MissionRevisionValue;
    UnitExecutionCacheEntryValue.LastTacticalBehavior = TacticalBehaviorScoreValue.Behavior;
    UnitExecutionCacheEntryValue.LastTargetKind = IntentTargetKindValue;
    UnitExecutionCacheEntryValue.LastTargetPoint = TacticalBehaviorScoreValue.TargetPoint;
    UnitExecutionCacheEntryValue.LastTargetUnitTag = TacticalBehaviorScoreValue.TargetUnitTag;
    UnitExecutionCacheEntryValue.LastIssuedGameLoop = CurrentGameLoopValue;
    UnitExecutionCacheEntryValue.bInsideAssemblyRadius = bInsideAssemblyRadiusValue;
}

FTacticalBehaviorScore BuildAdvanceScore(const Unit& ControlledUnitValue, const FArmyMissionDescriptor& MissionDescriptorValue,
                                         const Point2D& RallyPointValue)
{
    FTacticalBehaviorScore TacticalBehaviorScoreValue;
    TacticalBehaviorScoreValue.Behavior = EUnitTacticalBehavior::AdvanceToMissionAnchor;
    TacticalBehaviorScoreValue.ScoreValue = 300;
    TacticalBehaviorScoreValue.TargetPoint = MissionDescriptorValue.ObjectivePoint;

    if (MissionDescriptorValue.MissionType == EArmyMissionType::AssembleAtRally ||
        MissionDescriptorValue.MissionType == EArmyMissionType::Regroup)
    {
        TacticalBehaviorScoreValue.TargetPoint = RallyPointValue;
        TacticalBehaviorScoreValue.ScoreValue = 500;
    }

    const float ObjectiveDistanceSquaredValue = DistanceSquared2D(Point2D(ControlledUnitValue.pos),
                                                                  TacticalBehaviorScoreValue.TargetPoint);
    if (ObjectiveDistanceSquaredValue > ObjectiveReachedDistanceSquaredValue)
    {
        TacticalBehaviorScoreValue.ScoreValue += 250;
    }

    return TacticalBehaviorScoreValue;
}

FTacticalBehaviorScore BuildAttackThreatScore(const Unit& ControlledUnitValue, const Unit* EnemyThreatUnitPtrValue,
                                              const FArmyMissionDescriptor& MissionDescriptorValue)
{
    FTacticalBehaviorScore TacticalBehaviorScoreValue;
    TacticalBehaviorScoreValue.Behavior = EUnitTacticalBehavior::AttackLocalThreat;
    TacticalBehaviorScoreValue.ScoreValue = std::numeric_limits<int>::min();

    if (EnemyThreatUnitPtrValue == nullptr)
    {
        return TacticalBehaviorScoreValue;
    }

    TacticalBehaviorScoreValue.TargetUnitTag = EnemyThreatUnitPtrValue->tag;
    TacticalBehaviorScoreValue.TargetPoint = Point2D(EnemyThreatUnitPtrValue->pos);
    TacticalBehaviorScoreValue.ScoreValue =
        DistanceSquared2D(Point2D(ControlledUnitValue.pos), TacticalBehaviorScoreValue.TargetPoint) <=
                LocalThreatDistanceSquaredValue
            ? 900
            : 450;

    if (MissionDescriptorValue.MissionType == EArmyMissionType::DefendOwnedBase)
    {
        TacticalBehaviorScoreValue.ScoreValue += 150;
    }

    return TacticalBehaviorScoreValue;
}

FTacticalBehaviorScore BuildClearStructureScore(const Unit* EnemyStructureUnitPtrValue,
                                                const FArmyMissionDescriptor& MissionDescriptorValue)
{
    FTacticalBehaviorScore TacticalBehaviorScoreValue;
    TacticalBehaviorScoreValue.Behavior = EUnitTacticalBehavior::ClearBlockingStructure;
    TacticalBehaviorScoreValue.ScoreValue = std::numeric_limits<int>::min();

    if (EnemyStructureUnitPtrValue == nullptr)
    {
        return TacticalBehaviorScoreValue;
    }

    switch (MissionDescriptorValue.MissionType)
    {
        case EArmyMissionType::PressureKnownEnemyBase:
        case EArmyMissionType::ClearKnownEnemyStructures:
        case EArmyMissionType::SweepExpansionLocations:
            TacticalBehaviorScoreValue.ScoreValue = 650;
            TacticalBehaviorScoreValue.TargetUnitTag = EnemyStructureUnitPtrValue->tag;
            TacticalBehaviorScoreValue.TargetPoint = Point2D(EnemyStructureUnitPtrValue->pos);
            return TacticalBehaviorScoreValue;
        default:
            return TacticalBehaviorScoreValue;
    }
}

FTacticalBehaviorScore BuildRegroupScore(const Unit& ControlledUnitValue, const Point2D& RallyPointValue,
                                         const FArmyMissionDescriptor& MissionDescriptorValue)
{
    FTacticalBehaviorScore TacticalBehaviorScoreValue;
    TacticalBehaviorScoreValue.Behavior = EUnitTacticalBehavior::RegroupToSquadAnchor;
    TacticalBehaviorScoreValue.ScoreValue = std::numeric_limits<int>::min();
    TacticalBehaviorScoreValue.TargetPoint = RallyPointValue;

    if (MissionDescriptorValue.MissionType != EArmyMissionType::Regroup &&
        MissionDescriptorValue.MissionType != EArmyMissionType::AssembleAtRally)
    {
        return TacticalBehaviorScoreValue;
    }

    TacticalBehaviorScoreValue.ScoreValue = 950;
    if (DistanceSquared2D(Point2D(ControlledUnitValue.pos), RallyPointValue) <= ObjectiveReachedDistanceSquaredValue)
    {
        TacticalBehaviorScoreValue.ScoreValue = 300;
    }

    return TacticalBehaviorScoreValue;
}

FTacticalBehaviorScore BuildRetreatScore(const Unit& ControlledUnitValue, const Unit* EnemyThreatUnitPtrValue,
                                         const Point2D& RallyPointValue)
{
    FTacticalBehaviorScore TacticalBehaviorScoreValue;
    TacticalBehaviorScoreValue.Behavior = EUnitTacticalBehavior::RetreatToSafeAnchor;
    TacticalBehaviorScoreValue.ScoreValue = std::numeric_limits<int>::min();
    TacticalBehaviorScoreValue.TargetPoint = RallyPointValue;

    if (EnemyThreatUnitPtrValue == nullptr)
    {
        return TacticalBehaviorScoreValue;
    }

    const float HealthFractionValue =
        ControlledUnitValue.health_max > 0.0f ? (ControlledUnitValue.health / ControlledUnitValue.health_max) : 1.0f;
    const float DistanceSquaredValue =
        DistanceSquared2D(Point2D(ControlledUnitValue.pos), Point2D(EnemyThreatUnitPtrValue->pos));
    if (HealthFractionValue >= 0.35f || DistanceSquaredValue > RetreatThreatDistanceSquaredValue)
    {
        return TacticalBehaviorScoreValue;
    }

    TacticalBehaviorScoreValue.ScoreValue = 1000;
    return TacticalBehaviorScoreValue;
}

FTacticalBehaviorScore BuildHoldPositionScore(const Unit& ControlledUnitValue,
                                              const FArmyMissionDescriptor& MissionDescriptorValue)
{
    FTacticalBehaviorScore TacticalBehaviorScoreValue;
    TacticalBehaviorScoreValue.Behavior = EUnitTacticalBehavior::HoldPosition;
    TacticalBehaviorScoreValue.ScoreValue = std::numeric_limits<int>::min();
    TacticalBehaviorScoreValue.TargetPoint = MissionDescriptorValue.ObjectivePoint;

    switch (MissionDescriptorValue.MissionType)
    {
        case EArmyMissionType::DefendOwnedBase:
        case EArmyMissionType::AssembleAtRally:
        case EArmyMissionType::Regroup:
            if (DistanceSquared2D(Point2D(ControlledUnitValue.pos), MissionDescriptorValue.ObjectivePoint) <=
                ObjectiveReachedDistanceSquaredValue)
            {
                TacticalBehaviorScoreValue.ScoreValue = 700;
            }
            return TacticalBehaviorScoreValue;
        default:
            return TacticalBehaviorScoreValue;
    }
}

FTacticalBehaviorScore SelectBestBehaviorScore(const Unit& ControlledUnitValue, const Units& EnemyUnitsValue,
                                               const FArmyMissionDescriptor& MissionDescriptorValue,
                                               const Point2D& RallyPointValue)
{
    const Unit* EnemyThreatUnitPtrValue = FindNearestEnemyThreat(Point2D(ControlledUnitValue.pos), EnemyUnitsValue);
    const Unit* EnemyStructureUnitPtrValue =
        FindNearestEnemyStructure(MissionDescriptorValue.ObjectivePoint, EnemyUnitsValue);

    FTacticalBehaviorScore BestBehaviorScoreValue = BuildAdvanceScore(ControlledUnitValue, MissionDescriptorValue,
                                                                      RallyPointValue);

    const std::array<FTacticalBehaviorScore, 5U> CandidateScoresValue = {
        BuildAttackThreatScore(ControlledUnitValue, EnemyThreatUnitPtrValue, MissionDescriptorValue),
        BuildClearStructureScore(EnemyStructureUnitPtrValue, MissionDescriptorValue),
        BuildRegroupScore(ControlledUnitValue, RallyPointValue, MissionDescriptorValue),
        BuildRetreatScore(ControlledUnitValue, EnemyThreatUnitPtrValue, RallyPointValue),
        BuildHoldPositionScore(ControlledUnitValue, MissionDescriptorValue)};

    for (const FTacticalBehaviorScore& CandidateScoreValue : CandidateScoresValue)
    {
        if (CandidateScoreValue.ScoreValue > BestBehaviorScoreValue.ScoreValue)
        {
            BestBehaviorScoreValue = CandidateScoreValue;
        }
    }

    return BestBehaviorScoreValue;
}

FCommandOrderRecord CreateBehaviorExecutionOrder(const Unit& ControlledUnitValue,
                                                 const FTacticalBehaviorScore& TacticalBehaviorScoreValue,
                                                 const FCommandOrderRecord& SquadOrderValue,
                                                 const uint64_t CurrentGameLoopValue)
{
    const bool IsSupportUnitValue = IsSupportUnitType(ControlledUnitValue.unit_type.ToType());

    switch (TacticalBehaviorScoreValue.Behavior)
    {
        case EUnitTacticalBehavior::AttackLocalThreat:
        case EUnitTacticalBehavior::ClearBlockingStructure:
            if (!IsSupportUnitValue && TacticalBehaviorScoreValue.TargetUnitTag != NullTag)
            {
                return FCommandOrderRecord::CreateUnitTarget(
                    ECommandAuthorityLayer::UnitExecution, ControlledUnitValue.tag, ABILITY_ID::ATTACK_ATTACK,
                    TacticalBehaviorScoreValue.TargetUnitTag, SquadOrderValue.BasePriorityValue,
                    EIntentDomain::ArmyCombat, CurrentGameLoopValue, 0U, SquadOrderValue.OrderId,
                    SquadOrderValue.OwningArmyIndex, SquadOrderValue.OwningSquadIndex, false);
            }
            break;
        case EUnitTacticalBehavior::HoldPosition:
            return FCommandOrderRecord::CreateNoTarget(ECommandAuthorityLayer::UnitExecution, ControlledUnitValue.tag,
                                                       ABILITY_ID::GENERAL_HOLDPOSITION,
                                                       SquadOrderValue.BasePriorityValue, EIntentDomain::ArmyCombat,
                                                       CurrentGameLoopValue, 0U, SquadOrderValue.OrderId,
                                                       SquadOrderValue.OwningArmyIndex,
                                                       SquadOrderValue.OwningSquadIndex, false);
        case EUnitTacticalBehavior::AdvanceToMissionAnchor:
        case EUnitTacticalBehavior::RegroupToSquadAnchor:
        case EUnitTacticalBehavior::RetreatToSafeAnchor:
        default:
            break;
    }

    const ABILITY_ID AbilityIdValue = IsSupportUnitValue ? ABILITY_ID::MOVE_MOVE : ABILITY_ID::ATTACK_ATTACK;
    return FCommandOrderRecord::CreatePointTarget(ECommandAuthorityLayer::UnitExecution, ControlledUnitValue.tag,
                                                  AbilityIdValue, TacticalBehaviorScoreValue.TargetPoint,
                                                  SquadOrderValue.BasePriorityValue, EIntentDomain::ArmyCombat,
                                                  CurrentGameLoopValue, 0U, SquadOrderValue.OrderId,
                                                  SquadOrderValue.OwningArmyIndex, SquadOrderValue.OwningSquadIndex,
                                                  true, false, false);
}

bool ShouldCreateExecutionOrder(const Unit& ControlledUnitValue, const FTacticalBehaviorScore& TacticalBehaviorScoreValue)
{
    switch (TacticalBehaviorScoreValue.Behavior)
    {
        case EUnitTacticalBehavior::AttackLocalThreat:
        case EUnitTacticalBehavior::ClearBlockingStructure:
            return TacticalBehaviorScoreValue.TargetUnitTag != NullTag &&
                   ShouldRefreshUnitOrder(ControlledUnitValue, ABILITY_ID::ATTACK_ATTACK,
                                          TacticalBehaviorScoreValue.TargetUnitTag);
        case EUnitTacticalBehavior::HoldPosition:
            return ShouldRefreshHoldPosition(ControlledUnitValue);
        case EUnitTacticalBehavior::AdvanceToMissionAnchor:
        case EUnitTacticalBehavior::RegroupToSquadAnchor:
        case EUnitTacticalBehavior::RetreatToSafeAnchor:
            if (IsSupportUnitType(ControlledUnitValue.unit_type.ToType()))
            {
                return ShouldRefreshUnitOrder(ControlledUnitValue, ABILITY_ID::MOVE_MOVE,
                                              TacticalBehaviorScoreValue.TargetPoint);
            }

            return ShouldRefreshUnitOrder(ControlledUnitValue, ABILITY_ID::ATTACK_ATTACK,
                                          TacticalBehaviorScoreValue.TargetPoint);
        default:
            return false;
    }
}

}  // namespace

uint32_t FTerranArmyUnitExecutionPlanner::ExpandUnitExecutionOrders(
    const FFrameContext& FrameValue, const FAgentState& AgentStateValue,
    const FGameStateDescriptor& GameStateDescriptorValue, const Point2D& RallyPointValue,
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue) const
{
    if (FrameValue.Observation == nullptr)
    {
        return 0U;
    }

    uint32_t CreatedExecutionOrderCountValue = 0U;
    uint32_t ActiveUnitExecutionOrderCountValue =
        CountActiveUnitExecutionOrders(CommandAuthoritySchedulingStateValue);
    const Units EnemyUnitsValue = FrameValue.Observation->GetUnits(Unit::Alliance::Enemy);
    const std::vector<size_t> SquadOrderIndicesValue = CommandAuthoritySchedulingStateValue.SquadOrderIndices;
    std::unordered_set<Tag> ActiveCombatUnitTagsValue;

    for (const size_t SquadOrderIndexValue : SquadOrderIndicesValue)
    {
        if (CreatedExecutionOrderCountValue >= CommandAuthoritySchedulingStateValue.MaxUnitIntentsPerStep)
        {
            break;
        }

        const FCommandOrderRecord SquadOrderValue = CommandAuthoritySchedulingStateValue.GetOrderRecord(SquadOrderIndexValue);
        if (SquadOrderValue.SourceLayer != ECommandAuthorityLayer::Squad ||
            IsTerminalLifecycleState(SquadOrderValue.LifecycleState))
        {
            continue;
        }

        const FArmyMissionDescriptor* MissionDescriptorPtrValue =
            GetMissionDescriptorForArmyIndex(GameStateDescriptorValue, SquadOrderValue.OwningArmyIndex);
        if (MissionDescriptorPtrValue == nullptr)
        {
            CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(SquadOrderValue.OrderId,
                                                                       EOrderLifecycleState::Aborted);
            continue;
        }

        const uint64_t MissionRevisionValue = BuildMissionRevisionValue(*MissionDescriptorPtrValue);
        for (const Unit* ControlledUnitPtrValue : AgentStateValue.UnitContainer.ControlledUnits)
        {
            if (CreatedExecutionOrderCountValue >= CommandAuthoritySchedulingStateValue.MaxUnitIntentsPerStep)
            {
                break;
            }

            if (ControlledUnitPtrValue == nullptr || ControlledUnitPtrValue->build_progress < 1.0f ||
                !IsTerranCombatUnitType(ControlledUnitPtrValue->unit_type.ToType()))
            {
                continue;
            }

            ActiveCombatUnitTagsValue.insert(ControlledUnitPtrValue->tag);
            const FTacticalBehaviorScore TacticalBehaviorScoreValue = SelectBestBehaviorScore(
                *ControlledUnitPtrValue, EnemyUnitsValue, *MissionDescriptorPtrValue, RallyPointValue);
            if (TacticalBehaviorScoreValue.ScoreValue == std::numeric_limits<int>::min())
            {
                continue;
            }

            FCommandOrderRecord UnitExecutionOrderValue = CreateBehaviorExecutionOrder(
                *ControlledUnitPtrValue, TacticalBehaviorScoreValue, SquadOrderValue, FrameValue.GameLoop);
            const bool bInsideAssemblyRadiusValue =
                DistanceSquared2D(Point2D(ControlledUnitPtrValue->pos), RallyPointValue) <=
                ObjectiveReachedDistanceSquaredValue;
            FUnitExecutionCacheEntry& UnitExecutionCacheEntryValue =
                UnitExecutionCacheEntries[ControlledUnitPtrValue->tag];
            const bool bShouldCreateExecutionOrderValue =
                ShouldCreateExecutionOrder(*ControlledUnitPtrValue, TacticalBehaviorScoreValue);
            const bool bHasMatchingActiveExecutionOrderValue =
                HasMatchingActiveExecutionOrderForActor(CommandAuthoritySchedulingStateValue, UnitExecutionOrderValue);
            if (DoesCacheEntryMatchDesiredBehavior(UnitExecutionCacheEntryValue, MissionRevisionValue,
                                                   TacticalBehaviorScoreValue, bInsideAssemblyRadiusValue) &&
                (bHasMatchingActiveExecutionOrderValue || !bShouldCreateExecutionOrderValue))
            {
                continue;
            }

            const uint32_t ExpiredOrderCountValue =
                ExpireMismatchedExecutionOrdersForActor(CommandAuthoritySchedulingStateValue, UnitExecutionOrderValue);
            if (ExpiredOrderCountValue > 0U)
            {
                CommandAuthoritySchedulingStateValue.SupersededUnitExecutionOrderCount += ExpiredOrderCountValue;
                ActiveUnitExecutionOrderCountValue =
                    ActiveUnitExecutionOrderCountValue > ExpiredOrderCountValue
                        ? (ActiveUnitExecutionOrderCountValue - ExpiredOrderCountValue)
                        : 0U;
            }

            if (!bShouldCreateExecutionOrderValue)
            {
                UpdateUnitExecutionCacheEntry(UnitExecutionCacheEntryValue, MissionRevisionValue,
                                              TacticalBehaviorScoreValue, UnitExecutionOrderValue.TargetKind,
                                              FrameValue.GameLoop, bInsideAssemblyRadiusValue);
                continue;
            }

            if (bHasMatchingActiveExecutionOrderValue)
            {
                UpdateUnitExecutionCacheEntry(UnitExecutionCacheEntryValue, MissionRevisionValue,
                                              TacticalBehaviorScoreValue, UnitExecutionOrderValue.TargetKind,
                                              FrameValue.GameLoop, bInsideAssemblyRadiusValue);
                continue;
            }

            if (ActiveUnitExecutionOrderCountValue >=
                CommandAuthoritySchedulingStateValue.MaxActiveUnitExecutionOrders)
            {
                ++CommandAuthoritySchedulingStateValue.RejectedUnitExecutionAdmissionCount;
                continue;
            }

            UnitExecutionOrderValue.SourceGoalId = SquadOrderValue.SourceGoalId;
            UnitExecutionOrderValue.TaskPackageKind = SquadOrderValue.TaskPackageKind;
            UnitExecutionOrderValue.TaskNeedKind = SquadOrderValue.TaskNeedKind;
            UnitExecutionOrderValue.TaskType = SquadOrderValue.TaskType;
            UnitExecutionOrderValue.Origin = SquadOrderValue.Origin;
            UnitExecutionOrderValue.EffectivePriorityValue = SquadOrderValue.EffectivePriorityValue;
            UnitExecutionOrderValue.PriorityTier = SquadOrderValue.PriorityTier;
            UnitExecutionOrderValue.LifecycleState = EOrderLifecycleState::Ready;
            CommandAuthoritySchedulingStateValue.EnqueueOrder(UnitExecutionOrderValue);
            UpdateUnitExecutionCacheEntry(UnitExecutionCacheEntryValue, MissionRevisionValue,
                                          TacticalBehaviorScoreValue, UnitExecutionOrderValue.TargetKind,
                                          FrameValue.GameLoop, bInsideAssemblyRadiusValue);
            ++ActiveUnitExecutionOrderCountValue;
            ++CreatedExecutionOrderCountValue;
        }
    }

    for (std::unordered_map<Tag, FUnitExecutionCacheEntry>::iterator CacheIteratorValue = UnitExecutionCacheEntries.begin();
         CacheIteratorValue != UnitExecutionCacheEntries.end();)
    {
        if (ActiveCombatUnitTagsValue.find(CacheIteratorValue->first) != ActiveCombatUnitTagsValue.end())
        {
            ++CacheIteratorValue;
            continue;
        }

        CacheIteratorValue = UnitExecutionCacheEntries.erase(CacheIteratorValue);
    }

    return CreatedExecutionOrderCountValue;
}

}  // namespace sc2
