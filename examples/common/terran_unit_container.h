#pragma once

#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/logging.h"
#include "sc2api/sc2_unit.h"
#include "terran_models.h"

namespace sc2
{

enum StateFlag : uint8_t
{
    IsAlive = 1 << 0,
    IsABuilding = 1 << 1,
    IsFlying = 1 << 2,
    IsBurrowed = 1 << 3,
    IsHallucination = 1 << 4,
    IsSelected = 1 << 5,
    IsOnScreen = 1 << 6,
    IsASensorTowerBlip = 1 << 7,
};

struct FUnitStateFlags
{
    std::vector<uint8_t> StateFlags;

    void SetFlag(size_t Index, StateFlag Flag)
    {
        if (Index < StateFlags.size())
        {
            StateFlags[Index] |= Flag;
        }
    }

    void ClearFlag(size_t Index, StateFlag Flag)
    {
        if (Index < StateFlags.size())
        {
            StateFlags[Index] &= ~Flag;
        }
    }

    bool IsFlagSet(size_t Index, StateFlag Flag) const
    {
        return Index < StateFlags.size() && (StateFlags[Index] & Flag) != 0;
    }
};

struct FTerranUnitContainer
{
    std::vector<const Unit*> ControlledUnits;
    std::vector<bool> FilteredUnits;
    std::vector<const Unit*> SelectedUnits;

    std::vector<Unit::Alliance> Alliances;
    std::vector<Tag> Tags;
    std::vector<UnitTypeID> UnitTypes;

    std::vector<Point3D> Positions;
    std::vector<float> FacingRadians;
    std::vector<float> UnitRadius;
    std::vector<float> BuildProgress;

    std::vector<Unit::CloakState> CloakStates;

    std::vector<float> Health;
    std::vector<float> HealthMax;
    std::vector<float> Shield;
    std::vector<float> ShieldMax;
    std::vector<float> Energy;
    std::vector<float> EnergyMax;

    std::vector<std::vector<BuffID>> UnitBuffs;

    FUnitStateFlags StateFlags;

    std::vector<float> WeaponCooldownRemaining;
    std::vector<std::vector<UnitOrder>> Orders;
    std::vector<Tag> EngagedTargetTag;

    std::vector<std::vector<PassengerUnit>> Passengers;
    std::vector<int> CargoSpaceOccupied;
    std::vector<int> CargoSpaceMax;

    std::vector<Tag> AddonTag;
    std::vector<int> AssignedHarvisters;
    std::vector<int> IdealHarvesters;
    std::unordered_map<Tag, size_t> TagToIndexMap;

    FTerranUnitContainer() = default;

    void AddUnit(const Unit* NewUnit)
    {
        if (!NewUnit)
        {
            return;
        }

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
        UnitBuffs.push_back(NewUnit->buffs);
        WeaponCooldownRemaining.push_back(NewUnit->weapon_cooldown);
        Orders.push_back(NewUnit->orders);
        EngagedTargetTag.push_back(NewUnit->engaged_target_tag);
        Passengers.push_back(NewUnit->passengers);
        CargoSpaceOccupied.push_back(NewUnit->cargo_space_taken);
        CargoSpaceMax.push_back(NewUnit->cargo_space_max);
        AddonTag.push_back(NewUnit->add_on_tag);
        AssignedHarvisters.push_back(NewUnit->assigned_harvesters);
        IdealHarvesters.push_back(NewUnit->ideal_harvesters);

        StateFlags.StateFlags.push_back(0);

        const size_t Index = ControlledUnits.size() - 1;
        if (NewUnit->is_alive)
        {
            StateFlags.SetFlag(Index, IsAlive);
        }
        if (NewUnit->is_building)
        {
            StateFlags.SetFlag(Index, IsABuilding);
        }
        if (NewUnit->is_flying)
        {
            StateFlags.SetFlag(Index, IsFlying);
        }
        if (NewUnit->is_burrowed)
        {
            StateFlags.SetFlag(Index, IsBurrowed);
        }
        if (NewUnit->is_hallucination)
        {
            StateFlags.SetFlag(Index, IsHallucination);
        }
        if (NewUnit->is_selected)
        {
            StateFlags.SetFlag(Index, IsSelected);
        }
        if (NewUnit->is_on_screen)
        {
            StateFlags.SetFlag(Index, IsOnScreen);
        }
        if (NewUnit->is_blip)
        {
            StateFlags.SetFlag(Index, IsASensorTowerBlip);
        }

        FilteredUnits.resize(ControlledUnits.size(), false);
        TagToIndexMap[NewUnit->tag] = ControlledUnits.size() - 1U;
        AssertSynchronizedSizes();
    }

