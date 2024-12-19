#pragma once

#include <vector>

#include "sc2api/sc2_unit.h"
#include "terran_models.h"


namespace sc2 {

// Define State Flags as an Enum
enum StateFlag : uint8_t {
    IsAlive = 1 << 0,            // Bit 0
    IsABuilding = 1 << 1,        // Bit 1
    IsFlying = 1 << 2,           // Bit 2
    IsBurrowed = 1 << 3,         // Bit 3
    IsHallucination = 1 << 4,    // Bit 4
    IsSelected = 1 << 5,         // Bit 5
    IsOnScreen = 1 << 6,         // Bit 6
    IsASensorTowerBlip = 1 << 7  // Bit 7
};

struct FUnitStateFlags {

    std::vector<uint8_t> StateFlags;

    FUnitStateFlags() : StateFlags(std::vector<uint8_t>()) {
    }

    void SetFlag(const size_t Index, const StateFlag Flag) {
        if (Index < StateFlags.size()) {
            StateFlags[Index] |= Flag;
        }
    }

    void ClearFlag(const size_t Index, const StateFlag Flag) {
        if (Index < StateFlags.size()) {
            StateFlags[Index] &= ~Flag;
        }
    }

    bool IsFlagSet(const size_t Index, const StateFlag Flag) const {
        if (Index < StateFlags.size()) {
            return (StateFlags[Index] & Flag) != 0;
        }
        return false;
    }
};

struct FTerranUnitContainer {

    // Actual Units
    std::vector<const Unit*> ControlledUnits;

    // Filtering containers
    std::vector<bool> FilteredUnits;
    std::vector<const Unit*> SelectedUnits;

    // Identification
 
    // Relationship of the unit to this player.
    std::vector<sc2::Unit::Alliance> Alliances;
    // A unique identifier for the instance of a unit.
    std::vector<Tag> Tags;
    // An identifier of the type of unit.
    std::vector<UnitTypeID> UnitTypes;


    // World Transform Data

    // Position of the unit in the world.
    std::vector<Point3D> Positions;
    // Direction the unit faces in radians (1 radian == 57.296 degrees)
    std::vector<float> FacingRadians;
    // Radius of the unit.
    std::vector<float> UnitRadius;
    // Gives progress under construction. Range: [0.0, 1.0]. 1.0 == finished.
    std::vector<float> BuildProgress;

    // If the unit is cloaked.
    std::vector<sc2::Unit::CloakState> CloakStates;

    // Attributes
    std::vector<float> Health;
    std::vector<float> HealthMax;
    std::vector<float> Shield;
    std::vector<float> ShieldMax;
    std::vector<float> Energy;
    std::vector<float> EnergyMax;

    // Current buffs on this unit.
    std::vector<std::vector<BuffID>> UnitBuffs;

    // State Flags (packed bitfield)
    // bIsAlive             = bit 0
    // bIsABuilding         = bit 1
    // bIsFlying            = bit 2
    // bIsBurrowed          = bit 3
    // bIsHallucination     = bit 4
    // bIsSelected          = bit 5
    // bIsOnScreen          = bit 6
    // bIsASensorTowerBlip  = bit 7
    FUnitStateFlags StateFlags;

    // The cooldown of the unit's weapon.
    std::vector<float> WeaponCooldownRemaining;

    // Orders on a unit. Only valid for this player's units.
    std::vector<std::vector<UnitOrder>> Orders;

    // Target unit of a unit.
    std::vector<Tag> EngagedTargetTag;

    // Transport Data
    std::vector<std::vector<PassengerUnit>> Passengers;
    std::vector<int> CargoSpaceOccupied;
    std::vector<int> CargoSpaceMax;

    // Add-on like a tech lab or reactor.
    std::vector<Tag> AddonTag;

    // Workers associated with a town hall (e.g., Command Center).
    std::vector<int> AssignedHarvisters;
    std::vector<int> IdealHarvesters;

