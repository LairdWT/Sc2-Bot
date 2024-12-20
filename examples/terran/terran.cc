#pragma once

#include "terran.h"

namespace sc2 {

void TerranAgent::OnGameStart() {
    // sc2::renderer::Initialize("Feature layers", 50, 50, DRAW_SIZE_TWO, DRAW_SIZE_TWO);

    m_Observation = Observation();
    m_RawObservation = m_Observation->GetRawObservation();
    m_Render = &m_RawObservation->feature_layer_data().renders();
    m_MinimapRender = &m_RawObservation->feature_layer_data().minimap_renders();

    float rx = sc2::GetRandomScalar();
    float ry = sc2::GetRandomScalar();
    BarracksRally =
        sc2::Point2D(m_Observation->GetStartLocation().x + rx * 5.0f, m_Observation->GetStartLocation().y + ry * 5.0f);

    PrintAgentState();
}

void TerranAgent::OnStep() {
    m_Observation = Observation();

    UpdateAgentState();
    OnStepUnitUpdate();
    OnStepBuildUpdate();

    m_RawObservation = Observation()->GetRawObservation();
    m_Render = &m_RawObservation->feature_layer_data().renders();
    m_MinimapRender = &m_RawObservation->feature_layer_data().minimap_renders();

    // DrawFeatureLayerUnits8BPP(m_Render->unit_density(), 0, 0);
    // DrawFeatureLayer1BPP(m_Render->selected(), DRAW_SIZE, 0);
    // DrawFeatureLayerHeightMap8BPP(m_MinimapRender->height_map(), 0, DRAW_SIZE);
    // DrawFeatureLayer1BPP(m_MinimapRender->camera(), DRAW_SIZE, DRAW_SIZE);

    // sc2::renderer::Render();
    if (m_CurrentStep % 120 == 0) {
        PrintAgentState();
    }
}

void TerranAgent::OnGameEnd() {
    sc2::renderer::Shutdown();
}

void TerranAgent::OnUnitIdle(const sc2::Unit* unit) {
    switch (unit->unit_type.ToType()) {
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
            if (AgentState.Economy.Minerals < 51) {
                break;
            }
            Actions()->UnitCommand(unit, sc2::ABILITY_ID::TRAIN_MARINE);
            break;
        }

        case sc2::UNIT_TYPEID::TERRAN_MARINE: {
            float rx = sc2::GetRandomScalar();
            float ry = sc2::GetRandomScalar();
            BarracksRally = sc2::Point2D(m_Observation->GetStartLocation().x + rx * 15.0f,
                                         m_Observation->GetStartLocation().y + ry * 15.0f);
            Actions()->UnitCommand(unit, sc2::ABILITY_ID::ATTACK_ATTACK, BarracksRally);
            break;
        }

        default: {
            break;
        }
    }
}

void TerranAgent::OnUnitCreated(const sc2::Unit* unit) {
    return;
}

void TerranAgent::UpdateAgentState() {

    // Update Unit Counts
    for (const UNIT_TYPEID UnitType : TERRAN_UNIT_TYPES) {
        AgentState.Units.SetUnitCount(UnitType, CountUnitType(UnitType));
    }
    AgentState.Units.Update();

    for (const UNIT_TYPEID UnitType : TERRAN_BUILDING_TYPES) {
        AgentState.Buildings.SetBuildingCount(UnitType, CountUnitType(UnitType));
    }

    // Economy Resources
    AgentState.Economy.Minerals = m_Observation->GetMinerals();
    AgentState.Economy.Vespene = m_Observation->GetVespene();
    AgentState.Economy.Supply = m_Observation->GetFoodUsed();
    AgentState.Economy.SupplyCap = m_Observation->GetFoodCap();
    AgentState.Economy.SupplyAvailable =
        AgentState.Economy.SupplyCap - AgentState.Economy.Supply;
}

void TerranAgent::PrintAgentState() {
    AgentState.PrintStatus();
}

void TerranAgent::OnStepUnitUpdate() {
    if (!m_Observation) {
        SCLOG(LoggingVerbosity::error, "ERROR in TerranAgent::OnStepUnitUpdate() - m_Observation is null");
        return;
    }

    ControlledUnits = m_Observation->GetUnits(Unit::Alliance::Self);
    NeutralUnits = m_Observation->GetUnits(Unit::Alliance::Neutral);

    if (AgentState.Units.GetUnitCount(UNIT_TYPEID::TERRAN_MARINE) >= 24) {
        AllMarinesAttack();
    }
}

