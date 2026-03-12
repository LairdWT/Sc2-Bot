#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

#include "common/terran_models.h"
#include "sc2api/sc2_typeenums.h"

namespace sc2 {

struct FCostData {
    uint16_t Minerals;
    uint16_t Vespine;
    uint8_t Supply;
    uint8_t BuildTime;

    FCostData() : Minerals(0), Vespine(0), Supply(0), BuildTime(0) {
    }
};

struct FUnitCostData {
    sc2::UNIT_TYPEID UnitType;
    FCostData CostData;

    FUnitCostData() : UnitType(sc2::UNIT_TYPEID::INVALID), CostData(FCostData()) {
    }
};

struct FBuildingCostData {
    sc2::UNIT_TYPEID BuildingType;
    FCostData CostData;

    FBuildingCostData() : BuildingType(sc2::UNIT_TYPEID::INVALID), CostData(FCostData()) {
    }
};

struct FUpgradeCostData {
    sc2::ABILITY_ID UpgradeType;
    FCostData CostData;

    FUpgradeCostData() : UpgradeType(sc2::ABILITY_ID::INVALID), CostData(FCostData()) {
    }
};

class FTerranEconomicUnitDataSoA {
public:
    FTerranEconomicUnitDataSoA()
        : UnitTypes{}, MineralCosts{}, VespineCosts{}, SupplyCosts{}, BuildTimes{}, CurrentUnitData()
    {
        UnitTypes.fill(sc2::UNIT_TYPEID::INVALID);
        MineralCosts.fill(0);
        VespineCosts.fill(0);
        SupplyCosts.fill(0);
        BuildTimes.fill(0);

        std::copy(std::begin(TERRAN_UNIT_TYPES), std::end(TERRAN_UNIT_TYPES), UnitTypes.begin());

        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_MARINE, 50, 0, 1, 18);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_MARAUDER, 100, 25, 2, 21);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_MEDIVAC, 100, 100, 2, 30);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_WIDOWMINE, 75, 25, 2, 21);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_WIDOWMINEBURROWED, 75, 25, 2, 21);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_AUTOTURRET, 0, 0, 0, 0);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_BANSHEE, 150, 100, 3, 43);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_BATTLECRUISER, 400, 300, 6, 64);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_CYCLONE, 150, 100, 3, 32);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_GHOST, 150, 125, 2, 29);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_HELLION, 100, 0, 2, 21);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_HELLIONTANK, 100, 0, 2, 21);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_LIBERATOR, 150, 150, 3, 43);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_LIBERATORAG, 150, 150, 3, 43);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_MULE, 0, 0, 0, 0);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_NUKE, 100, 100, 0, 43);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_RAVEN, 100, 150, 2, 43);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_REAPER, 50, 50, 1, 32);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_SCV, 50, 0, 1, 12);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_SIEGETANK, 150, 125, 3, 32);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_SIEGETANKSIEGED, 150, 125, 3, 32);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_THOR, 300, 200, 6, 43);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_THORAP, 300, 200, 6, 43);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_VIKINGASSAULT, 150, 75, 2, 30);
        SetUnitCostEntry(sc2::UNIT_TYPEID::TERRAN_VIKINGFIGHTER, 150, 75, 2, 30);

        ResetCurrentUnitCostData();
    }

    void SetCurrentUnitCostData(const sc2::UNIT_TYPEID UnitType) {
        const size_t UnitTypeIndex = GetTerranUnitTypeIndex(UnitType);
        if (!IsTerranUnitTypeIndexValid(UnitTypeIndex))
        {
            ResetCurrentUnitCostData();
            return;
        }

        CurrentUnitData.UnitType = UnitTypes[UnitTypeIndex];
        CurrentUnitData.CostData.Minerals = MineralCosts[UnitTypeIndex];
        CurrentUnitData.CostData.Vespine = VespineCosts[UnitTypeIndex];
        CurrentUnitData.CostData.Supply = SupplyCosts[UnitTypeIndex];
        CurrentUnitData.CostData.BuildTime = BuildTimes[UnitTypeIndex];
    }

    FUnitCostData& GetCurrentUnitCostData() {
        return CurrentUnitData;
    }

    uint16_t GetMineralCost(const sc2::UNIT_TYPEID UnitType) const {
        const size_t UnitTypeIndex = GetTerranUnitTypeIndex(UnitType);
        return IsTerranUnitTypeIndexValid(UnitTypeIndex) ? MineralCosts[UnitTypeIndex] : 0;
    }

    uint16_t GetVespineCost(const sc2::UNIT_TYPEID UnitType) const {
        const size_t UnitTypeIndex = GetTerranUnitTypeIndex(UnitType);
        return IsTerranUnitTypeIndexValid(UnitTypeIndex) ? VespineCosts[UnitTypeIndex] : 0;
    }

    uint8_t GetSupplyCost(const sc2::UNIT_TYPEID UnitType) const {
        const size_t UnitTypeIndex = GetTerranUnitTypeIndex(UnitType);
        return IsTerranUnitTypeIndexValid(UnitTypeIndex) ? SupplyCosts[UnitTypeIndex] : 0;
    }

    uint8_t GetBuildTime(const sc2::UNIT_TYPEID UnitType) const {
        const size_t UnitTypeIndex = GetTerranUnitTypeIndex(UnitType);
        return IsTerranUnitTypeIndexValid(UnitTypeIndex) ? BuildTimes[UnitTypeIndex] : 0;
    }

    FUnitCostData& GetUnitCostData(const sc2::UNIT_TYPEID UnitType) {
        SetCurrentUnitCostData(UnitType);
        return CurrentUnitData;
    }