    FTerranUnitContainer() = default;

    // Add a single unit to the container
    void AddUnit(const Unit* NewUnit) {
        ControlledUnits.push_back(NewUnit);
        Alliances.push_back(NewUnit->alliance);
        Tags.push_back(NewUnit->tag);
        UnitTypes.push_back(NewUnit->unit_type);
        Positions.push_back(NewUnit->pos);
        FacingRadians.push_back(NewUnit->facing);
        UnitRadius.push_back(NewUnit->radius);
        BuildProgress.push_back(NewUnit->build_progress);
        CloakStates.push_back(NewUnit->cloak);
        Health.push_back(NewUnit->health);
        HealthMax.push_back(NewUnit->health_max);
        Shield.push_back(NewUnit->shield);
        ShieldMax.push_back(NewUnit->shield_max);
        Energy.push_back(NewUnit->energy);
        EnergyMax.push_back(NewUnit->energy_max);

        // Check the state of the unit and set the appropriate flags
        StateFlags.StateFlags.push_back(0);
        if (NewUnit->is_alive)
            StateFlags.SetFlag(ControlledUnits.size() - 1, IsAlive);
        if (NewUnit->is_building)
            StateFlags.SetFlag(ControlledUnits.size() - 1, IsABuilding);
        if (NewUnit->is_flying)
            StateFlags.SetFlag(ControlledUnits.size() - 1, IsFlying);
        if (NewUnit->is_burrowed)
            StateFlags.SetFlag(ControlledUnits.size() - 1, IsBurrowed);
        if (NewUnit->is_hallucination)
            StateFlags.SetFlag(ControlledUnits.size() - 1, IsHallucination);
        if (NewUnit->is_selected)
            StateFlags.SetFlag(ControlledUnits.size() - 1, IsSelected);
        if (NewUnit->is_on_screen)
            StateFlags.SetFlag(ControlledUnits.size() - 1, IsOnScreen);
        if (NewUnit->is_blip)
            StateFlags.SetFlag(ControlledUnits.size() - 1, IsASensorTowerBlip);

        WeaponCooldownRemaining.push_back(NewUnit->weapon_cooldown);
        Orders.push_back(NewUnit->orders);
        EngagedTargetTag.push_back(NewUnit->engaged_target_tag);
        Passengers.push_back(NewUnit->passengers);
        CargoSpaceOccupied.push_back(NewUnit->cargo_space_taken);
        CargoSpaceMax.push_back(NewUnit->cargo_space_max);
        AddonTag.push_back(NewUnit->add_on_tag);
        AssignedHarvisters.push_back(NewUnit->assigned_harvesters);
        IdealHarvesters.push_back(NewUnit->ideal_harvesters);
    }

    // Add multiple units to the container
    void AddUnits(const std::vector<const Unit*>& NewUnits) {
        for (const auto& Unit : NewUnits) {
            AddUnit(Unit);
        }
    }

    void ResetFilteredUnits() {
        FilteredUnits.clear();
        FilteredUnits.resize(ControlledUnits.size(), false);
    }

    void ResetSelectedUnits() {
        SelectedUnits.clear();
    }

