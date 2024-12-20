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
        if (!NewUnit) {
            return;  // Ensure the input is valid
        }

        // Add the new unit to the controlled units container
        ControlledUnits.push_back(NewUnit);

        // Synchronize all related containers
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
        WeaponCooldownRemaining.push_back(NewUnit->weapon_cooldown);
        Orders.push_back(NewUnit->orders);
        EngagedTargetTag.push_back(NewUnit->engaged_target_tag);
        Passengers.push_back(NewUnit->passengers);
        CargoSpaceOccupied.push_back(NewUnit->cargo_space_taken);
        CargoSpaceMax.push_back(NewUnit->cargo_space_max);
        AddonTag.push_back(NewUnit->add_on_tag);
        AssignedHarvisters.push_back(NewUnit->assigned_harvesters);
        IdealHarvesters.push_back(NewUnit->ideal_harvesters);
        UnitBuffs.push_back(NewUnit->buffs);

        // Add a default state flag for the new unit
        StateFlags.StateFlags.push_back(0);

        // Set initial state flags based on the unit's attributes
        size_t Index = ControlledUnits.size() - 1;  // Index of the new unit
        if (NewUnit->is_alive) {
            StateFlags.SetFlag(Index, IsAlive);
        }
        if (NewUnit->is_building) {
            StateFlags.SetFlag(Index, IsABuilding);
        }
        if (NewUnit->is_flying) {
            StateFlags.SetFlag(Index, IsFlying);
        }
        if (NewUnit->is_burrowed) {
            StateFlags.SetFlag(Index, IsBurrowed);
        }
        if (NewUnit->is_hallucination) {
            StateFlags.SetFlag(Index, IsHallucination);
        }
        if (NewUnit->is_selected) {
            StateFlags.SetFlag(Index, IsSelected);
        }
        if (NewUnit->is_on_screen) {
            StateFlags.SetFlag(Index, IsOnScreen);
        }
        if (NewUnit->is_blip) {
            StateFlags.SetFlag(Index, IsASensorTowerBlip);
        }

        // Ensure all containers stay synchronized in size
        FilteredUnits.resize(ControlledUnits.size(), false);
    }

    // Add multiple units to the container
    void AddUnits(const std::vector<const Unit*>& NewUnits) {
        for (const auto& Unit : NewUnits) {
            AddUnit(Unit);
        }
    }

    void ResetAll() {
        ControlledUnits.clear();
        FilteredUnits.clear();
        SelectedUnits.clear();
        Alliances.clear();
        Tags.clear();
        UnitTypes.clear();
        Positions.clear();
        FacingRadians.clear();
        UnitRadius.clear();
        BuildProgress.clear();
        CloakStates.clear();
        Health.clear();
        HealthMax.clear();
        Shield.clear();
        ShieldMax.clear();
        Energy.clear();
        EnergyMax.clear();
        StateFlags.StateFlags.clear();
        WeaponCooldownRemaining.clear();
        Orders.clear();
        EngagedTargetTag.clear();
        Passengers.clear();
        CargoSpaceOccupied.clear();
        CargoSpaceMax.clear();
        AddonTag.clear();
        AssignedHarvisters.clear();
        IdealHarvesters.clear();
    }

    void SetUnits(const std::vector<const Unit*>& NewUnits) {
        ResetAll();
        ControlledUnits.reserve(NewUnits.size());
        FilteredUnits.reserve(NewUnits.size());
        SelectedUnits.reserve(NewUnits.size());
        AddUnits(NewUnits);
    }

    void ResetFilteredUnits() {
        for (size_t Index = 0; Index < FilteredUnits.size(); ++Index) {
            FilteredUnits[Index] = true;
        }
    }

    void ResetSelectedUnits() {
        SelectedUnits.clear();
        SelectedUnits.reserve(ControlledUnits.size());
    }

