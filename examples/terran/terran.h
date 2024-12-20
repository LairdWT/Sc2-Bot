#pragma once

#include <cstdint>
#include <iostream>
#include <limits>

#include "common/logging.h"
#include "common/render_settings.h"
#include "common/bot_status_models.h"
#include "common/economic_models.h"

#include "sc2api/sc2_api.h"             // For Agent, ObservationInterface, ABILITY_ID, UNIT_TYPEID, etc.
#include "sc2api/sc2_client.h"          // For Client and Agent
#include "sc2api/sc2_common.h"          // For Point2D
#include "sc2api/sc2_typeenums.h"       // For ABILITY_ID, UNIT_TYPEID
#include "sc2api/sc2_unit.h"            // For Unit, Units
#include <sc2api/sc2_unit_filters.h>    // For IsUnit type filter
#include "sc2renderer/sc2_renderer.h"
#include "sc2utils/sc2_manage_process.h"

namespace sc2 {

class TerranAgent : public Agent {
public:
    void OnGameStart() final;

    void OnStep() final;

    void OnGameEnd() final;

    void OnUnitIdle(const sc2::Unit* unit) final;

    void OnUnitCreated(const sc2::Unit* unit) final;


    //~ Agent State ~//
    void UpdateAgentState();

    void PrintAgentState();

    //~ Unit events
    inline void OnStepUnitUpdate();

    inline size_t CountUnitType(const sc2::UNIT_TYPEID unit_type) {
        return m_Observation->GetUnits(sc2::Unit::Alliance::Self, sc2::IsUnit(unit_type)).size();
    }

    void AllMarinesAttack();

    //~ Build Structure Actions
    void OnStepBuildUpdate();

    bool TryBuildStructure(sc2::ABILITY_ID StructureAbilityId, sc2::UNIT_TYPEID UnitTypeId);

    bool TryBuildSupplyDepot();

    bool TryBuildBarracks();

    //~ Build Unit Actions
    bool ShouldBuildMarine();

    bool TryBuildMarine();


    //~ Environment Query
    const sc2::Unit* FindNearestMineralPatch(const sc2::Point2D& Origin);

    Units NeutralUnits;
    Point2D BarracksRally;

    //~ Agent State ~//
    FAgentState AgentState;

private:

    inline void DrawFeatureLayer1BPP(const SC2APIProtocol::ImageData& image_data, int off_x, int off_y) {
        renderer::Matrix1BPP(image_data.data().c_str(), image_data.size().x(), image_data.size().y(), off_x, off_y,
                                  PIXEL_DRAW_SIZE, PIXEL_DRAW_SIZE);
    }

    inline void DrawFeatureLayerUnits8BPP(const SC2APIProtocol::ImageData& image_data, int off_x, int off_y) {
        renderer::Matrix8BPPPlayers(image_data.data().c_str(), image_data.size().x(), image_data.size().y(), off_x,
                                         off_y, PIXEL_DRAW_SIZE, PIXEL_DRAW_SIZE);
    }

    inline void DrawFeatureLayerHeightMap8BPP(const SC2APIProtocol::ImageData& image_data, int off_x, int off_y) {
        renderer::Matrix8BPPHeightMap(image_data.data().c_str(), image_data.size().x(), image_data.size().y(),
                                           off_x, off_y, PIXEL_DRAW_SIZE, PIXEL_DRAW_SIZE);
    }

    const ObservationInterface* m_Observation{nullptr};
    const SC2APIProtocol::Observation* m_RawObservation{nullptr};
    const SC2APIProtocol::FeatureLayers* m_Render{nullptr};
    const SC2APIProtocol::FeatureLayersMinimap* m_MinimapRender{nullptr};

    uint64_t m_CurrentStep{0};
};

}  // end namespace sc2
