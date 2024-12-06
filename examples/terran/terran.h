#pragma once

#include <cstdint>
#include <iostream>
#include <limits>

#include "common/logging.h"
#include "common/render_settings.h"
#include "sc2api/sc2_api.h"        // For Agent, ObservationInterface, ABILITY_ID, UNIT_TYPEID, etc.
#include "sc2api/sc2_client.h"     // For Client and Agent
#include "sc2api/sc2_common.h"     // For Point2D
#include "sc2api/sc2_typeenums.h"  // For ABILITY_ID, UNIT_TYPEID
#include "sc2api/sc2_unit.h"       // For Unit, Units
#include <sc2api/sc2_unit_filters.h> // For IsUnit type filter
#include "sc2renderer/sc2_renderer.h"
#include "sc2utils/sc2_manage_process.h"

class TerranAgent : public sc2::Agent {
public:
    virtual void OnGameStart() final {
        sc2::renderer::Initialize("Feature layers", 50, 50, DRAW_SIZE_TWO, DRAW_SIZE_TWO);

        m_Observation = Observation();
        m_RawObservation = m_Observation->GetRawObservation();
        m_Render = &m_RawObservation->feature_layer_data().renders();
        m_MinimapRender = &m_RawObservation->feature_layer_data().minimap_renders();

        float rx = sc2::GetRandomScalar();
        float ry = sc2::GetRandomScalar();
        BarracksRally = sc2::Point2D(m_Observation->GetStartLocation().x + rx * 5.0f,
                                     m_Observation->GetStartLocation().y + ry * 5.0f);
    }

    virtual void OnStep() final {
        m_Observation = Observation();

        OnStepUnitUpdate();
        OnStepBuildUpdate();

        m_RawObservation = Observation()->GetRawObservation();
        m_Render = &m_RawObservation->feature_layer_data().renders();
        m_MinimapRender = &m_RawObservation->feature_layer_data().minimap_renders();

        DrawFeatureLayerUnits8BPP(m_Render->unit_density(), 0, 0);
        DrawFeatureLayer1BPP(m_Render->selected(), DRAW_SIZE, 0);
        DrawFeatureLayerHeightMap8BPP(m_MinimapRender->height_map(), 0, DRAW_SIZE);
        DrawFeatureLayer1BPP(m_MinimapRender->camera(), DRAW_SIZE, DRAW_SIZE);

        sc2::renderer::Render();
    }

    virtual void OnGameEnd() final {
        sc2::renderer::Shutdown();
    }

    virtual void OnUnitIdle(const sc2::Unit* unit) final
    {
        switch (unit->unit_type.ToType())
        {
            case sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER: {
                if (CountUnitType(sc2::UNIT_TYPEID::TERRAN_SCV) < 26)
                Actions()->UnitCommand(unit, sc2::ABILITY_ID::TRAIN_SCV);
                break;
            }

            case sc2::UNIT_TYPEID::TERRAN_SCV: {
                const sc2::Unit* NearestMineralPatch = FindNearestMineralPatch(unit->pos);
                if (!NearestMineralPatch) {
                    Actions()->UnitCommand(unit, sc2::ABILITY_ID::ATTACK_ATTACK,
                                           m_Observation->GetGameInfo().enemy_start_locations.front());
                    break;
                }
                Actions()->UnitCommand(unit, sc2::ABILITY_ID::SMART, NearestMineralPatch);
                break;
            }

            case sc2::UNIT_TYPEID::TERRAN_BARRACKS: {
                if (m_Observation->GetMinerals() < 101) {
                    break;
                }
                Actions()->UnitCommand(unit, sc2::ABILITY_ID::TRAIN_MARINE);
                break;
            }

            case sc2::UNIT_TYPEID::TERRAN_MARINE: {
                float rx = sc2::GetRandomScalar();
                float ry = sc2::GetRandomScalar();
                BarracksRally = sc2::Point2D(m_Observation->GetStartLocation().x + rx * 5.0f,
                                             m_Observation->GetStartLocation().y + ry * 5.0f);
                Actions()->UnitCommand(unit, sc2::ABILITY_ID::ATTACK_ATTACK, BarracksRally);
                break;
            }

            default:
            {
                break;
            }
        }
    }

    // Unit events

