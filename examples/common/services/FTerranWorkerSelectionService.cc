#include "common/services/FTerranWorkerSelectionService.h"

#include <limits>

#include "common/agent_framework.h"
#include "common/bot_status_models.h"
#include "common/planning/FCommandAuthoritySchedulingState.h"
#include "sc2api/sc2_interfaces.h"
#include "sc2api/sc2_unit_filters.h"

namespace sc2
{
namespace
{

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

bool HasActiveSchedulerOrderForActorTag(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                        const Tag ActorTagValue)
{
    const size_t OrderCountValue = CommandAuthoritySchedulingStateValue.OrderIds.size();
    for (size_t OrderIndexValue = 0U; OrderIndexValue < OrderCountValue; ++OrderIndexValue)
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

const Unit* FindNearestReadyTownHallUnit(const Units& ReadyTownHallUnitsValue, const Point2D& OriginPointValue)
{
    const Unit* BestTownHallUnitValue = nullptr;
    float BestDistanceSquaredValue = std::numeric_limits<float>::max();

    for (const Unit* TownHallUnitValue : ReadyTownHallUnitsValue)
    {
        if (TownHallUnitValue == nullptr || TownHallUnitValue->build_progress < 1.0f)
        {
            continue;
        }

        const float DistanceSquaredValue =
            DistanceSquared2D(Point2D(TownHallUnitValue->pos), OriginPointValue);
        if (BestTownHallUnitValue != nullptr && DistanceSquaredValue >= BestDistanceSquaredValue)
        {
            continue;
        }

        BestTownHallUnitValue = TownHallUnitValue;
        BestDistanceSquaredValue = DistanceSquaredValue;
    }

    return BestTownHallUnitValue;
}

const Unit* FindMineralTownHallForWorker(const ObservationInterface& ObservationValue, const Unit& WorkerUnitValue,
                                         const Units& ReadyTownHallUnitsValue)
{
    if (!IsWorkerCommittedToMinerals(ObservationValue, WorkerUnitValue))
    {
        return nullptr;
    }

    return FindNearestReadyTownHallUnit(ReadyTownHallUnitsValue, Point2D(WorkerUnitValue.pos));
}

}  // namespace

const Unit* FTerranWorkerSelectionService::SelectWorkerForRefinery(
    const ObservationInterface& ObservationValue,
    const FAgentState& AgentStateValue,
    const FCommandAuthoritySchedulingState& SchedulingStateValue,
    const FIntentBuffer& IntentBufferValue,
    const Unit& RefineryUnitValue,
    const std::unordered_set<Tag>& ReservedWorkerTagsValue) const
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
            HasActiveSchedulerOrderForActorTag(SchedulingStateValue, WorkerUnitValue->tag) ||
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

const Unit* FTerranWorkerSelectionService::SelectWorkerForGasRelief(
    const ObservationInterface& ObservationValue,
    const FAgentState& AgentStateValue,
    const FCommandAuthoritySchedulingState& SchedulingStateValue,
    const FIntentBuffer& IntentBufferValue,
    const Unit& RefineryUnitValue,
    const std::unordered_set<Tag>& ReservedWorkerTagsValue) const
{
    const Unit* BestRefineryWorkerValue = nullptr;
    float BestRefineryWorkerDistanceSquaredValue = std::numeric_limits<float>::max();

    for (const Unit* WorkerUnitValue : AgentStateValue.UnitContainer.ControlledUnits)
    {
        if (WorkerUnitValue == nullptr || WorkerUnitValue->unit_type.ToType() != UNIT_TYPEID::TERRAN_SCV ||
            WorkerUnitValue->build_progress < 1.0f ||
            ReservedWorkerTagsValue.find(WorkerUnitValue->tag) != ReservedWorkerTagsValue.end() ||
            IntentBufferValue.HasIntentForActor(WorkerUnitValue->tag) ||
            HasActiveSchedulerOrderForActorTag(SchedulingStateValue, WorkerUnitValue->tag) ||
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

const Unit* FTerranWorkerSelectionService::SelectWorkerForMineralRebalance(
    const ObservationInterface& ObservationValue,
    const FAgentState& AgentStateValue,
    const FCommandAuthoritySchedulingState& SchedulingStateValue,
    const FIntentBuffer& IntentBufferValue,
    const Unit& ReceiverTownHallUnitValue,
    const Units& ReadyTownHallUnitsValue,
    const std::unordered_set<Tag>& ReservedWorkerTagsValue,
    const std::unordered_map<Tag, int>& PlannedInboundCountsByTownHallTagValue,
    const std::unordered_map<Tag, int>& PlannedOutboundCountsByTownHallTagValue) const
{
    const Unit* BestWorkerUnitValue = nullptr;
    float BestDistanceSquaredValue = std::numeric_limits<float>::max();
    bool BestWorkerIsCarryingMineralsValue = true;
    int BestSourceHarvesterSurplusValue = 0;

    for (const Unit* WorkerUnitValue : AgentStateValue.UnitContainer.ControlledUnits)
    {
        if (WorkerUnitValue == nullptr || WorkerUnitValue->unit_type.ToType() != UNIT_TYPEID::TERRAN_SCV ||
            WorkerUnitValue->build_progress < 1.0f ||
            ReservedWorkerTagsValue.find(WorkerUnitValue->tag) != ReservedWorkerTagsValue.end() ||
            IntentBufferValue.HasIntentForActor(WorkerUnitValue->tag) ||
            HasActiveSchedulerOrderForActorTag(SchedulingStateValue, WorkerUnitValue->tag) ||
            IsWorkerCommittedToConstruction(*WorkerUnitValue) ||
            IsWorkerCommittedToRefinery(ObservationValue, *WorkerUnitValue) ||
            !IsWorkerCommittedToMinerals(ObservationValue, *WorkerUnitValue))
        {
            continue;
        }

        const Unit* SourceTownHallUnitValue =
            FindMineralTownHallForWorker(ObservationValue, *WorkerUnitValue, ReadyTownHallUnitsValue);
        if (SourceTownHallUnitValue == nullptr || SourceTownHallUnitValue->tag == ReceiverTownHallUnitValue.tag)
        {
            continue;
        }

        const int EffectiveAssignedHarvesterCountValue =
            std::max(SourceTownHallUnitValue->assigned_harvesters, 0) +
            GetPlannedHarvesterDeltaForTownHall(PlannedInboundCountsByTownHallTagValue,
                                                PlannedOutboundCountsByTownHallTagValue,
                                                SourceTownHallUnitValue->tag);
        const int SourceHarvesterSurplusValue =
            EffectiveAssignedHarvesterCountValue - SourceTownHallUnitValue->ideal_harvesters;
        if (SourceHarvesterSurplusValue <= 0)
        {
            continue;
        }

        const bool WorkerIsCarryingMineralsValue = IsCarryingMinerals(*WorkerUnitValue);
        const float DistanceSquaredValue =
            DistanceSquared2D(Point2D(WorkerUnitValue->pos), Point2D(ReceiverTownHallUnitValue.pos));
        const bool ShouldReplaceBestWorkerValue =
            BestWorkerUnitValue == nullptr ||
            SourceHarvesterSurplusValue > BestSourceHarvesterSurplusValue ||
            (SourceHarvesterSurplusValue == BestSourceHarvesterSurplusValue &&
             ((BestWorkerIsCarryingMineralsValue && !WorkerIsCarryingMineralsValue) ||
              (BestWorkerIsCarryingMineralsValue == WorkerIsCarryingMineralsValue &&
               DistanceSquaredValue < BestDistanceSquaredValue)));
        if (!ShouldReplaceBestWorkerValue)
        {
            continue;
        }

        BestWorkerUnitValue = WorkerUnitValue;
        BestDistanceSquaredValue = DistanceSquaredValue;
        BestWorkerIsCarryingMineralsValue = WorkerIsCarryingMineralsValue;
        BestSourceHarvesterSurplusValue = SourceHarvesterSurplusValue;
    }

    return BestWorkerUnitValue;
}

int FTerranWorkerSelectionService::GetCommittedHarvesterCountForRefinery(
    const ObservationInterface& ObservationValue,
    const FAgentState& AgentStateValue,
    const Tag RefineryTagValue) const
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

int FTerranWorkerSelectionService::GetPlannedHarvesterDeltaForRefinery(
    const std::unordered_map<Tag, int>& PlannedFillCountsByRefineryTagValue,
    const std::unordered_map<Tag, int>& PlannedReliefCountsByRefineryTagValue,
    const Tag RefineryTagValue) const
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

int FTerranWorkerSelectionService::GetPlannedHarvesterDeltaForTownHall(
    const std::unordered_map<Tag, int>& PlannedInboundCountsByTownHallTagValue,
    const std::unordered_map<Tag, int>& PlannedOutboundCountsByTownHallTagValue,
    const Tag TownHallTagValue) const
{
    int PlannedHarvesterDeltaValue = 0;

    const std::unordered_map<Tag, int>::const_iterator PlannedInboundIteratorValue =
        PlannedInboundCountsByTownHallTagValue.find(TownHallTagValue);
    if (PlannedInboundIteratorValue != PlannedInboundCountsByTownHallTagValue.end())
    {
        PlannedHarvesterDeltaValue += PlannedInboundIteratorValue->second;
    }

    const std::unordered_map<Tag, int>::const_iterator PlannedOutboundIteratorValue =
        PlannedOutboundCountsByTownHallTagValue.find(TownHallTagValue);
    if (PlannedOutboundIteratorValue != PlannedOutboundCountsByTownHallTagValue.end())
    {
        PlannedHarvesterDeltaValue -= PlannedOutboundIteratorValue->second;
    }

    return PlannedHarvesterDeltaValue;
}

}  // namespace sc2
