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
    PrintAgentState();
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
            if (AgentState.EconomyResources.Minerals < 51) {
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

void TerranAgent::UpdateAgentState() {
    // Economy Resources
    AgentState.EconomyResources.Workers = CountUnitType(sc2::UNIT_TYPEID::TERRAN_SCV);
    AgentState.EconomyResources.SupplyDepots = CountUnitType(sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT);
    AgentState.EconomyResources.CommandCenters = CountUnitType(sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER);
    AgentState.EconomyResources.Minerals = m_Observation->GetMinerals();
    AgentState.EconomyResources.Vespene = m_Observation->GetVespene();
    AgentState.EconomyResources.Supply = m_Observation->GetFoodUsed();
    AgentState.EconomyResources.SupplyCap = m_Observation->GetFoodCap();
    AgentState.EconomyResources.SupplyAvailable =
        AgentState.EconomyResources.SupplyCap - AgentState.EconomyResources.Supply;

    // Military Resources
    AgentState.MilitaryResources.Marines = CountUnitType(sc2::UNIT_TYPEID::TERRAN_MARINE);
    AgentState.MilitaryResources.Marauders = CountUnitType(sc2::UNIT_TYPEID::TERRAN_MARAUDER);
    AgentState.MilitaryResources.Medivacs = CountUnitType(sc2::UNIT_TYPEID::TERRAN_MEDIVAC);

    AgentState.MilitaryResources.ArmyCount = AgentState.MilitaryResources.Marines +
                                             AgentState.MilitaryResources.Marauders +
                                             AgentState.MilitaryResources.Medivacs;

    AgentState.MilitaryResources.ArmyValue = (AgentState.MilitaryResources.Marines * 50) +
                                             (AgentState.MilitaryResources.Marauders * 125) +
                                             (AgentState.MilitaryResources.Medivacs * 200);

    AgentState.MilitaryResources.ArmySupply = AgentState.MilitaryResources.Marines +
                                             (AgentState.MilitaryResources.Marauders * 2.0f) +
                                             (AgentState.MilitaryResources.Medivacs * 2.0f);

    AgentState.MilitaryResources.Barracks = CountUnitType(sc2::UNIT_TYPEID::TERRAN_BARRACKS);
    AgentState.MilitaryResources.Factories = CountUnitType(sc2::UNIT_TYPEID::TERRAN_FACTORY);
    AgentState.MilitaryResources.Starports = CountUnitType(sc2::UNIT_TYPEID::TERRAN_STARPORT);
}

void TerranAgent::PrintAgentState() {
    AgentState.PrintStatus();
}

void TerranAgent::OnStepUnitUpdate() {
    if (!m_Observation) {
        SCLOG(LoggingVerbosity::error, "ERROR in TerranAgent::OnStepUnitUpdate() - m_Observation is null");
        return;
    }

    ControlledUnits = m_Observation->GetUnits(sc2::Unit::Alliance::Self);
    NeutralUnits = m_Observation->GetUnits(sc2::Unit::Alliance::Neutral);

    if (AgentState.MilitaryResources.Marines >= 24) {
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
    if (m_Observation->GetFoodUsed() <= (m_Observation->GetFoodCap() - (m_Observation->GetFoodCap() / 6))) {
        return false;
    }

    // If no barracks exist yet, don't build more supply depots than needed
    if (AgentState.MilitaryResources.Barracks < 1 &&
        AgentState.EconomyResources.SupplyDepots > 0) {
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

bool TerranAgent::TryBuildBarracks() {
    // Ensure there are enough supply depots, command centers, and workers before building a barracks
    if (AgentState.EconomyResources.SupplyDepots < 1 ||
        AgentState.EconomyResources.CommandCenters < 1 ||
        AgentState.EconomyResources.Workers < 8) {
        return false;
    }

    // Ensure there are enough supply depots if building multiple barracks
    if (AgentState.EconomyResources.SupplyDepots < 2 &&
        AgentState.MilitaryResources.Barracks > 0) {
        return false;
    }

    // Avoid building too many barracks if mineral count is low
    if (AgentState.MilitaryResources.Barracks > 4 && AgentState.EconomyResources.Minerals < 600) {
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