    void FilterByType(const UnitTypeID UnitType) {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (UnitTypes[i] == UnitType) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByAlliance(const sc2::Unit::Alliance UnitAlliance) {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (Alliances[i] == UnitAlliance) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByIsAlive() {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (StateFlags.IsFlagSet(i, IsAlive)) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByIsUnit() {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (!StateFlags.IsFlagSet(i, IsABuilding)) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByIsBuilding() {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (StateFlags.IsFlagSet(i, IsABuilding)) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByIsGroundUnit() {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (!StateFlags.IsFlagSet(i, IsFlying)) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByIsFlyingUnit() {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (StateFlags.IsFlagSet(i, IsFlying)) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByIsNotBurrowed() {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (!StateFlags.IsFlagSet(i, IsBurrowed)) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByIsBurrowed() {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (StateFlags.IsFlagSet(i, IsBurrowed)) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByIsNotHallucination() {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (!StateFlags.IsFlagSet(i, IsHallucination)) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByIsHallucination() {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (StateFlags.IsFlagSet(i, IsHallucination)) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByIsSelected() {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (StateFlags.IsFlagSet(i, IsSelected)) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByIsNotSelected() {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (!StateFlags.IsFlagSet(i, IsSelected)) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByIsOnScreen() {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (StateFlags.IsFlagSet(i, IsOnScreen)) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByIsNotOnScreen() {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (!StateFlags.IsFlagSet(i, IsOnScreen)) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByIsASensorTowerBlip() {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (StateFlags.IsFlagSet(i, IsASensorTowerBlip)) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByIsNotASensorTowerBlip() {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (!StateFlags.IsFlagSet(i, IsASensorTowerBlip)) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByIsCloaked() {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (CloakStates[i] == sc2::Unit::CloakState::Cloaked) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByIsNotCloaked() {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (CloakStates[i] != sc2::Unit::CloakState::Cloaked) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByIsCloakedDetected() {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (CloakStates[i] == sc2::Unit::CloakState::CloakedDetected) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByIsNotCloakedDetected() {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (CloakStates[i] != sc2::Unit::CloakState::CloakedDetected) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByIsNotCloakedUnknown() {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (CloakStates[i] != sc2::Unit::CloakState::CloakedUnknown) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByIsWeaponOnCooldown() {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (WeaponCooldownRemaining[i] > 0.0f) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByWeaponAvailable() {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (WeaponCooldownRemaining[i] <= 0.0f) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByUnitBuffActive(const BuffID Buff) {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (std::find(UnitBuffs[i].begin(), UnitBuffs[i].end(), Buff) != UnitBuffs[i].end()) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByUnitOrderTargetTag(const Tag TargetTag) {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            for (const auto& Order : Orders[i]) {
                if (Order.target_unit_tag == TargetTag) {
                    FilteredUnits[i] = true;
                    break;
                }
            }
        }
    }

    void FilterByUnitOrderTargetPosition(const Point2D TargetPos) {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            for (const auto& Order : Orders[i]) {
                if (Order.target_pos == TargetPos) {
                    FilteredUnits[i] = true;
                    break;
                }
            }
        }
    }

    void FilterByEngagedTargetTag(const Tag TargetTag) {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (EngagedTargetTag[i] == TargetTag) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByCargoSpaceOccupied(const int OccupiedSpace) {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (CargoSpaceOccupied[i] == OccupiedSpace) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByCargoSpaceMax(const int MaxSpace) {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (CargoSpaceMax[i] == MaxSpace) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByCargoSpaceAvailable(const int AvailableSpace) {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (CargoSpaceMax[i] - CargoSpaceOccupied[i] >= AvailableSpace) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByPassengerCount(const int PassengerCount) {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (Passengers[i].size() == PassengerCount) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByAddonTag(const Tag Addon) {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (AddonTag[i] == Addon) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByAssignedHarvisters(const int Harvisters) {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (AssignedHarvisters[i] == Harvisters) {
                FilteredUnits[i] = true;
            }
        }
    }

    void FilterByIdealHarvesters(const int Harvisters) {
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (IdealHarvesters[i] == Harvisters) {
                FilteredUnits[i] = true;
            }
        }
    }

    void SelectFilteredUnits() {
        ResetSelectedUnits();
        for (size_t i = 0; i < ControlledUnits.size(); ++i) {
            if (FilteredUnits[i]) {
                SelectedUnits.push_back(ControlledUnits[i]);
            }
        }
    }

    std::vector<const Unit*>& GetSelectedUnits() {
        SelectFilteredUnits();
        return SelectedUnits;
    }
};

}  // namespace sc2