    void AddUnits(const std::vector<const Unit*>& NewUnits)
    {
        for (const Unit* UnitPtr : NewUnits)
        {
            AddUnit(UnitPtr);
        }
    }

    void ResetAll()
    {
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
        UnitBuffs.clear();
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
        TagToIndexMap.clear();
        AssertSynchronizedSizes();
    }

    void SetUnits(const std::vector<const Unit*>& NewUnits)
    {
        ResetAll();

        ControlledUnits.reserve(NewUnits.size());
        FilteredUnits.reserve(NewUnits.size());
        SelectedUnits.reserve(NewUnits.size());
        Alliances.reserve(NewUnits.size());
        Tags.reserve(NewUnits.size());
        UnitTypes.reserve(NewUnits.size());
        Positions.reserve(NewUnits.size());
        FacingRadians.reserve(NewUnits.size());
        UnitRadius.reserve(NewUnits.size());
        BuildProgress.reserve(NewUnits.size());
        CloakStates.reserve(NewUnits.size());
        Health.reserve(NewUnits.size());
        HealthMax.reserve(NewUnits.size());
        Shield.reserve(NewUnits.size());
        ShieldMax.reserve(NewUnits.size());
        Energy.reserve(NewUnits.size());
        EnergyMax.reserve(NewUnits.size());
        UnitBuffs.reserve(NewUnits.size());
        StateFlags.StateFlags.reserve(NewUnits.size());
        WeaponCooldownRemaining.reserve(NewUnits.size());
        Orders.reserve(NewUnits.size());
        EngagedTargetTag.reserve(NewUnits.size());
        Passengers.reserve(NewUnits.size());
        CargoSpaceOccupied.reserve(NewUnits.size());
        CargoSpaceMax.reserve(NewUnits.size());
        AddonTag.reserve(NewUnits.size());
        AssignedHarvisters.reserve(NewUnits.size());
        IdealHarvesters.reserve(NewUnits.size());
        TagToIndexMap.reserve(NewUnits.size());

        AddUnits(NewUnits);
    }

    bool HasSynchronizedSizes() const
    {
        const size_t ExpectedSize = ControlledUnits.size();
        return FilteredUnits.size() == ExpectedSize && Alliances.size() == ExpectedSize && Tags.size() == ExpectedSize &&
               UnitTypes.size() == ExpectedSize && Positions.size() == ExpectedSize && FacingRadians.size() == ExpectedSize &&
               UnitRadius.size() == ExpectedSize && BuildProgress.size() == ExpectedSize && CloakStates.size() == ExpectedSize &&
               Health.size() == ExpectedSize && HealthMax.size() == ExpectedSize && Shield.size() == ExpectedSize &&
               ShieldMax.size() == ExpectedSize && Energy.size() == ExpectedSize && EnergyMax.size() == ExpectedSize &&
               UnitBuffs.size() == ExpectedSize && StateFlags.StateFlags.size() == ExpectedSize &&
               WeaponCooldownRemaining.size() == ExpectedSize && Orders.size() == ExpectedSize &&
               EngagedTargetTag.size() == ExpectedSize && Passengers.size() == ExpectedSize &&
               CargoSpaceOccupied.size() == ExpectedSize && CargoSpaceMax.size() == ExpectedSize &&
               AddonTag.size() == ExpectedSize && AssignedHarvisters.size() == ExpectedSize &&
               IdealHarvesters.size() == ExpectedSize && TagToIndexMap.size() == ExpectedSize;
    }

    void ResetFilteredUnits()
    {
        FilteredUnits.assign(ControlledUnits.size(), true);
    }

    void ResetSelectedUnits()
    {
        SelectedUnits.clear();
        SelectedUnits.reserve(ControlledUnits.size());
    }

