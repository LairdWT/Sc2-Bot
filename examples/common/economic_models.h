#pragma once

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

// Using structure of arrays (SoA) for unit data
struct FTerranEconomicUnitDataSoA {
    sc2::UNIT_TYPEID UnitTypes[17];
    uint16_t MineralCosts[17];
    uint16_t VespineCosts[17];
    uint8_t SupplyCosts[17];
    uint8_t BuildTimes[17];
    FUnitCostData CurrentUnitData;

    FTerranEconomicUnitDataSoA() {
        std::copy(std::begin(TERRAN_UNIT_TYPES), std::end(TERRAN_UNIT_TYPES), UnitTypes);
        uint16_t mineralCosts[17] = {50, 100, 50, 150, 100, 75, 150, 150, 300, 150, 100, 150, 100, 400, 150, 50, 0};
        std::copy(std::begin(mineralCosts), std::end(mineralCosts), MineralCosts);
        uint16_t vespineCosts[17] = {0, 25, 0, 125, 0, 25, 100, 125, 200, 75, 0, 100, 200, 300, 150, 0, 0};
        std::copy(std::begin(vespineCosts), std::end(vespineCosts), VespineCosts);
        uint8_t supplyCosts[17] = {1, 2, 1, 3, 2, 2, 3, 3, 6, 2, 2, 3, 2, 6, 3, 1, 0};
        std::copy(std::begin(supplyCosts), std::end(supplyCosts), SupplyCosts);
        uint8_t buildTimes[17] = {25, 30, 30, 40, 21, 29, 32, 32, 60, 43, 30, 43, 43, 64, 43, 12, 0};
        std::copy(std::begin(buildTimes), std::end(buildTimes), BuildTimes);
    }

    void SetCurrentUnitCostData(const sc2::UNIT_TYPEID UnitType) {
        const uint8_t UnitIndex = GetTerranUnitTypeIndex(UnitType);
        CurrentUnitData.UnitType = UnitTypes[UnitIndex];
        CurrentUnitData.CostData.Minerals = MineralCosts[UnitIndex];
        CurrentUnitData.CostData.Vespine = VespineCosts[UnitIndex];
        CurrentUnitData.CostData.Supply = SupplyCosts[UnitIndex];
        CurrentUnitData.CostData.BuildTime = BuildTimes[UnitIndex];
    };

    FUnitCostData& GetCurrentUnitCostData() {
        return CurrentUnitData;
    }

    uint16_t GetMineralCost(const sc2::UNIT_TYPEID UnitType) const {
        return MineralCosts[GetTerranUnitTypeIndex(UnitType)];
    }

    uint16_t GetVespineCost(const sc2::UNIT_TYPEID UnitType) const {
        return VespineCosts[GetTerranUnitTypeIndex(UnitType)];
    }

    uint8_t GetSupplyCost(const sc2::UNIT_TYPEID UnitType) const {
        return SupplyCosts[GetTerranUnitTypeIndex(UnitType)];
    }

    uint8_t GetBuildTime(const sc2::UNIT_TYPEID UnitType) const {
        return BuildTimes[GetTerranUnitTypeIndex(UnitType)];
    }

    FUnitCostData& GetUnitCostData(const sc2::UNIT_TYPEID UnitType) {
        SetCurrentUnitCostData(UnitType);
        return CurrentUnitData;
    }
};

// Structure of Arrays (SoA) for Terran Building Data
struct FTerranBuildingDataSoA {
    sc2::UNIT_TYPEID BuildingTypes[21];
    uint16_t MineralCosts[21];
    uint16_t VespineCosts[21];
    uint8_t BuildTimes[21];
    FBuildingCostData CurrentBuildingData;

    FTerranBuildingDataSoA() {
        std::copy(std::begin(TERRAN_BUILDING_TYPES), std::end(TERRAN_BUILDING_TYPES), BuildingTypes);

        uint16_t mineralCosts[21] = {
            400, 550, 550, 100, 150, 50, 50, 150, 50, 50,
            150, 50, 50, 125, 150, 150, 100, 125, 150, 75, 75
        };
        std::copy(std::begin(mineralCosts), std::end(mineralCosts), MineralCosts);

        uint16_t vespineCosts[21] = {
            0, 0, 150, 0, 0, 50, 25, 100, 50, 25,
            100, 50, 25, 0, 100, 150, 0, 100, 50, 0, 0
        };
        std::copy(std::begin(vespineCosts), std::end(vespineCosts), VespineCosts);

        uint8_t buildTimes[21] = {
            71, 25, 50, 21, 46, 36, 29, 43, 36, 29,
            36, 36, 29, 25, 46, 46, 18, 25, 29, 21, 21
        };
        std::copy(std::begin(buildTimes), std::end(buildTimes), BuildTimes);
    }