void FilterByType(const UnitTypeID UnitType) {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && UnitTypes[Index] != UnitType) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByAlliance(const sc2::Unit::Alliance UnitAlliance) {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && Alliances[Index] != UnitAlliance) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsAlive() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && !StateFlags.IsFlagSet(Index, IsAlive)) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsUnit() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && !IsTerranUnit(ControlledUnits[Index]->unit_type.ToType())) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsBuilding() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && !IsTerranBuilding(ControlledUnits[Index]->unit_type.ToType())) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsWorker() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            const UNIT_TYPEID UnitType = ControlledUnits[Index]->unit_type.ToType();
            if (FilteredUnits[Index] && UnitType != UNIT_TYPEID::TERRAN_SCV) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByConstructionIsFinished() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && BuildProgress[Index] < 1.0f) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsConstructionNotFinished() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && BuildProgress[Index] >= 1.0f) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsGroundUnit() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && StateFlags.IsFlagSet(Index, IsFlying)) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsFlyingUnit() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && !StateFlags.IsFlagSet(Index, IsFlying)) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsNotBurrowed() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && StateFlags.IsFlagSet(Index, IsBurrowed)) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsBurrowed() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && !StateFlags.IsFlagSet(Index, IsBurrowed)) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsNotHallucination() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && StateFlags.IsFlagSet(Index, IsHallucination)) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsHallucination() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && !StateFlags.IsFlagSet(Index, IsHallucination)) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsSelected() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && !StateFlags.IsFlagSet(Index, IsSelected)) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsNotSelected() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && StateFlags.IsFlagSet(Index, IsSelected)) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsOnScreen() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && !StateFlags.IsFlagSet(Index, IsOnScreen)) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsNotOnScreen() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && StateFlags.IsFlagSet(Index, IsOnScreen)) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsASensorTowerBlip() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && !StateFlags.IsFlagSet(Index, IsASensorTowerBlip)) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsNotASensorTowerBlip() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && StateFlags.IsFlagSet(Index, IsASensorTowerBlip)) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsCloaked() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && CloakStates[Index] != sc2::Unit::CloakState::Cloaked) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsNotCloaked() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && CloakStates[Index] == sc2::Unit::CloakState::Cloaked) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsCloakedDetected() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && CloakStates[Index] != sc2::Unit::CloakState::CloakedDetected) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsNotCloakedDetected() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && CloakStates[Index] == sc2::Unit::CloakState::CloakedDetected) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsNotCloakedUnknown() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && CloakStates[Index] == sc2::Unit::CloakState::CloakedUnknown) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsWeaponOnCooldown() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && WeaponCooldownRemaining[Index] <= 0.0f) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByWeaponAvailable() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && WeaponCooldownRemaining[Index] > 0.0f) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByUnitBuffActive(const BuffID Buff) {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] &&
                std::find(UnitBuffs[Index].begin(), UnitBuffs[Index].end(), Buff) == UnitBuffs[Index].end()) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByUnitOrderTargetTag(const Tag TargetTag) {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            bool MatchFound = false;
            if (FilteredUnits[Index]) {
                for (const auto& Order : Orders[Index]) {
                    if (Order.target_unit_tag == TargetTag) {
                        MatchFound = true;
                        break;
                    }
                }
                if (!MatchFound) {
                    FilteredUnits[Index] = false;
                }
            }
        }
    }

    void FilterByUnitOrderTargetPosition(const Point2D TargetPos) {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            bool MatchFound = false;
            if (FilteredUnits[Index]) {
                for (const auto& Order : Orders[Index]) {
                    if (Order.target_pos == TargetPos) {
                        MatchFound = true;
                        break;
                    }
                }
                if (!MatchFound) {
                    FilteredUnits[Index] = false;
                }
            }
        }
    }

    void FilterByEngagedTargetTag(const Tag TargetTag) {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && EngagedTargetTag[Index] != TargetTag) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByCargoSpaceOccupied(const int OccupiedSpace) {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && CargoSpaceOccupied[Index] != OccupiedSpace) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByCargoSpaceMax(const int MaxSpace) {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && CargoSpaceMax[Index] != MaxSpace) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByCargoSpaceAvailable(const int AvailableSpace) {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && (CargoSpaceMax[Index] - CargoSpaceOccupied[Index]) < AvailableSpace) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByPassengerCount(const int PassengerCount) {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && Passengers[Index].size() != PassengerCount) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByAddonTag(const Tag Addon) {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && AddonTag[Index] != Addon) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByAssignedHarvisters(const int Harvisters) {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && AssignedHarvisters[Index] != Harvisters) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIdealHarvesters(const int Harvisters) {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index] && IdealHarvesters[Index] != Harvisters) {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsTrainingUnit() {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index]) {
                for (const auto& Order : Orders[Index]) {
                    if (IsTrainTerranUnit(Order.ability_id)) {
                        FilteredUnits[Index] = false;
                    }
                }
            }
        }
    }

    void SelectFilteredUnits() {
        ResetSelectedUnits();
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index) {
            if (FilteredUnits[Index]) {
                SelectedUnits.push_back(ControlledUnits[Index]);
            }
        }
    }

    std::vector<const Unit*>& GetSelectedUnits() {
        SelectFilteredUnits();
        return SelectedUnits;
    }

    std::vector<const Unit*>& GetUnits() {
        ResetFilteredUnits();
        FilterByIsUnit();
        FilterByConstructionIsFinished();
        return GetSelectedUnits();
    }

    std::vector<const Unit*>& GetBuildings() {
        ResetFilteredUnits();
        FilterByIsBuilding();
        FilterByConstructionIsFinished();
        return GetSelectedUnits();
    }

    std::vector<const Unit*>& GetBuildingsInConstruction() {
        ResetFilteredUnits();
        FilterByIsBuilding();
        FilterByIsConstructionNotFinished();
        return GetSelectedUnits();
    }

    std::vector<const Unit*>& GetWorkers() {
        ResetFilteredUnits();
        FilterByIsWorker();
        return GetSelectedUnits();
    }

    const Unit* GetUnitByTag(const Tag UnitTag) {
        for (const auto& Unit : ControlledUnits) {
            if (Unit->tag == UnitTag) {
                return Unit;
            }
        }
        return nullptr;
    }

    const Unit* GetBuildingByTag(const Tag BuildingTag) {
        for (const auto& Unit : ControlledUnits) {
            if (Unit->tag == BuildingTag && IsTerranBuilding(Unit->unit_type.ToType())) {
                return Unit;
            }
        }
        return nullptr;
    }

    const Unit* GetIdleWorker() {
        for (uint16_t Index = 0; Index < UnitTypes.size(); ++Index) {
            switch (UnitTypes[Index].ToType())
            case UNIT_TYPEID::TERRAN_SCV:
                if (ControlledUnits[Index]->orders.empty()) {
                    return ControlledUnits[Index];
                }
                break;
            continue;
        }
        return nullptr;
    }

    const Unit* GetFirstIdleBarracks() {
        ResetFilteredUnits();
        FilterByType(UNIT_TYPEID::TERRAN_BARRACKS);
        FilterByConstructionIsFinished();
        std::vector<const Unit*>& Barracks = GetSelectedUnits();
        for (const Unit* Barrack : Barracks) {
            if (Barrack->orders.empty()) {
                return Barrack;
            }
        }
        return nullptr;
    }
};

}  // namespace sc2
