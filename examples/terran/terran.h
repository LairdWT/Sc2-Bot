#pragma once

#include <cstdint>
#include <iostream>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "common/bot_status_models.h"
#include "common/descriptors/FTerranGameStateDescriptorBuilder.h"
#include "common/descriptors/FTerranForecastStateBuilder.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/economy/FEconomyDomainState.h"
#include "common/logging.h"
#include "common/planning/FTerranArmyPlanner.h"
#include "common/planning/FTerranArmyOrderExpander.h"
#include "common/planning/FTerranArmyUnitExecutionPlanner.h"
#include "common/planning/FTerranCommandTaskAdmissionService.h"
#include "common/planning/FTerranRampWallController.h"
#include "common/planning/FTerranCommandTaskPriorityService.h"
#include "common/planning/FTerranEconomyProductionOrderExpander.h"
#include "common/planning/FTerranSquadOrderExpander.h"
#include "common/planning/FTerranTimingAttackBuildPlanner.h"
#include "common/planning/FCommandAuthorityProcessor.h"
#include "common/planning/FDefaultStrategicDirector.h"
#include "common/planning/IArmyPlanner.h"
#include "common/planning/IArmyOrderExpander.h"
#include "common/planning/IBuildPlanner.h"
#include "common/planning/ICommandTaskPriorityService.h"
#include "common/planning/FProductionRallyState.h"
#include "common/planning/IEconomyProductionOrderExpander.h"
#include "common/planning/FIntentSchedulingService.h"
#include "common/planning/ISquadOrderExpander.h"
#include "common/planning/IStrategicDirector.h"
#include "common/planning/IUnitExecutionPlanner.h"
#include "common/render_settings.h"
#include "common/descriptors/FTerranEnemyObservationBuilder.h"
#include "common/descriptors/IEnemyObservationBuilder.h"
#include "common/services/FTerranBuildPlacementService.h"
#include "common/services/FTerranWorkerSelectionService.h"
#include "common/services/IBuildPlacementService.h"
#include "common/services/IWorkerSelectionService.h"
#include "common/telemetry/FAgentExecutionTelemetry.h"

#include "sc2api/sc2_api.h"
#include "sc2api/sc2_client.h"
#include "sc2api/sc2_common.h"
#include "sc2api/sc2_typeenums.h"
#include "sc2api/sc2_unit.h"
#include "sc2api/sc2_unit_filters.h"
#include "sc2renderer/sc2_renderer.h"
#include "sc2utils/sc2_manage_process.h"

namespace sc2
{

class TerranAgent : public Agent
{
public:
    void OnGameStart() final;
    void OnStep() final;
    void OnGameEnd() final;
    void OnUnitIdle(const Unit* UnitPtr) final;
    void OnUnitCreated(const Unit* UnitPtr) final;
    void OnBuildingConstructionComplete(const Unit* UnitPtr) final;

    void UpdateAgentState(const FFrameContext& Frame);
    void InitializeRampWallDescriptor(const FFrameContext& Frame);
    void InitializeMainBaseLayoutDescriptor(const FFrameContext& Frame);
    void RebuildObservedGameStateDescriptor(const FFrameContext& Frame);
    void RebuildEnemyObservationDescriptor(const FFrameContext& Frame);
    void RebuildForecastState();
    void RebuildExecutionPressureDescriptor(const FFrameContext& Frame);
    void UpdateStrategicAndPlanningState();
    void UpdateRallyAnchor();
    void PrintAgentState();
    void PrintWallState() const;
    FBuildPlacementContext CreateBuildPlacementContext() const;

