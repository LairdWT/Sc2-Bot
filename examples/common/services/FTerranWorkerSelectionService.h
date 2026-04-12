#pragma once

#include "common/services/IWorkerSelectionService.h"

namespace sc2
{

class FTerranWorkerSelectionService : public IWorkerSelectionService
{
public:
    FTerranWorkerSelectionService() = default;
    ~FTerranWorkerSelectionService() override = default;

    const Unit* SelectWorkerForRefinery(
        const ObservationInterface& ObservationValue,
        const FAgentState& AgentStateValue,
        const FCommandAuthoritySchedulingState& SchedulingStateValue,
        const FIntentBuffer& IntentBufferValue,
        const Unit& RefineryUnitValue,
        const std::unordered_set<Tag>& ReservedWorkerTagsValue) const override;

    const Unit* SelectWorkerForGasRelief(
        const ObservationInterface& ObservationValue,
        const FAgentState& AgentStateValue,
        const FCommandAuthoritySchedulingState& SchedulingStateValue,
        const FIntentBuffer& IntentBufferValue,
        const Unit& RefineryUnitValue,
        const std::unordered_set<Tag>& ReservedWorkerTagsValue) const override;

    const Unit* SelectWorkerForMineralRebalance(
        const ObservationInterface& ObservationValue,
        const FAgentState& AgentStateValue,
        const FCommandAuthoritySchedulingState& SchedulingStateValue,
        const FIntentBuffer& IntentBufferValue,
        const Unit& ReceiverTownHallUnitValue,
        const Units& ReadyTownHallUnitsValue,
        const std::unordered_set<Tag>& ReservedWorkerTagsValue,
        const std::unordered_map<Tag, int>& PlannedInboundCountsByTownHallTagValue,
        const std::unordered_map<Tag, int>& PlannedOutboundCountsByTownHallTagValue) const override;

    int GetCommittedHarvesterCountForRefinery(
        const ObservationInterface& ObservationValue,
        const FAgentState& AgentStateValue,
        const Tag RefineryTagValue) const override;

    int GetPlannedHarvesterDeltaForRefinery(
        const std::unordered_map<Tag, int>& PlannedFillCountsByRefineryTagValue,
        const std::unordered_map<Tag, int>& PlannedReliefCountsByRefineryTagValue,
        const Tag RefineryTagValue) const override;

    int GetPlannedHarvesterDeltaForTownHall(
        const std::unordered_map<Tag, int>& PlannedInboundCountsByTownHallTagValue,
        const std::unordered_map<Tag, int>& PlannedOutboundCountsByTownHallTagValue,
        const Tag TownHallTagValue) const override;
};

}  // namespace sc2