void TerranAgent::AllMarinesAttack() {
    sc2::Units Marines = m_Observation->GetUnits(sc2::Unit::Alliance::Self,
                                                    sc2::IsUnit(sc2::UNIT_TYPEID::TERRAN_MARINE));

    m_CurrentStep++;
    for (const sc2::Unit* Marine : Marines) {
        
        // if current step is divisible by 5, move to a random location
        if (m_CurrentStep % 3 == 1) {

            float rx = sc2::GetRandomScalar();
            float ry = sc2::GetRandomScalar();
            sc2::Point2D MoveLocation =
                sc2::Point2D(rx * 20.0f, ry * 5.0f) + m_Observation->GetGameInfo().enemy_start_locations.front();

            Actions()->UnitCommand(Marine, sc2::ABILITY_ID::MOVE_MOVE, MoveLocation);
        } else {
            float rx = sc2::GetRandomScalar();
            float ry = sc2::GetRandomScalar();
            sc2::Point2D AttackLocation =
                sc2::Point2D(rx * 15.0f, ry * 15.0f) + m_Observation->GetGameInfo().enemy_start_locations.front();

            Actions()->UnitCommand(Marine, sc2::ABILITY_ID::ATTACK_ATTACK, AttackLocation);
        }
    }
}

void TerranAgent::OnStepBuildUpdate() {

    for (const UNIT_TYPEID UnitType : TERRAN_BUILDING_TYPES) {
        AgentState.Buildings.SetCurrentlyInConstruction(UnitType, 0);
    }

    for (const sc2::Unit* unit : ControlledUnits) {
        if (unit->is_building && !unit->IsBuildFinished()) {
            AgentState.Buildings.IncrementCurrentlyInConstruction(unit->unit_type.ToType());
        }
    }

    TryBuildSupplyDepot();
    TryBuildBarracks();
}

bool TerranAgent::TryBuildStructure(sc2::ABILITY_ID StructureAbilityId, sc2::UNIT_TYPEID UnitTypeId) {
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

bool TerranAgent::TryBuildSupplyDepot() {
    // Check if we are close to the supply cap
    if (AgentState.Economy.Supply <= (AgentState.Economy.SupplyCap - (AgentState.Economy.SupplyCap / 6))) {
        return false;
    }

    // If no barracks exist yet, don't build more supply depots than needed
    if (AgentState.Buildings.GetBarracksCount() < 1 && AgentState.Buildings.GetSupplyDepotCount() > 0) {
        return false;
    }

    // Limit the number of supply depots being built concurrently to avoid over-issuing commands
    const int max_supply_depots_in_progress = 2;
    if (AgentState.Buildings.GetCurrentlyInConstruction(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) >=
        max_supply_depots_in_progress) {
        return false;
    }

    if (TryBuildStructure(sc2::ABILITY_ID::BUILD_SUPPLYDEPOT, sc2::UNIT_TYPEID::TERRAN_SCV)) {
        return true;
    }

    return false;
}

bool TerranAgent::TryBuildBarracks() {
    // Ensure there are enough supply depots, command centers, and workers before building a barracks
    if (AgentState.Buildings.GetSupplyDepotCount() < 1 || AgentState.Buildings.GetTownHallCount() < 1 ||
        AgentState.Units.GetWorkerCount() < 8) {
        return false;
    }

    // Ensure there are enough supply depots if building multiple barracks
    if (AgentState.Buildings.GetSupplyDepotCount() < 2 && AgentState.Buildings.GetBarracksCount() > 0) {
        return false;
    }

    // Avoid building too many barracks if mineral count is low
    if (AgentState.Buildings.GetBarracksCount() > 4 && AgentState.Economy.Minerals < 600) {
        return false;
    }

    // Prevent nearly getting supply blocked before building a barracks
    if (m_Observation->GetFoodCap() - m_Observation->GetFoodUsed() < 2) {
        return false;
    }

    // Limit the number of barracks being built concurrently
    const int max_barracks_in_progress = 2;
    if (AgentState.Buildings.GetCurrentlyInConstruction(UNIT_TYPEID::TERRAN_BARRACKS) >= max_barracks_in_progress) {
        return false;
    }

    if (TryBuildStructure(sc2::ABILITY_ID::BUILD_BARRACKS, sc2::UNIT_TYPEID::TERRAN_SCV)) {
        return true;
    }

    return false;
}

const sc2::Unit* TerranAgent::FindNearestMineralPatch(const sc2::Point2D& Origin) {
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

}  // end namespace sc2