private:
    void ResetCurrentUnitCostData()
    {
        CurrentUnitData = FUnitCostData();
    }

    void SetUnitCostEntry(const sc2::UNIT_TYPEID UnitType, const uint16_t MineralCostValue,
                          const uint16_t VespineCostValue, const uint8_t SupplyCostValue,
                          const uint8_t BuildTimeValue)
    {
        const size_t UnitTypeIndex = GetTerranUnitTypeIndex(UnitType);
        if (!IsTerranUnitTypeIndexValid(UnitTypeIndex))
        {
            return;
        }

        UnitTypes[UnitTypeIndex] = UnitType;
        MineralCosts[UnitTypeIndex] = MineralCostValue;
        VespineCosts[UnitTypeIndex] = VespineCostValue;
        SupplyCosts[UnitTypeIndex] = SupplyCostValue;
        BuildTimes[UnitTypeIndex] = BuildTimeValue;
    }

    std::array<sc2::UNIT_TYPEID, NUM_TERRAN_UNITS> UnitTypes;
    std::array<uint16_t, NUM_TERRAN_UNITS> MineralCosts;
    std::array<uint16_t, NUM_TERRAN_UNITS> VespineCosts;
    std::array<uint8_t, NUM_TERRAN_UNITS> SupplyCosts;
    std::array<uint8_t, NUM_TERRAN_UNITS> BuildTimes;
    FUnitCostData CurrentUnitData;
};

