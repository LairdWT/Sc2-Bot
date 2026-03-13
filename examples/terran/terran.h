#pragma once

#include <cstdint>
#include <iostream>
#include <limits>
#include <unordered_set>
#include <vector>

#include "common/bot_status_models.h"
#include "common/descriptors/FTerranGameStateDescriptorBuilder.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/logging.h"
#include "common/planning/FTerranArmyPlanner.h"
#include "common/planning/FTerranEconomyProductionOrderExpander.h"
#include "common/planning/FTerranTimingAttackBuildPlanner.h"
#include "common/planning/FCommandAuthorityProcessor.h"
#include "common/planning/FDefaultStrategicDirector.h"
#include "common/planning/IArmyPlanner.h"
#include "common/planning/IBuildPlanner.h"
#include "common/planning/IEconomyProductionOrderExpander.h"
#include "common/planning/FIntentSchedulingService.h"
#include "common/planning/IStrategicDirector.h"
#include "common/render_settings.h"
#include "common/services/FTerranBuildPlacementService.h"
#include "common/services/IBuildPlacementService.h"
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

    void UpdateAgentState(const FFrameContext& Frame);
    void RebuildGameStateDescriptor(const FFrameContext& Frame);
    void UpdateRallyAnchor();
    void PrintAgentState();

    void ProduceRecoveryIntents(const FFrameContext& Frame);
    void ProduceSchedulerOpeningIntents(const FFrameContext& Frame);
    void ProduceArmyIntents(const FFrameContext& Frame);
    void UpdateExecutionTelemetry(const FFrameContext& Frame);
    void ExecuteResolvedIntents(const FFrameContext& Frame, const std::vector<FUnitIntent>& Intents);
    void CompleteDispatchedSchedulerOrders();

    void AllMarinesAttack();

    uint32_t CountOrdersAndIntentsForAbility(ABILITY_ID AbilityIdValue) const;
    bool ShouldLaunchMarineAttack() const;

    bool ShouldRefreshArmyOrder(const Unit& UnitValue, ABILITY_ID AbilityValue, const Point2D& TargetPointValue) const;

    const Unit* FindNearestMineralPatch(const Point2D& Origin);
    Point2D GetEnemyTargetLocation() const;
    Point2D GetRandomPointNear(const Point2D& Origin, float XRadius, float YRadius) const;

    Units NeutralUnits;
    Point2D BarracksRally;
    FAgentState AgentState;
    std::vector<FUnitIntent> ResolvedIntents;

private:
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
    FTerranGameStateDescriptorBuilder DefaultGameStateDescriptorBuilder;
    const IGameStateDescriptorBuilder* GameStateDescriptorBuilder{&DefaultGameStateDescriptorBuilder};
    FDefaultStrategicDirector DefaultStrategicDirector;
    const IStrategicDirector* StrategicDirector{&DefaultStrategicDirector};
    FTerranTimingAttackBuildPlanner DefaultBuildPlanner;
    const IBuildPlanner* BuildPlanner{&DefaultBuildPlanner};
    FTerranArmyPlanner DefaultArmyPlanner;
    const IArmyPlanner* ArmyPlanner{&DefaultArmyPlanner};
    FCommandAuthorityProcessor CommandAuthorityProcessor;
    FTerranEconomyProductionOrderExpander DefaultEconomyProductionOrderExpander;
    const IEconomyProductionOrderExpander* EconomyProductionOrderExpander{&DefaultEconomyProductionOrderExpander};
    FTerranBuildPlacementService DefaultBuildPlacementService;
    const IBuildPlacementService* BuildPlacementService{&DefaultBuildPlacementService};
    FIntentSchedulingService IntentSchedulingService;
    std::vector<Point2D> ExpansionLocations;
    FAgentExecutionTelemetry ExecutionTelemetry;
};

}  // namespace sc2