    void ProduceRecoveryIntents(const FFrameContext& Frame);
    void ProduceSchedulerIntents(const FFrameContext& Frame);
    void ProduceWallGateIntents(const FFrameContext& Frame);
    void ProduceWorkerHarvestIntents(const FFrameContext& Frame);
    void ProduceProductionRallyIntents();
    void ExecuteProductionRallyIntents();
    void UpdateExecutionTelemetry(const FFrameContext& Frame);
    void ExecuteResolvedIntents(const FFrameContext& Frame, const std::vector<FUnitIntent>& Intents);
    void ExecuteOrbitalAbilities(const FFrameContext& Frame);
    void UpdateDispatchedSchedulerOrders(const FFrameContext& Frame);
    void CaptureNewlyDispatchedSchedulerOrders(const FFrameContext& Frame);

    uint32_t CountOrdersAndIntentsForAbility(ABILITY_ID AbilityIdValue) const;
    uint32_t GetObservedCountForOrder(const FCommandOrderRecord& CommandOrderRecordValue) const;
    uint32_t GetObservedInConstructionCountForOrder(const FCommandOrderRecord& CommandOrderRecordValue) const;
    bool HasProducerConfirmedDispatchedOrder(const FCommandOrderRecord& CommandOrderRecordValue,
                                             const Unit* ActorUnitValue) const;

    const Unit* FindNearestMineralPatch(const Point2D& OriginPointValue) const;
    const Unit* FindNearestMineralPatchForTownHall(const Point2D& TownHallPointValue) const;
    const Unit* FindNearestReadyTownHall(const Point2D& OriginPointValue) const;
    const Unit* SelectRecoveryMineralPatchForWorker(const Unit& WorkerUnitValue) const;
    Point2D GetEnemyTargetLocation() const;

    Units NeutralUnits;
    Point2D ArmyAssemblyPoint;
    Point2D ProductionRallyPoint;
    FAgentState AgentState;
    std::vector<FUnitIntent> ResolvedIntents;

private:
    void ProduceWorkerMineralRebalanceIntents(const FFrameContext& Frame,
                                              std::unordered_set<Tag>& ReservedWorkerTagsValue);

    inline void DrawFeatureLayer1BPP(const SC2APIProtocol::ImageData& ImageData, int OffsetX, int OffsetY)
    {
        renderer::Matrix1BPP(ImageData.data().c_str(), ImageData.size().x(), ImageData.size().y(), OffsetX, OffsetY,
                             PIXEL_DRAW_SIZE, PIXEL_DRAW_SIZE);
    }

    inline void DrawFeatureLayerUnits8BPP(const SC2APIProtocol::ImageData& ImageData, int OffsetX, int OffsetY)
    {
        renderer::Matrix8BPPPlayers(ImageData.data().c_str(), ImageData.size().x(), ImageData.size().y(), OffsetX,
                                    OffsetY, PIXEL_DRAW_SIZE, PIXEL_DRAW_SIZE);
    }

    inline void DrawFeatureLayerHeightMap8BPP(const SC2APIProtocol::ImageData& ImageData, int OffsetX, int OffsetY)
    {
        renderer::Matrix8BPPHeightMap(ImageData.data().c_str(), ImageData.size().x(), ImageData.size().y(), OffsetX,
                                      OffsetY, PIXEL_DRAW_SIZE, PIXEL_DRAW_SIZE);
    }

