#pragma once

#include <unordered_map>
#include <unordered_set>

#include "sc2api/sc2_unit.h"

namespace sc2
{

struct FAgentState;
struct FCommandAuthoritySchedulingState;
struct FIntentBuffer;

class IWorkerSelectionService
{
public:
    virtual ~IWorkerSelectionService() = default;

    virtual const Unit* SelectWorkerForRefinery(
        const ObservationInterface& ObservationValue,
        const FAgentState& AgentStateValue,
        const FCommandAuthoritySchedulingState& SchedulingStateValue,
        const FIntentBuffer& IntentBufferValue,
        const Unit& RefineryUnitValue,
        const std::unordered_set<Tag>& ReservedWorkerTagsValue) const = 0;

    virtual const Unit* SelectWorkerForGasRelief(
        const ObservationInterface& ObservationValue,
        const FAgentState& AgentStateValue,
        const FCommandAuthoritySchedulingState& SchedulingStateValue,
        const FIntentBuffer& IntentBufferValue,
        const Unit& RefineryUnitValue,
        const std::unordered_set<Tag>& ReservedWorkerTagsValue) const = 0;

    virtual const Unit* SelectWorkerForMineralRebalance(
        const ObservationInterface& ObservationValue,
        const FAgentState& AgentStateValue,
        const FCommandAuthoritySchedulingState& SchedulingStateValue,
        const FIntentBuffer& IntentBufferValue,
        const Unit& ReceiverTownHallUnitValue,
        const Units& ReadyTownHallUnitsValue,
        const std::unordered_set<Tag>& ReservedWorkerTagsValue,
        const std::unordered_map<Tag, int>& PlannedInboundCountsByTownHallTagValue,
        const std::unordered_map<Tag, int>& PlannedOutboundCountsByTownHallTagValue) const = 0;

    virtual int GetCommittedHarvesterCountForRefinery(
        const ObservationInterface& ObservationValue,
        const FAgentState& AgentStateValue,
        const Tag RefineryTagValue) const = 0;

    virtual int GetPlannedHarvesterDeltaForRefinery(
        const std::unordered_map<Tag, int>& PlannedFillCountsByRefineryTagValue,
        const std::unordered_map<Tag, int>& PlannedReliefCountsByRefineryTagValue,
        const Tag RefineryTagValue) const = 0;

    virtual int GetPlannedHarvesterDeltaForTownHall(
        const std::unordered_map<Tag, int>& PlannedInboundCountsByTownHallTagValue,
        const std::unordered_map<Tag, int>& PlannedOutboundCountsByTownHallTagValue,
        const Tag TownHallTagValue) const = 0;
};

}  // namespace sc2