class FTerranBuildingDataSoA {
public:
    FTerranBuildingDataSoA()
        : BuildingTypes{}, MineralCosts{}, VespineCosts{}, BuildTimes{}, CurrentBuildingData()
    {
        BuildingTypes.fill(sc2::UNIT_TYPEID::INVALID);
        MineralCosts.fill(0);
        VespineCosts.fill(0);
        BuildTimes.fill(0);

        std::copy(std::begin(TERRAN_BUILDING_TYPES), std::end(TERRAN_BUILDING_TYPES), BuildingTypes.begin());

        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_ARMORY, 150, 100, 46);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_BARRACKS, 150, 0, 46);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_BARRACKSFLYING, 150, 0, 46);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_BARRACKSREACTOR, 200, 50, 82);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_BARRACKSTECHLAB, 200, 25, 71);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_BUNKER, 100, 0, 29);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER, 400, 0, 71);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_COMMANDCENTERFLYING, 400, 0, 71);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMAND, 550, 0, 96);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING, 550, 0, 96);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_PLANETARYFORTRESS, 550, 150, 86);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_ENGINEERINGBAY, 125, 0, 25);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_FACTORY, 150, 100, 43);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_FACTORYFLYING, 150, 100, 43);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_FACTORYREACTOR, 200, 150, 79);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_FACTORYTECHLAB, 200, 125, 68);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_FUSIONCORE, 150, 150, 46);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_GHOSTACADEMY, 150, 50, 29);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_MISSILETURRET, 100, 0, 18);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_REACTOR, 50, 50, 36);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_REFINERY, 75, 0, 21);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_REFINERYRICH, 75, 0, 21);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_SENSORTOWER, 125, 100, 18);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_STARPORT, 150, 100, 36);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_STARPORTFLYING, 150, 100, 36);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_STARPORTREACTOR, 200, 150, 72);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_STARPORTTECHLAB, 200, 125, 61);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 100, 0, 21);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED, 100, 0, 21);
        SetBuildingCostEntry(sc2::UNIT_TYPEID::TERRAN_TECHLAB, 50, 25, 18);

        ResetCurrentBuildingCostData();
    }

    void SetCurrentBuildingCostData(const sc2::UNIT_TYPEID BuildingType) {
        const size_t BuildingTypeIndex = GetTerranBuildingTypeIndex(BuildingType);
        if (!IsTerranBuildingTypeIndexValid(BuildingTypeIndex))
        {
            ResetCurrentBuildingCostData();
            return;
        }

        CurrentBuildingData.BuildingType = BuildingTypes[BuildingTypeIndex];
        CurrentBuildingData.CostData.Minerals = MineralCosts[BuildingTypeIndex];
        CurrentBuildingData.CostData.Vespine = VespineCosts[BuildingTypeIndex];
        CurrentBuildingData.CostData.BuildTime = BuildTimes[BuildingTypeIndex];
    }

    uint16_t GetMineralCost(const sc2::UNIT_TYPEID BuildingType) const {
        const size_t BuildingTypeIndex = GetTerranBuildingTypeIndex(BuildingType);
        return IsTerranBuildingTypeIndexValid(BuildingTypeIndex) ? MineralCosts[BuildingTypeIndex] : 0;
    }

    uint16_t GetVespineCost(const sc2::UNIT_TYPEID BuildingType) const {
        const size_t BuildingTypeIndex = GetTerranBuildingTypeIndex(BuildingType);
        return IsTerranBuildingTypeIndexValid(BuildingTypeIndex) ? VespineCosts[BuildingTypeIndex] : 0;
    }

    uint8_t GetBuildTime(const sc2::UNIT_TYPEID BuildingType) const {
        const size_t BuildingTypeIndex = GetTerranBuildingTypeIndex(BuildingType);
        return IsTerranBuildingTypeIndexValid(BuildingTypeIndex) ? BuildTimes[BuildingTypeIndex] : 0;
    }

    FBuildingCostData& GetCurrentBuildingCostData() {
        return CurrentBuildingData;
    }

    FBuildingCostData& GetBuildingCostData(const sc2::UNIT_TYPEID BuildingType) {
        SetCurrentBuildingCostData(BuildingType);
        return CurrentBuildingData;
    }

private:
    void ResetCurrentBuildingCostData()
    {
        CurrentBuildingData = FBuildingCostData();
    }

    void SetBuildingCostEntry(const sc2::UNIT_TYPEID BuildingType, const uint16_t MineralCostValue,
                              const uint16_t VespineCostValue, const uint8_t BuildTimeValue)
    {
        const size_t BuildingTypeIndex = GetTerranBuildingTypeIndex(BuildingType);
        if (!IsTerranBuildingTypeIndexValid(BuildingTypeIndex))
        {
            return;
        }

        BuildingTypes[BuildingTypeIndex] = BuildingType;
        MineralCosts[BuildingTypeIndex] = MineralCostValue;
        VespineCosts[BuildingTypeIndex] = VespineCostValue;
        BuildTimes[BuildingTypeIndex] = BuildTimeValue;
    }

    std::array<sc2::UNIT_TYPEID, NUM_TERRAN_BUILDINGS> BuildingTypes;
    std::array<uint16_t, NUM_TERRAN_BUILDINGS> MineralCosts;
    std::array<uint16_t, NUM_TERRAN_BUILDINGS> VespineCosts;
    std::array<uint8_t, NUM_TERRAN_BUILDINGS> BuildTimes;
    FBuildingCostData CurrentBuildingData;
};