    inline void OnStepUnitUpdate() {
        if (!m_Observation) {
            SCLOG(LoggingVerbosity::error, "ERROR in TerranAgent::OnStepUnitUpdate() - m_Observation is null");
            return;
        }

        ControlledUnits = m_Observation->GetUnits(sc2::Unit::Alliance::Self);
        NeutralUnits = m_Observation->GetUnits(sc2::Unit::Alliance::Neutral);

        if (CountUnitType(sc2::UNIT_TYPEID::TERRAN_MARINE) >= 18) {
            AllMarinesAttack();
        }
    }

    size_t CountUnitType(const sc2::UNIT_TYPEID unit_type) {
        return m_Observation->GetUnits(sc2::Unit::Alliance::Self, sc2::IsUnit(unit_type)).size();
    }

    inline void AllMarinesAttack() {
        sc2::Units Marines = m_Observation->GetUnits(sc2::Unit::Alliance::Self,
                                                     sc2::IsUnit(sc2::UNIT_TYPEID::TERRAN_MARINE));

        float rx = sc2::GetRandomScalar();
        float ry = sc2::GetRandomScalar();
        sc2::Point2D AttackLocation =
            sc2::Point2D(rx * 5.0f, ry * 5.0f) + m_Observation->GetGameInfo().enemy_start_locations.front();

        for (const sc2::Unit* Marine : Marines) {
            Actions()->UnitCommand(Marine, sc2::ABILITY_ID::ATTACK_ATTACK, AttackLocation);
        }

        std::cout << "All Marines Attack!" << std::endl;
    }

    // Build Structure Actions
    inline void OnStepBuildUpdate() {
        TryBuildSupplyDepot();
        TryBuildBarracks();
    }

inline bool TryBuildStructure(sc2::ABILITY_ID StructureAbilityId, sc2::UNIT_TYPEID UnitTypeId) {
        const sc2::Unit* UnitToBuild = nullptr;
        float ShortestDistance = std::numeric_limits<float>::max();
        sc2::Point2D BuildPosition = sc2::Point2D(50.0f, 50.0f);

        for (const sc2::Unit* ControlledUnit : ControlledUnits) {
            if (ControlledUnit->unit_type == UnitTypeId && ControlledUnit->orders.empty()) {
                float distance = sc2::DistanceSquared2D(ControlledUnit->pos, BuildPosition);
                if (distance < ShortestDistance) {
                    UnitToBuild = ControlledUnit;
                    ShortestDistance = distance;
                }
            }
        }

        if (!UnitToBuild) {
            for (const sc2::Unit* ControlledUnit : ControlledUnits) {
                if (ControlledUnit->unit_type == UnitTypeId) {
                    if (!ControlledUnit->orders.empty() &&
                        ControlledUnit->orders[0].ability_id == sc2::ABILITY_ID::HARVEST_GATHER) {
                        float distance = sc2::DistanceSquared2D(ControlledUnit->pos, BuildPosition);
                        if (distance < ShortestDistance) {
                            UnitToBuild = ControlledUnit;
                            ShortestDistance = distance;
                        }
                    }
                }
            }
        }

        if (!UnitToBuild) {
            for (const sc2::Unit* ControlledUnit : ControlledUnits) {
                if (ControlledUnit->unit_type == UnitTypeId) {
                    float distance = sc2::DistanceSquared2D(ControlledUnit->pos, BuildPosition);
                    if (distance < ShortestDistance) {
                        UnitToBuild = ControlledUnit;
                        ShortestDistance = distance;
                    }
                }
            }
        }

        if (!UnitToBuild) {
            return false;
        }

        float rx = sc2::GetRandomScalar();
        float ry = sc2::GetRandomScalar();

        Actions()->UnitCommand(UnitToBuild, StructureAbilityId,
                               sc2::Point2D(UnitToBuild->pos.x + rx * 15.0f, UnitToBuild->pos.y + ry * 15.0f));

        return true;
    }

inline bool TryBuildSupplyDepot() {
        // Check if we are close to the supply cap
        if (m_Observation->GetFoodUsed() <= (m_Observation->GetFoodCap() - (m_Observation->GetFoodCap() / 6))) {
            return false;
        }

        // If no barracks exist yet, don't build more supply depots than needed
        if (CountUnitType(sc2::UNIT_TYPEID::TERRAN_BARRACKS) < 1 &&
            CountUnitType(sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT) > 0) {
            return false;
        }

        // check how many supply depots are currently being built
        int supply_depots_in_progress = 0;
        for (const sc2::Unit* unit : ControlledUnits) {
            for (const sc2::UnitOrder& order : unit->orders) {
                if (order.ability_id == sc2::ABILITY_ID::BUILD_SUPPLYDEPOT) {
                    supply_depots_in_progress++;
                }
            }
        }

        // Limit the number of supply depots being built concurrently to avoid over-issuing commands
        const int max_supply_depots_in_progress = 2;
        if (supply_depots_in_progress >= max_supply_depots_in_progress) {
            return false;
        }

        return TryBuildStructure(sc2::ABILITY_ID::BUILD_SUPPLYDEPOT, sc2::UNIT_TYPEID::TERRAN_SCV);
    }

inline bool TryBuildBarracks() {
        // Ensure there are enough supply depots, command centers, and workers before building a barracks
        if (CountUnitType(sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT) < 1 ||
            CountUnitType(sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER) < 1 ||
            CountUnitType(sc2::UNIT_TYPEID::TERRAN_SCV) < 8) {
            return false;
        }

        // Ensure there are enough supply depots if building multiple barracks
        if (CountUnitType(sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT) < 2 &&
            CountUnitType(sc2::UNIT_TYPEID::TERRAN_BARRACKS) > 0) {
            return false;
        }

        // Avoid building too many barracks if mineral count is low
        if (CountUnitType(sc2::UNIT_TYPEID::TERRAN_BARRACKS) > 2 && m_Observation->GetMinerals() < 500) {
            return false;
        }

        // Prevent nearly getting supply blocked before building a barracks
        if (m_Observation->GetFoodCap() - m_Observation->GetFoodUsed() < 2) {
            return false;
        }

        // Count the number of barracks currently being built
        int barracks_in_progress = 0;

        for (const sc2::Unit* unit : ControlledUnits) {
            for (const sc2::UnitOrder& order : unit->orders) {
                if (order.ability_id == sc2::ABILITY_ID::BUILD_BARRACKS) {
                    barracks_in_progress++;
                }
            }
        }

        // Limit the number of barracks being built concurrently
        const int max_barracks_in_progress = 2;
        if (barracks_in_progress >= max_barracks_in_progress) {
            return false;
        }

        return TryBuildStructure(sc2::ABILITY_ID::BUILD_BARRACKS, sc2::UNIT_TYPEID::TERRAN_SCV);
    }