    const ObservationInterface* ObservationPtr{nullptr};
    uint64_t CurrentStep{0};
    FIntentBuffer IntentBuffer;
    FIntentArbiter IntentArbiter;
    std::unordered_set<Tag> PendingRecoveryWorkers;
    FGameStateDescriptor GameStateDescriptor;
    FEconomyDomainState EconomyDomainState;
    FTerranGameStateDescriptorBuilder DefaultGameStateDescriptorBuilder;
    const IGameStateDescriptorBuilder* GameStateDescriptorBuilder{&DefaultGameStateDescriptorBuilder};
    FTerranForecastStateBuilder DefaultForecastStateBuilder;
    FDefaultStrategicDirector DefaultStrategicDirector;
    const IStrategicDirector* StrategicDirector{&DefaultStrategicDirector};
    FTerranTimingAttackBuildPlanner DefaultBuildPlanner;
    const IBuildPlanner* BuildPlanner{&DefaultBuildPlanner};
    FTerranArmyPlanner DefaultArmyPlanner;
    const IArmyPlanner* ArmyPlanner{&DefaultArmyPlanner};
    FTerranCommandTaskPriorityService DefaultCommandTaskPriorityService;
    const ICommandTaskPriorityService* CommandTaskPriorityService{&DefaultCommandTaskPriorityService};
    FTerranCommandTaskAdmissionService DefaultCommandTaskAdmissionService;
    const ICommandTaskAdmissionService* CommandTaskAdmissionService{&DefaultCommandTaskAdmissionService};
    FCommandAuthorityProcessor CommandAuthorityProcessor;
    FTerranArmyOrderExpander DefaultArmyOrderExpander;
    const IArmyOrderExpander* ArmyOrderExpander{&DefaultArmyOrderExpander};
    FTerranSquadOrderExpander DefaultSquadOrderExpander;
    const ISquadOrderExpander* SquadOrderExpander{&DefaultSquadOrderExpander};
    FTerranArmyUnitExecutionPlanner DefaultUnitExecutionPlanner;
    const IUnitExecutionPlanner* UnitExecutionPlanner{&DefaultUnitExecutionPlanner};
    FTerranEconomyProductionOrderExpander DefaultEconomyProductionOrderExpander;
    const IEconomyProductionOrderExpander* EconomyProductionOrderExpander{&DefaultEconomyProductionOrderExpander};
    FTerranRampWallController DefaultWallGateController;
    const IWallGateController* WallGateController{&DefaultWallGateController};
    FTerranBuildPlacementService DefaultBuildPlacementService;
    const IBuildPlacementService* BuildPlacementService{&DefaultBuildPlacementService};
    FTerranWorkerSelectionService DefaultWorkerSelectionService;
    const IWorkerSelectionService* WorkerSelectionService{&DefaultWorkerSelectionService};
    FTerranEnemyObservationBuilder DefaultEnemyObservationBuilder;
    const IEnemyObservationBuilder* EnemyObservationBuilder{&DefaultEnemyObservationBuilder};
    FIntentSchedulingService IntentSchedulingService;
    std::vector<Point2D> ExpansionLocations;
    std::unordered_map<Tag, FProductionRallyState> ProductionRallyStates;
    std::vector<FUnitIntent> PendingProductionRallyIntents;
    FAgentExecutionTelemetry ExecutionTelemetry;
    EWallGateState CurrentWallGateState{EWallGateState::Unavailable};
    uint32_t LastArmyExecutionOrderCount{0U};
    uint64_t LastStepMicroseconds{0U};
    uint64_t LastAgentStateUpdateMicroseconds{0U};
    uint64_t LastDescriptorRebuildMicroseconds{0U};
    uint64_t LastDispatchMaintenanceMicroseconds{0U};
    uint64_t LastSchedulerStrategicProcessingMicroseconds{0U};
    uint64_t LastSchedulerEconomyProcessingMicroseconds{0U};
    uint64_t LastSchedulerArmyProcessingMicroseconds{0U};
    uint64_t LastSchedulerSquadProcessingMicroseconds{0U};
    uint64_t LastSchedulerUnitExecutionProcessingMicroseconds{0U};
    uint64_t LastSchedulerDrainMicroseconds{0U};
    uint64_t LastIntentResolutionMicroseconds{0U};
    uint64_t LastIntentExecutionMicroseconds{0U};
    uint64_t LastDispatchCaptureMicroseconds{0U};
    uint32_t LastProductionRallyApplyCount{0U};
    uint64_t RecentProductionRallyCounterWindowStartStep{0U};
    uint32_t RecentProductionRallyApplyCount{0U};
    uint32_t LastBlockerReliefMoveCount{0U};
    uint32_t LastUnitExecutionReplanCount{0U};
    uint32_t LastActiveIndexedExecutionOrderCount{0U};
    uint64_t LastTerminalCompactionStep{0U};
};

}  // namespace sc2
