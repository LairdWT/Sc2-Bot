#pragma once

#include <cstdint>
#include <iostream>
#include <limits>
#include <unordered_set>
#include <vector>

#include "common/bot_status_models.h"
#include "common/economic_models.h"
#include "common/logging.h"
#include "common/render_settings.h"

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
    void PrintAgentState();

    void ProduceRecoveryIntents(const FFrameContext& Frame);
    void ProduceStructureBuildIntents(const FFrameContext& Frame);
    void ProduceUnitProductionIntents(const FFrameContext& Frame);
    void ProduceArmyIntents(const FFrameContext& Frame);
    void ExecuteResolvedIntents(const FFrameContext& Frame, const std::vector<FUnitIntent>& Intents);

    void AllMarinesAttack();

    bool TryBuildStructure(ABILITY_ID StructureAbilityId, UNIT_TYPEID WorkerTypeId, int Priority);
    bool TryBuildSupplyDepot();
    bool TryBuildBarracks();
    bool ShouldBuildSCV() const;
    bool TryBuildSCV();
    bool ShouldBuildMarine() const;
    bool TryBuildMarine();
    bool ShouldLaunchMarineAttack() const;

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
};

}  // namespace sc2