    // Environment Query
    inline const sc2::Unit* FindNearestMineralPatch(const sc2::Point2D& Origin) {
        float NearestDistance = std::numeric_limits<float>::max();
        const sc2::Unit* Target = nullptr;

        for (const sc2::Unit* NeutralUnit : NeutralUnits) {
            if (NeutralUnit->unit_type == sc2::UNIT_TYPEID::NEUTRAL_MINERALFIELD) {
                sc2::Point2D Diff = NeutralUnit->pos - Origin;
                float Distance = sc2::Dot2D(Diff, Diff);
                if (Distance <= NearestDistance) {
                    NearestDistance = Distance;
                    Target = NeutralUnit;
                }
            }
        }
        return Target;
    }

    sc2::Units ControlledUnits;
    sc2::Units NeutralUnits;

    sc2::Point2D BarracksRally;

private:
    inline void DrawFeatureLayer1BPP(const SC2APIProtocol::ImageData& image_data, int off_x, int off_y) {
        sc2::renderer::Matrix1BPP(image_data.data().c_str(), image_data.size().x(), image_data.size().y(), off_x, off_y,
                                  PIXEL_DRAW_SIZE, PIXEL_DRAW_SIZE);
    }

    inline void DrawFeatureLayerUnits8BPP(const SC2APIProtocol::ImageData& image_data, int off_x, int off_y) {
        sc2::renderer::Matrix8BPPPlayers(image_data.data().c_str(), image_data.size().x(), image_data.size().y(), off_x,
                                         off_y, PIXEL_DRAW_SIZE, PIXEL_DRAW_SIZE);
    }

    inline void DrawFeatureLayerHeightMap8BPP(const SC2APIProtocol::ImageData& image_data, int off_x, int off_y) {
        sc2::renderer::Matrix8BPPHeightMap(image_data.data().c_str(), image_data.size().x(), image_data.size().y(),
                                           off_x, off_y, PIXEL_DRAW_SIZE, PIXEL_DRAW_SIZE);
    }

    const sc2::ObservationInterface* m_Observation{nullptr};
    const SC2APIProtocol::Observation* m_RawObservation{nullptr};
    const SC2APIProtocol::FeatureLayers* m_Render{nullptr};
    const SC2APIProtocol::FeatureLayersMinimap* m_MinimapRender{nullptr};
};