    void FilterByType(UnitTypeID UnitType)
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && UnitTypes[Index] != UnitType)
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByAlliance(Unit::Alliance UnitAlliance)
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && Alliances[Index] != UnitAlliance)
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsAlive()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && !StateFlags.IsFlagSet(Index, IsAlive))
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsUnit()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && !IsTerranUnit(ControlledUnits[Index]->unit_type.ToType()))
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsBuilding()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && !IsTerranBuilding(ControlledUnits[Index]->unit_type.ToType()))
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsWorker()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && UnitTypes[Index].ToType() != UNIT_TYPEID::TERRAN_SCV)
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByConstructionIsFinished()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && BuildProgress[Index] < 1.0f)
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsConstructionNotFinished()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && BuildProgress[Index] >= 1.0f)
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsGroundUnit()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && StateFlags.IsFlagSet(Index, IsFlying))
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsFlyingUnit()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && !StateFlags.IsFlagSet(Index, IsFlying))
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsNotBurrowed()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && StateFlags.IsFlagSet(Index, IsBurrowed))
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsBurrowed()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && !StateFlags.IsFlagSet(Index, IsBurrowed))
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsNotHallucination()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && StateFlags.IsFlagSet(Index, IsHallucination))
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsHallucination()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && !StateFlags.IsFlagSet(Index, IsHallucination))
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsSelected()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && !StateFlags.IsFlagSet(Index, IsSelected))
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsNotSelected()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && StateFlags.IsFlagSet(Index, IsSelected))
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsOnScreen()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && !StateFlags.IsFlagSet(Index, IsOnScreen))
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsNotOnScreen()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && StateFlags.IsFlagSet(Index, IsOnScreen))
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsASensorTowerBlip()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && !StateFlags.IsFlagSet(Index, IsASensorTowerBlip))
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsNotASensorTowerBlip()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && StateFlags.IsFlagSet(Index, IsASensorTowerBlip))
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsCloaked()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && CloakStates[Index] != Unit::CloakState::Cloaked)
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsNotCloaked()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && CloakStates[Index] == Unit::CloakState::Cloaked)
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsCloakedDetected()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && CloakStates[Index] != Unit::CloakState::CloakedDetected)
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsNotCloakedDetected()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && CloakStates[Index] == Unit::CloakState::CloakedDetected)
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsNotCloakedUnknown()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && CloakStates[Index] == Unit::CloakState::CloakedUnknown)
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsWeaponOnCooldown()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && WeaponCooldownRemaining[Index] <= 0.0f)
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByWeaponAvailable()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && WeaponCooldownRemaining[Index] > 0.0f)
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByUnitBuffActive(BuffID Buff)
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] &&
                std::find(UnitBuffs[Index].begin(), UnitBuffs[Index].end(), Buff) == UnitBuffs[Index].end())
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByUnitOrderTargetTag(Tag TargetTag)
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            bool MatchFound = false;
            if (FilteredUnits[Index])
            {
                for (const UnitOrder& OrderValue : Orders[Index])
                {
                    if (OrderValue.target_unit_tag == TargetTag)
                    {
                        MatchFound = true;
                        break;
                    }
                }
                if (!MatchFound)
                {
                    FilteredUnits[Index] = false;
                }
            }
        }
    }

    void FilterByUnitOrderTargetPosition(const Point2D& TargetPos)
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            bool MatchFound = false;
            if (FilteredUnits[Index])
            {
                for (const UnitOrder& OrderValue : Orders[Index])
                {
                    if (OrderValue.target_pos == TargetPos)
                    {
                        MatchFound = true;
                        break;
                    }
                }
                if (!MatchFound)
                {
                    FilteredUnits[Index] = false;
                }
            }
        }
    }

    void FilterByEngagedTargetTag(Tag TargetTag)
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && EngagedTargetTag[Index] != TargetTag)
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByCargoSpaceOccupied(int OccupiedSpace)
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && CargoSpaceOccupied[Index] != OccupiedSpace)
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByCargoSpaceMax(int MaxSpace)
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && CargoSpaceMax[Index] != MaxSpace)
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByCargoSpaceAvailable(int AvailableSpace)
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && (CargoSpaceMax[Index] - CargoSpaceOccupied[Index]) < AvailableSpace)
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByPassengerCount(int PassengerCount)
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && static_cast<int>(Passengers[Index].size()) != PassengerCount)
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByAddonTag(Tag Addon)
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && AddonTag[Index] != Addon)
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByAssignedHarvisters(int Harvisters)
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && AssignedHarvisters[Index] != Harvisters)
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIdealHarvesters(int Harvisters)
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index] && IdealHarvesters[Index] != Harvisters)
            {
                FilteredUnits[Index] = false;
            }
        }
    }

    void FilterByIsTrainingUnit()
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index])
            {
                for (const UnitOrder& OrderValue : Orders[Index])
                {
                    if (IsTrainTerranUnit(OrderValue.ability_id))
                    {
                        FilteredUnits[Index] = false;
                        break;
                    }
                }
            }
        }
    }

    void SelectFilteredUnits()
    {
        ResetSelectedUnits();
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (FilteredUnits[Index])
            {
                SelectedUnits.push_back(ControlledUnits[Index]);
            }
        }
    }

    std::vector<const Unit*>& GetSelectedUnits()
    {
        SelectFilteredUnits();
        return SelectedUnits;
    }

    std::vector<const Unit*>& GetUnits()
    {
        ResetFilteredUnits();
        FilterByIsUnit();
        FilterByConstructionIsFinished();
        return GetSelectedUnits();
    }

    std::vector<const Unit*>& GetBuildings()
    {
        ResetFilteredUnits();
        FilterByIsBuilding();
        FilterByConstructionIsFinished();
        return GetSelectedUnits();
    }

    std::vector<const Unit*>& GetBuildingsInConstruction()
    {
        ResetFilteredUnits();
        FilterByIsBuilding();
        FilterByIsConstructionNotFinished();
        return GetSelectedUnits();
    }

    std::vector<const Unit*>& GetWorkers()
    {
        ResetFilteredUnits();
        FilterByIsWorker();
        return GetSelectedUnits();
    }

    const Unit* GetUnitByTag(const Tag UnitTagValue) const
    {
        const std::unordered_map<Tag, size_t>::const_iterator FoundIndexIterator = TagToIndexMap.find(UnitTagValue);
        if (FoundIndexIterator == TagToIndexMap.end() || FoundIndexIterator->second >= ControlledUnits.size())
        {
            return nullptr;
        }
        return ControlledUnits[FoundIndexIterator->second];
    }

    bool TryGetUnitIndexByTag(const Tag UnitTagValue, size_t& OutIndexValue) const
    {
        const std::unordered_map<Tag, size_t>::const_iterator FoundIndexIterator = TagToIndexMap.find(UnitTagValue);
        if (FoundIndexIterator == TagToIndexMap.end() || FoundIndexIterator->second >= ControlledUnits.size())
        {
            return false;
        }
        OutIndexValue = FoundIndexIterator->second;
        return true;
    }

    const Unit* GetBuildingByTag(Tag BuildingTag) const
    {
        for (const Unit* UnitPtr : ControlledUnits)
        {
            if (UnitPtr->tag == BuildingTag && IsTerranBuilding(UnitPtr->unit_type.ToType()))
            {
                return UnitPtr;
            }
        }
        return nullptr;
    }

    const Unit* GetIdleWorker() const
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (UnitTypes[Index].ToType() == UNIT_TYPEID::TERRAN_SCV && BuildProgress[Index] >= 1.0f &&
                Orders[Index].empty())
            {
                return ControlledUnits[Index];
            }
        }
        return nullptr;
    }

    const Unit* GetFirstIdleTownHall() const
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            const UNIT_TYPEID UnitType = UnitTypes[Index].ToType();
            if (BuildProgress[Index] < 1.0f || !Orders[Index].empty())
            {
                continue;
            }

            switch (UnitType)
            {
                case UNIT_TYPEID::TERRAN_COMMANDCENTER:
                case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
                    return ControlledUnits[Index];
                default:
                    break;
            }
        }
        return nullptr;
    }

    const Unit* GetFirstIdleBarracks() const
    {
        for (size_t Index = 0; Index < ControlledUnits.size(); ++Index)
        {
            if (UnitTypes[Index].ToType() == UNIT_TYPEID::TERRAN_BARRACKS && BuildProgress[Index] >= 1.0f &&
                Orders[Index].empty())
            {
                return ControlledUnits[Index];
            }
        }
        return nullptr;
    }

private:
    void AssertSynchronizedSizes() const
    {
        if (!HasSynchronizedSizes())
        {
            SCLOG(LoggingVerbosity::error,
                  "INVARIANT VIOLATION: FTerranUnitContainer vector sizes desynchronized at UnitCount=" +
                      std::to_string(ControlledUnits.size()));
        }
    }
};

}  // namespace sc2