class FTerranUpgradeResearchDataSoA {
public:
    FTerranUpgradeResearchDataSoA()
        : UpgradeTypes{}, MineralCosts{}, VespineCosts{}, ResearchTimes{}, CurrentUpgradeData()
    {
        UpgradeTypes.fill(sc2::ABILITY_ID::INVALID);
        MineralCosts.fill(0);
        VespineCosts.fill(0);
        ResearchTimes.fill(0);

        std::copy(std::begin(TERRAN_RESEARCH_UPGRADE_TYPES), std::end(TERRAN_RESEARCH_UPGRADE_TYPES), UpgradeTypes.begin());

        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_STIMPACK, 100, 100, 121);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_COMBATSHIELD, 50, 50, 79);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_CONCUSSIVESHELLS, 50, 50, 57);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_INFERNALPREIGNITER, 150, 150, 100);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_DRILLINGCLAWS, 150, 150, 57);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_SMARTSERVOS, 100, 100, 57);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_CYCLONERAPIDFIRELAUNCHERS, 150, 150, 79);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_LIBERATORAGMODE, 150, 150, 121);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_RAVENCORVIDREACTOR, 150, 150, 79);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_HISECAUTOTRACKING, 75, 75, 43);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_TERRANSTRUCTUREARMORUPGRADE, 150, 150, 57);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONSLEVEL1, 100, 0, 114);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONSLEVEL2, 175, 100, 114);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONSLEVEL3, 250, 150, 114);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_TERRANINFANTRYARMORLEVEL1, 100, 0, 114);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_TERRANINFANTRYARMORLEVEL2, 175, 100, 114);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_TERRANINFANTRYARMORLEVEL3, 250, 150, 114);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_TERRANVEHICLEWEAPONSLEVEL1, 100, 100, 114);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_TERRANVEHICLEWEAPONSLEVEL2, 175, 175, 114);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_TERRANVEHICLEWEAPONSLEVEL3, 250, 250, 114);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_TERRANSHIPWEAPONSLEVEL1, 100, 100, 114);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_TERRANSHIPWEAPONSLEVEL2, 175, 175, 114);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_TERRANSHIPWEAPONSLEVEL3, 250, 250, 114);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_TERRANVEHICLEANDSHIPPLATINGLEVEL1, 150, 150, 114);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_TERRANVEHICLEANDSHIPPLATINGLEVEL2, 225, 225, 114);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_TERRANVEHICLEANDSHIPPLATINGLEVEL3, 300, 300, 114);
        SetUpgradeResearchEntry(sc2::ABILITY_ID::RESEARCH_BATTLECRUISERWEAPONREFIT, 150, 150, 79);

        ResetCurrentUpgradeCostData();
    }

    uint16_t GetMineralCost(const sc2::ABILITY_ID UpgradeType) const {
        const size_t UpgradeTypeIndex = GetTerranUpgradeTypeIndex(UpgradeType);
        return IsTerranUpgradeTypeIndexValid(UpgradeTypeIndex) ? MineralCosts[UpgradeTypeIndex] : 0;
    }

    uint16_t GetVespineCost(const sc2::ABILITY_ID UpgradeType) const {
        const size_t UpgradeTypeIndex = GetTerranUpgradeTypeIndex(UpgradeType);
        return IsTerranUpgradeTypeIndexValid(UpgradeTypeIndex) ? VespineCosts[UpgradeTypeIndex] : 0;
    }

    uint8_t GetResearchTime(const sc2::ABILITY_ID UpgradeType) const {
        const size_t UpgradeTypeIndex = GetTerranUpgradeTypeIndex(UpgradeType);
        return IsTerranUpgradeTypeIndexValid(UpgradeTypeIndex) ? ResearchTimes[UpgradeTypeIndex] : 0;
    }

    FUpgradeCostData& GetCurrentUpgradeCostData() {
        return CurrentUpgradeData;
    }

    void SetCurrentUpgradeCostData(const sc2::ABILITY_ID UpgradeType) {
        const size_t UpgradeTypeIndex = GetTerranUpgradeTypeIndex(UpgradeType);
        if (!IsTerranUpgradeTypeIndexValid(UpgradeTypeIndex))
        {
            ResetCurrentUpgradeCostData();
            return;
        }

        CurrentUpgradeData.UpgradeType = UpgradeTypes[UpgradeTypeIndex];
        CurrentUpgradeData.CostData.Minerals = MineralCosts[UpgradeTypeIndex];
        CurrentUpgradeData.CostData.Vespine = VespineCosts[UpgradeTypeIndex];
        CurrentUpgradeData.CostData.BuildTime = ResearchTimes[UpgradeTypeIndex];
    }

    FUpgradeCostData& GetUpgradeCostData(const sc2::ABILITY_ID UpgradeType) {
        SetCurrentUpgradeCostData(UpgradeType);
        return CurrentUpgradeData;
    }

