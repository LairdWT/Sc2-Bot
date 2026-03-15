#pragma once

#include "sc2api/sc2_unit.h"

namespace sc2
{

inline Unit MakeSelfBuildingUnit(const Tag TagValue, const UNIT_TYPEID UnitTypeIdValue, const float BuildProgressValue)
{
    Unit UnitValue;
    UnitValue.display_type = Unit::Visible;
    UnitValue.alliance = Unit::Self;
    UnitValue.tag = TagValue;
    UnitValue.unit_type = UnitTypeIdValue;
    UnitValue.owner = 1;
    UnitValue.pos = Point3D(0.0f, 0.0f, 0.0f);
    UnitValue.facing = 0.0f;
    UnitValue.radius = 0.5f;
    UnitValue.build_progress = BuildProgressValue;
    UnitValue.cloak = Unit::NotCloaked;
    UnitValue.detect_range = 0.0f;
    UnitValue.radar_range = 0.0f;
    UnitValue.is_selected = false;
    UnitValue.is_on_screen = true;
    UnitValue.is_blip = false;
    UnitValue.health = 100.0f;
    UnitValue.health_max = 100.0f;
    UnitValue.shield = 0.0f;
    UnitValue.shield_max = 0.0f;
    UnitValue.energy = 0.0f;
    UnitValue.energy_max = 0.0f;
    UnitValue.mineral_contents = 0;
    UnitValue.vespene_contents = 0;
    UnitValue.is_flying = false;
    UnitValue.is_burrowed = false;
    UnitValue.is_hallucination = false;
    UnitValue.weapon_cooldown = 0.0f;
    UnitValue.add_on_tag = NullTag;
    UnitValue.cargo_space_taken = 0;
    UnitValue.cargo_space_max = 0;
    UnitValue.assigned_harvesters = 0;
    UnitValue.ideal_harvesters = 0;
    UnitValue.engaged_target_tag = NullTag;
    UnitValue.is_powered = true;
    UnitValue.is_alive = true;
    UnitValue.last_seen_game_loop = 0;
    UnitValue.attack_upgrade_level = 0;
    UnitValue.armor_upgrade_level = 0;
    UnitValue.shield_upgrade_level = 0;
    UnitValue.is_building = true;
    return UnitValue;
}

}  // namespace sc2