    void SetCurrentBuildingCostData(const sc2::UNIT_TYPEID BuildingType) {
        const uint8_t BuildingIndex = GetTerranBuildingTypeIndex(BuildingType);
        CurrentBuildingData.BuildingType = BuildingTypes[BuildingIndex];
        CurrentBuildingData.CostData.Minerals = MineralCosts[BuildingIndex];
        CurrentBuildingData.CostData.Vespine = VespineCosts[BuildingIndex];
        CurrentBuildingData.CostData.BuildTime = BuildTimes[BuildingIndex];
    }

    uint16_t GetMineralCost(const sc2::UNIT_TYPEID BuildingType) const {
        return MineralCosts[GetTerranBuildingTypeIndex(BuildingType)];
    }

    uint16_t GetVespineCost(const sc2::UNIT_TYPEID BuildingType) const {
        return VespineCosts[GetTerranBuildingTypeIndex(BuildingType)];
    }

    uint8_t GetBuildTime(const sc2::UNIT_TYPEID BuildingType) const {
        return BuildTimes[GetTerranBuildingTypeIndex(BuildingType)];
    }

    FBuildingCostData& GetCurrentBuildingCostData() {
        return CurrentBuildingData;
    }

    FBuildingCostData& GetBuildingCostData(const sc2::UNIT_TYPEID BuildingType) {
        SetCurrentBuildingCostData(BuildingType);
        return CurrentBuildingData;
    }

};


struct FTerranUpgradeResearchDataSoA {
    sc2::ABILITY_ID UpgradeTypes[26];
    uint16_t MineralCosts[26];
    uint16_t VespineCosts[26];
    uint8_t ResearchTimes[26];
    FUpgradeCostData CurrentUpgradeData;

    FTerranUpgradeResearchDataSoA() {
        std::copy(std::begin(TERRAN_RESEARCH_UPGRADE_TYPES), std::end(TERRAN_RESEARCH_UPGRADE_TYPES), UpgradeTypes);

        uint16_t mineralCosts[26] = {
            100, 50, 50, 150, 150, 100, 150, 150, 150, 75,
            150, 100, 175, 250, 100, 175, 250, 100, 175, 250,
            100, 175, 250, 150, 225, 300
        };
        std::copy(std::begin(mineralCosts), std::end(mineralCosts), MineralCosts);

        uint16_t vespineCosts[26] = {
            100, 50, 50, 150, 150, 100, 150, 150, 150, 75,
            150, 0, 100, 150, 0, 100, 150, 100, 175, 250,
            100, 175, 250, 150, 225, 300
        };
        std::copy(std::begin(vespineCosts), std::end(vespineCosts), VespineCosts);

        uint8_t researchTimes[26] = {
            121, 79, 57, 100, 57, 57, 79, 121, 79, 43,
            57, 114, 114, 114, 114, 114, 114, 114, 114, 114,
            114, 114, 114, 114, 114, 114
        };
        std::copy(std::begin(researchTimes), std::end(researchTimes), ResearchTimes);
    }

    uint16_t GetMineralCost(const sc2::ABILITY_ID UpgradeType) const {
        return MineralCosts[GetTerranUpgradeTypeIndex(UpgradeType)];
    }

    uint16_t GetVespineCost(const sc2::ABILITY_ID UpgradeType) const {
        return VespineCosts[GetTerranUpgradeTypeIndex(UpgradeType)];
    }

    uint8_t GetResearchTime(const sc2::ABILITY_ID UpgradeType) const {
        return ResearchTimes[GetTerranUpgradeTypeIndex(UpgradeType)];
    }

    FUpgradeCostData& GetCurrentUpgradeCostData() {
        return CurrentUpgradeData;
    }

    void SetCurrentUpgradeCostData(const sc2::ABILITY_ID UpgradeType) {
        const uint8_t UpgradeIndex = GetTerranUpgradeTypeIndex(UpgradeType);
        CurrentUpgradeData.UpgradeType = UpgradeTypes[UpgradeIndex];
        CurrentUpgradeData.CostData.Minerals = MineralCosts[UpgradeIndex];
        CurrentUpgradeData.CostData.Vespine = VespineCosts[UpgradeIndex];
        CurrentUpgradeData.CostData.BuildTime = ResearchTimes[UpgradeIndex];
    }

    FUpgradeCostData& GetUpgradeCostData(const sc2::ABILITY_ID UpgradeType) {
        SetCurrentUpgradeCostData(UpgradeType);
        return CurrentUpgradeData;
    }
};

struct FTerranEconomicDataSoA {
    FTerranEconomicUnitDataSoA UnitData;
    FTerranBuildingDataSoA BuildingData;
    FTerranUpgradeResearchDataSoA UpgradeData;

    FTerranEconomicDataSoA() : UnitData(), BuildingData(), UpgradeData()
    {}

    uint8_t GetUnitTypeIndex(const sc2::UNIT_TYPEID UnitType) const {
        return GetTerranUnitTypeIndex(UnitType);
    }

    uint8_t GetBuildingTypeIndex(const sc2::UNIT_TYPEID BuildingType) const {
        return GetTerranBuildingTypeIndex(BuildingType);
    }

    uint8_t GetUpgradeTypeIndex(const sc2::ABILITY_ID UpgradeType) const {
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

};

}  // namespace sc2