private:
    void ResetCurrentUpgradeCostData()
    {
        CurrentUpgradeData = FUpgradeCostData();
    }

    void SetUpgradeResearchEntry(const sc2::ABILITY_ID UpgradeType, const uint16_t MineralCostValue,
                                 const uint16_t VespineCostValue, const uint8_t ResearchTimeValue)
    {
        const size_t UpgradeTypeIndex = GetTerranUpgradeTypeIndex(UpgradeType);
        if (!IsTerranUpgradeTypeIndexValid(UpgradeTypeIndex))
        {
            return;
        }

        UpgradeTypes[UpgradeTypeIndex] = UpgradeType;
        MineralCosts[UpgradeTypeIndex] = MineralCostValue;
        VespineCosts[UpgradeTypeIndex] = VespineCostValue;
        ResearchTimes[UpgradeTypeIndex] = ResearchTimeValue;
    }

    std::array<sc2::ABILITY_ID, NUM_TERRAN_UPGRADES> UpgradeTypes;
    std::array<uint16_t, NUM_TERRAN_UPGRADES> MineralCosts;
    std::array<uint16_t, NUM_TERRAN_UPGRADES> VespineCosts;
    std::array<uint8_t, NUM_TERRAN_UPGRADES> ResearchTimes;
    FUpgradeCostData CurrentUpgradeData;
};

class FTerranEconomicDataSoA {
public:
    FTerranEconomicDataSoA() : UnitData(), BuildingData(), UpgradeData()
    {
    }

    size_t GetUnitTypeIndex(const sc2::UNIT_TYPEID UnitType) const {
        return GetTerranUnitTypeIndex(UnitType);
    }

    size_t GetBuildingTypeIndex(const sc2::UNIT_TYPEID BuildingType) const {
        return GetTerranBuildingTypeIndex(BuildingType);
    }

    size_t GetUpgradeTypeIndex(const sc2::ABILITY_ID UpgradeType) const {
        return GetTerranUpgradeTypeIndex(UpgradeType);
    }

    void SetCurrentUnitData(const sc2::UNIT_TYPEID UnitType) {
        UnitData.SetCurrentUnitCostData(UnitType);
    }

    void SetCurrentBuildingData(const sc2::UNIT_TYPEID BuildingType) {
        BuildingData.SetCurrentBuildingCostData(BuildingType);
    }

    void SetCurrentUpgradeData(const sc2::ABILITY_ID UpgradeType) {
        UpgradeData.SetCurrentUpgradeCostData(UpgradeType);
    }

    FUnitCostData& GetUnitCostData(const sc2::UNIT_TYPEID UnitType) {
        UnitData.SetCurrentUnitCostData(UnitType);
        return UnitData.GetCurrentUnitCostData();
    }

    FBuildingCostData& GetBuildingCostData(const sc2::UNIT_TYPEID BuildingType) {
        BuildingData.SetCurrentBuildingCostData(BuildingType);
        return BuildingData.GetCurrentBuildingCostData();
    }

    FUpgradeCostData& GetUpgradeCostData(const sc2::ABILITY_ID UpgradeType) {
        UpgradeData.SetCurrentUpgradeCostData(UpgradeType);
        return UpgradeData.GetCurrentUpgradeCostData();
    }

private:
    FTerranEconomicUnitDataSoA UnitData;
    FTerranBuildingDataSoA BuildingData;
    FTerranUpgradeResearchDataSoA UpgradeData;
};

inline FTerranEconomicDataSoA TERRAN_ECONOMIC_DATA = FTerranEconomicDataSoA();

}  // namespace sc2
