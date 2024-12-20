#pragma once

#include <cstdint>
#include <string>
#include <iostream>

#include "sc2api/sc2_api.h"

#include "common/economic_models.h"
#include "common/terran_models.h"
#include "common/terran_unit_container.h"

namespace sc2 {

/* Represent the current game progression state
 *  @Opening: The opening phase of the game where the bot is setting up its first buildings
 *  @EarlyGame: The early game phase of the game, usually from 30 supply to 70 supply (<5min)
 *  @MidGame: The mid game phase of the game, usually from 70 supply to 140 supply (5min)
 *  @LateGame: The late game phase of the game, usually from 140 supply and up (10min)
 *  @EndGame: The end game phase of the game, usually when bases begin to run out
 */
enum class EGameStateProgression : uint8_t { Opening, EarlyGame, MidGame, LateGame, EndGame };


//~ Agent Strength Assessment ~//

// Represents the perceived agent threat level
enum class EThreatLevel : uint8_t { Low, Medium, High, Maximum };

// Represents the perceived economic strength of the bot
enum class EEconomicStrength : uint8_t { Dire, Weak, Good, Strong, Maximum };

// Represents the perceived military strength of the bot
enum class EMilitaryStrength : uint8_t { Dire, Weak, Good, Strong, Maximum };

// Represents the perceived tech level of the bot
enum class ETechLevel : uint8_t { None, Low, Medium, High, Maximum };

// Represents the perceived map control of the bot
enum class EMapControl : uint8_t { None, Low, Medium, High, Maximum };

// Represents the perceived map vision of the bot
enum class EMapVision : uint8_t { None, Low, Medium, High, Maximum };

struct FAgentAssessment {

    EThreatLevel ThreatLevel;
    EEconomicStrength EconomicStrength;
    EMilitaryStrength MilitaryStrength;
    ETechLevel TechLevel;
    EMapControl MapControl;
    EMapVision MapVision;

    FAgentAssessment()
        : ThreatLevel(EThreatLevel::High),
          EconomicStrength(EEconomicStrength::Weak),
          MilitaryStrength(EMilitaryStrength::Dire),
          TechLevel(ETechLevel::None),
          MapControl(EMapControl::None),
          MapVision(EMapVision::None) {
    }

    FAgentAssessment(const EThreatLevel InThreatLevel,
                     const EEconomicStrength InEconomicStrength,
                     const EMilitaryStrength InMilitaryStrength,
                     const ETechLevel InTechLevel,
                     const EMapControl InMapControl,
                     const EMapVision InMapVision)
        : ThreatLevel(InThreatLevel),
          EconomicStrength(InEconomicStrength),
          MilitaryStrength(InMilitaryStrength),
          TechLevel(InTechLevel),
          MapControl(InMapControl),
          MapVision(InMapVision) {
    }

    // Utility functions to return each assessment enum value as a string
    std::string GetThreatLevelAsString() const {
        switch (ThreatLevel) {
            case EThreatLevel::Low:
                return "Low";
            case EThreatLevel::Medium:
                return "Medium";
            case EThreatLevel::High:
                return "High";
            case EThreatLevel::Maximum:
                return "Maximum";
            default:
                return "Unknown";
        }
    }

    std::string GetEconomicStrengthAsString() const {
        switch (EconomicStrength) {
            case EEconomicStrength::Dire:
                return "Dire";
            case EEconomicStrength::Weak:
                return "Weak";
            case EEconomicStrength::Good:
                return "Good";
            case EEconomicStrength::Strong:
                return "Strong";
            case EEconomicStrength::Maximum:
                return "Maximum";
            default:
                return "Unknown";
        }
    }

    std::string GetMilitaryStrengthAsString() const {
        switch (MilitaryStrength) {
            case EMilitaryStrength::Dire:
                return "Dire";
            case EMilitaryStrength::Weak:
                return "Weak";
            case EMilitaryStrength::Good:
                return "Good";
            case EMilitaryStrength::Strong:
                return "Strong";
            case EMilitaryStrength::Maximum:
                return "Maximum";
            default:
                return "Unknown";
        }
    }

    std::string GetTechLevelAsString() const {
        switch (TechLevel) {
            case ETechLevel::None:
                return "None";
            case ETechLevel::Low:
                return "Low";
            case ETechLevel::Medium:
                return "Medium";
            case ETechLevel::High:
                return "High";
            case ETechLevel::Maximum:
                return "Maximum";
            default:
                return "Unknown";
        }
    }

    std::string GetMapControlAsString() const {
        switch (MapControl) {
            case EMapControl::None:
                return "None";
            case EMapControl::Low:
                return "Low";
            case EMapControl::Medium:
                return "Medium";
            case EMapControl::High:
                return "High";
            case EMapControl::Maximum:
                return "Maximum";
            default:
                return "Unknown";
        }
    }

    std::string GetMapVisionAsString() const {
        switch (MapVision) {
            case EMapVision::None:
                return "None";
            case EMapVision::Low:
                return "Low";
            case EMapVision::Medium:
                return "Medium";
            case EMapVision::High:
                return "High";
            case EMapVision::Maximum:
                return "Maximum";
            default:
                return "Unknown";
        }
    }
};

// Manages the assement of the bot and it's opponent
struct FAgentAssessments {
    FAgentAssessment Self;
    FAgentAssessment Opponent;

    FAgentAssessments() : Self(FAgentAssessment()), Opponent(FAgentAssessment()) {
    }

    FAgentAssessments(const FAgentAssessment InBotAssessment, const FAgentAssessment InOpponentAssessment)
        : Self(InBotAssessment), Opponent(InOpponentAssessment) {
    }
};


//~ Agent Resources ~//

struct FAgentEconomy {
    uint32_t Minerals;
    uint32_t Vespene;
    uint8_t Supply;
    uint8_t SupplyCap;
    uint8_t SupplyAvailable;

    FAgentEconomy()
        : Minerals(0),
          Vespene(0),
          Supply(0),
          SupplyCap(0),
          SupplyAvailable(0) {
    }

    FAgentEconomy(const uint32_t InMinerals, const uint32_t InVespene, const uint8_t InSupply,
                           const uint8_t InSupplyCap, const uint8_t InSupplyAvailable)
        : Minerals(InMinerals),
          Vespene(InVespene),
          Supply(InSupply),
          SupplyCap(InSupplyCap),
          SupplyAvailable(InSupplyAvailable) {
    }
};

struct FAgentUnits {
    uint32_t ArmyCount;
    uint32_t ArmyValueMinerals;
    uint32_t ArmyValueVespene;
    uint32_t ArmySupply;

    std::array<uint16_t, NUM_TERRAN_UNITS> UnitCounts;
    std::array<uint16_t, NUM_TERRAN_UNITS> UnitsInConstruction;

    FAgentUnits()
        : ArmyCount(0), ArmyValueMinerals(0), ArmyValueVespene(0), ArmySupply(0), UnitCounts{}, UnitsInConstruction{} {
    }

    void Update() {
        UpdateArmyUnitCounts();
        UpdateArmyUnitValues();
    }

    void UpdateArmyUnitCounts() {
        ArmyCount = 0;
        for (const sc2::UNIT_TYPEID UnitType : TERRAN_UNIT_TYPES) {
            switch (UnitType) {
                // Ignore SCVs and MULEs for army calculation
                case sc2::UNIT_TYPEID::TERRAN_SCV:
                    continue;
                case sc2::UNIT_TYPEID::TERRAN_MULE:
                    continue;
                default:
                    ArmyCount += GetUnitCount(UnitType);
                    break;
            }
        }
    }

    void UpdateArmyUnitValues() {
        ArmyValueMinerals = 0;
        ArmyValueVespene = 0;
        ArmySupply = 0;
        for (const sc2::UNIT_TYPEID UnitType : TERRAN_UNIT_TYPES) {
            switch (UnitType) {
                // Ignore SCVs and MULEs for army value calculation
                case sc2::UNIT_TYPEID::TERRAN_SCV:
                    continue;
                case sc2::UNIT_TYPEID::TERRAN_MULE:
                    continue;
                default:
                    ArmyValueMinerals +=
                        GetUnitCount(UnitType) * TERRAN_ECONOMIC_DATA.GetUnitCostData(UnitType).CostData.Minerals;
                    ArmyValueVespene +=
                        GetUnitCount(UnitType) * TERRAN_ECONOMIC_DATA.GetUnitCostData(UnitType).CostData.Vespine;
                    ArmySupply +=
                        GetUnitCount(UnitType) * TERRAN_ECONOMIC_DATA.GetUnitCostData(UnitType).CostData.Supply;
                    break;
            }
        }
    }

    uint16_t GetArmyCount() const {
        return ArmyCount;
    }

    uint16_t GetUnitCount(const sc2::UNIT_TYPEID UnitType) const {
        return UnitCounts[GetTerranUnitTypeIndex(UnitType)];
    }

    void SetUnitCount(const sc2::UNIT_TYPEID UnitType, const uint16_t Count) {
        UnitCounts[GetTerranUnitTypeIndex(UnitType)] = Count;
    }

    void IncrementUnitCount(const sc2::UNIT_TYPEID UnitType) {
        UnitCounts[GetTerranUnitTypeIndex(UnitType)]++;
    }

    void DecrementUnitCount(const sc2::UNIT_TYPEID UnitType) {
        const size_t Index = GetTerranUnitTypeIndex(UnitType);
        if (UnitCounts[Index] > 1) {
            UnitCounts[Index]--;
            return;
        }
        UnitCounts[Index] = 0;
    }

    void SetUnitsInConstruction(const sc2::UNIT_TYPEID UnitType, const uint16_t Count) {
        UnitsInConstruction[GetTerranUnitTypeIndex(UnitType)] = Count;
    }

    void IncrementUnitsInConstruction(const sc2::UNIT_TYPEID UnitType) {
        UnitsInConstruction[GetTerranUnitTypeIndex(UnitType)]++;
    }

    uint16_t GetUnitsInConstruction(const sc2::UNIT_TYPEID UnitType) const {
        return UnitsInConstruction[GetTerranUnitTypeIndex(UnitType)];
    }

    uint16_t GetWorkerCount() const {
        return GetUnitCount(sc2::UNIT_TYPEID::TERRAN_SCV);
    }
};

struct FAgentBuildings {

    std::array<uint16_t, NUM_TERRAN_BUILDINGS> BuildingCounts;

    std::array<uint16_t, NUM_TERRAN_BUILDINGS> CurrentlyInConstruction;

    FAgentBuildings() : BuildingCounts{}, CurrentlyInConstruction{} {
        for (uint16_t& Count : BuildingCounts) {
            Count = 0;
        }

        for (uint16_t& Count : CurrentlyInConstruction) {
            Count = 0;
        }
    }

    uint16_t GetBuildingCount(const sc2::UNIT_TYPEID BuildingType) const {
        return BuildingCounts[GetTerranBuildingTypeIndex(BuildingType)];
    }

    void SetBuildingCount(const sc2::UNIT_TYPEID BuildingType, const uint16_t Count) {
        BuildingCounts[GetTerranBuildingTypeIndex(BuildingType)] = Count;
    }

    void IncrementBuildingCount(const sc2::UNIT_TYPEID BuildingType) {
        BuildingCounts[GetTerranBuildingTypeIndex(BuildingType)]++;
    }

    void DecrementBuildingCount(const sc2::UNIT_TYPEID BuildingType) {
        const size_t Index = GetTerranBuildingTypeIndex(BuildingType);
        if (BuildingCounts[Index] < 1) {
            BuildingCounts[Index] = 0;
            return;
        }
        BuildingCounts[Index]--;
    }

    uint16_t GetCurrentlyInConstruction(const sc2::UNIT_TYPEID BuildingType) const {
        return CurrentlyInConstruction[GetTerranBuildingTypeIndex(BuildingType)];
    }

    void SetCurrentlyInConstruction(const sc2::UNIT_TYPEID BuildingType, const uint16_t Count) {
        CurrentlyInConstruction[GetTerranBuildingTypeIndex(BuildingType)] = Count;
    }

    void IncrementCurrentlyInConstruction(const sc2::UNIT_TYPEID BuildingType) {
        CurrentlyInConstruction[GetTerranBuildingTypeIndex(BuildingType)]++;
    }

    uint16_t GetTownHallCount() const {
        return BuildingCounts[GetTerranBuildingTypeIndex(sc2::UNIT_TYPEID::TERRAN_COMMANDCENTER)] +
               BuildingCounts[GetTerranBuildingTypeIndex(sc2::UNIT_TYPEID::TERRAN_COMMANDCENTERFLYING)] +
               BuildingCounts[GetTerranBuildingTypeIndex(sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMAND)] +
               BuildingCounts[GetTerranBuildingTypeIndex(sc2::UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING)] +
               BuildingCounts[GetTerranBuildingTypeIndex(sc2::UNIT_TYPEID::TERRAN_PLANETARYFORTRESS)];
    }

    uint16_t GetSupplyDepotCount() const {
        return BuildingCounts[GetTerranBuildingTypeIndex(sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOT)] +
               BuildingCounts[GetTerranBuildingTypeIndex(sc2::UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED)];
    }

    uint16_t GetBarracksCount() const {
        return BuildingCounts[GetTerranBuildingTypeIndex(sc2::UNIT_TYPEID::TERRAN_BARRACKS)] +
               BuildingCounts[GetTerranBuildingTypeIndex(sc2::UNIT_TYPEID::TERRAN_BARRACKSFLYING)] +
               BuildingCounts[GetTerranBuildingTypeIndex(sc2::UNIT_TYPEID::TERRAN_BARRACKSREACTOR)] +
               BuildingCounts[GetTerranBuildingTypeIndex(sc2::UNIT_TYPEID::TERRAN_BARRACKSTECHLAB)];
    }

    uint16_t GetFactoryCount() const {
        return BuildingCounts[GetTerranBuildingTypeIndex(sc2::UNIT_TYPEID::TERRAN_FACTORY)] +
               BuildingCounts[GetTerranBuildingTypeIndex(sc2::UNIT_TYPEID::TERRAN_FACTORYFLYING)] +
               BuildingCounts[GetTerranBuildingTypeIndex(sc2::UNIT_TYPEID::TERRAN_FACTORYREACTOR)] +
               BuildingCounts[GetTerranBuildingTypeIndex(sc2::UNIT_TYPEID::TERRAN_FACTORYTECHLAB)];
    }

    uint16_t GetStarportCount() const {
        return BuildingCounts[GetTerranBuildingTypeIndex(sc2::UNIT_TYPEID::TERRAN_STARPORT)] +
               BuildingCounts[GetTerranBuildingTypeIndex(sc2::UNIT_TYPEID::TERRAN_STARPORTFLYING)] +
               BuildingCounts[GetTerranBuildingTypeIndex(sc2::UNIT_TYPEID::TERRAN_STARPORTREACTOR)] +
               BuildingCounts[GetTerranBuildingTypeIndex(sc2::UNIT_TYPEID::TERRAN_STARPORTTECHLAB)];
    }
};

//~ Agent Strategy ~//

// Represents the Macro level focus of the bot
enum class EStrategicFocus : uint8_t { Economic, Defensive, Balanced, Aggressive, AllIn };

// Represents the current production focus
enum class EProductionStrategy : uint8_t { Workers, Expansion, ArmyBuildings, ArmyUnits, ArmyUpgrades, TechUp };

// Represents the current military goals
enum class EMilitaryStrategy : uint8_t { MapControl, Harass, Defend, AttackNatural, AttackMain, AllIn };

struct FAgentStrategy {
    EStrategicFocus StrategicFocus;
    EProductionStrategy ProductionStrategy;
    EMilitaryStrategy MilitaryStrategy;

    FAgentStrategy()
        : StrategicFocus(EStrategicFocus::Balanced),
          ProductionStrategy(EProductionStrategy::Workers),
          MilitaryStrategy(EMilitaryStrategy::MapControl) {
    }

    FAgentStrategy(const EStrategicFocus InStrategicFocus, const EProductionStrategy InProductionStrategy,
                   const EMilitaryStrategy InMilitaryStrategy)
        : StrategicFocus(InStrategicFocus),
          ProductionStrategy(InProductionStrategy),
          MilitaryStrategy(InMilitaryStrategy) {
    }

    // Utility functions to return each strategy enum value as a string
    std::string GetStrategicFocusAsString() const {
        switch (StrategicFocus) {
            case EStrategicFocus::Economic:
                return "Economic";
            case EStrategicFocus::Defensive:
                return "Defensive";
            case EStrategicFocus::Balanced:
                return "Balanced";
            case EStrategicFocus::Aggressive:
                return "Aggressive";
            case EStrategicFocus::AllIn:
                return "AllIn";
            default:
                return "Unknown";
        }
    }

    std::string GetProductionStrategyAsString() const {
        switch (ProductionStrategy) {
            case EProductionStrategy::Workers:
                return "Workers";
            case EProductionStrategy::Expansion:
                return "Expansion";
            case EProductionStrategy::ArmyBuildings:
                return "ArmyBuildings";
            case EProductionStrategy::ArmyUnits:
                return "ArmyUnits";
            case EProductionStrategy::ArmyUpgrades:
                return "ArmyUpgrades";
            case EProductionStrategy::TechUp:
                return "TechUp";
            default:
                return "Unknown";
        }
    }

    std::string GetMilitaryStrategyAsString() const {
        switch (MilitaryStrategy) {
            case EMilitaryStrategy::MapControl:
                return "MapControl";
            case EMilitaryStrategy::Harass:
                return "Harass";
            case EMilitaryStrategy::Defend:
                return "Defend";
            case EMilitaryStrategy::AttackNatural:
                return "AttackNatural";
            case EMilitaryStrategy::AttackMain:
                return "AttackMain";
            case EMilitaryStrategy::AllIn:
                return "AllIn";
            default:
                return "Unknown";
        }
    }
};

//~ Agent State Management ~//

struct FAgentState {
    EGameStateProgression GameStateProgression;
    FAgentAssessments Assessments;
    FAgentStrategy Strategy;

    FTerranUnitContainer UnitContainer;

    FAgentEconomy Economy;
    FAgentUnits Units;
    FAgentBuildings Buildings;

    FAgentState()
        : GameStateProgression(EGameStateProgression::Opening),
          Assessments(FAgentAssessments()),
          Strategy(FAgentStrategy()),
          Economy(FAgentEconomy()),
          Units(FAgentUnits()),
          Buildings(FAgentBuildings()) {
    }

    FAgentState(const EGameStateProgression InGameStateProgression, const FAgentAssessments InAssessments,
                const FAgentStrategy InStrategy)
        : GameStateProgression(InGameStateProgression),
          Assessments(InAssessments),
          Strategy(InStrategy),
          Economy(FAgentEconomy()),
          Units(FAgentUnits()),
          Buildings(FAgentBuildings()) {
    }

    // Utility functions to print the formatted agent state data in-place.
    void PrintStatus() const {
        // ANSI escape sequence to clear the screen and move the cursor to the top-left corner.
        std::cout << "\033[2J\033[H";

        // Self Assessment
        /*
        std::cout << "Self Assessment: \n";
        std::cout << "Threat Level: " << Assessments.Self.GetThreatLevelAsString() << "\n";
        std::cout << "Economic Strength: " << Assessments.Self.GetEconomicStrengthAsString() << "\n";
        std::cout << "Military Strength: " << Assessments.Self.GetMilitaryStrengthAsString() << "\n";
        std::cout << "Tech Level: " << Assessments.Self.GetTechLevelAsString() << "\n";
        std::cout << "Map Control: " << Assessments.Self.GetMapControlAsString() << "\n";
        std::cout << "Map Vision: " << Assessments.Self.GetMapVisionAsString() << "\n";
        std::cout << std::endl;

        // Self Strategy
        std::cout << "Self Strategy: \n";
        std::cout << "Strategic Focus: " << Strategy.GetStrategicFocusAsString() << "\n";
        std::cout << "Production Strategy: " << Strategy.GetProductionStrategyAsString() << "\n";
        std::cout << "Military Strategy: " << Strategy.GetMilitaryStrategyAsString() << "\n";
        std::cout << std::endl;
        */

        // Economy Resources
        std::cout << "Economy Resources: \n";
        std::cout << "Workers: " << static_cast<int>(Units.GetUnitCount(UNIT_TYPEID::TERRAN_SCV))
                  << " | Minerals: " << Economy.Minerals << " | Vespene: " << Economy.Vespene
                  << " | Supply: " << static_cast<int>(Economy.Supply) << "/"
                  << static_cast<int>(Economy.SupplyCap)
                  << " | Supply Available: " << static_cast<int>(Economy.SupplyAvailable) << "\n";
        std::cout << std::endl;

        // Military Resources
        std::cout << "Military Resources: \n";
        std::cout << "Army Count: " << Units.ArmyCount
                  << " | Army Value Minerals: " << Units.ArmyValueMinerals
                  << " | Army Value Vespene: " << Units.ArmyValueVespene
                  << " | Army Supply: " << Units.ArmySupply << "\n";
        std::cout << "Marines: " << static_cast<int>(Units.GetUnitCount(UNIT_TYPEID::TERRAN_MARINE))
                  << " | Marauders: " << static_cast<int>(Units.GetUnitCount(UNIT_TYPEID::TERRAN_MARAUDER))
                  << " | Medivacs: " << static_cast<int>(Units.GetUnitCount(UNIT_TYPEID::TERRAN_MEDIVAC))
                  << "\n";
        std::cout << std::endl;

        // Building Counts
        std::cout << "Building Counts: \n";
        std::cout << "Command Centers: "
                  << static_cast<int>(Buildings.GetBuildingCount(UNIT_TYPEID::TERRAN_COMMANDCENTER))
                  << " | Supply Depots: "
                  << static_cast<int>(Buildings.GetBuildingCount(UNIT_TYPEID::TERRAN_SUPPLYDEPOT))
                  << " | Barracks: " << static_cast<int>(Buildings.GetBuildingCount(UNIT_TYPEID::TERRAN_BARRACKS))
                  << " | Factories: " << static_cast<int>(Buildings.GetBuildingCount(UNIT_TYPEID::TERRAN_FACTORY))
                  << " | Starports: " << static_cast<int>(Buildings.GetBuildingCount(UNIT_TYPEID::TERRAN_STARPORT))
                  << "\n";
        std::cout << std::endl;

        // Unit in construction
        std::cout << "Currently Constructing Units: \n";
        std::cout << "Workers: " << static_cast<int>(Units.GetUnitsInConstruction(UNIT_TYPEID::TERRAN_SCV))
                  << " | Marines: " << static_cast<int>(Units.GetUnitsInConstruction(UNIT_TYPEID::TERRAN_MARINE))
                  << " | Marauders: " << static_cast<int>(Units.GetUnitsInConstruction(UNIT_TYPEID::TERRAN_MARAUDER))
                  << " | Medivacs: " << static_cast<int>(Units.GetUnitsInConstruction(UNIT_TYPEID::TERRAN_MEDIVAC))
                  << "\n";
        std::cout << std::endl;

        // Building in construction
        std::cout << "Currently Constructing Buildings: \n";
        std::cout << "Command Centers: "
                  << static_cast<int>(Buildings.GetCurrentlyInConstruction(UNIT_TYPEID::TERRAN_COMMANDCENTER))
                  << " | Supply Depots: "
                  << static_cast<int>(Buildings.GetCurrentlyInConstruction(UNIT_TYPEID::TERRAN_SUPPLYDEPOT))
                  << " | Barracks: "
                  << static_cast<int>(Buildings.GetCurrentlyInConstruction(UNIT_TYPEID::TERRAN_BARRACKS))
                  << " | Factories: "
                  << static_cast<int>(Buildings.GetCurrentlyInConstruction(UNIT_TYPEID::TERRAN_FACTORY))
                  << " | Starports: "
                  << static_cast<int>(Buildings.GetCurrentlyInConstruction(UNIT_TYPEID::TERRAN_STARPORT))
                  << "\n";
        std::cout << std::endl;
    }

    void Update(const ObservationInterface* Observation) {
        if (!Observation) { return; }
        UnitContainer.SetUnits(Observation->GetUnits(Unit::Alliance::Self));

        Economy.Minerals = Observation->GetMinerals();
        Economy.Vespene = Observation->GetVespene();
        Economy.Supply = Observation->GetFoodUsed();
        Economy.SupplyCap = Observation->GetFoodCap();
        Economy.SupplyAvailable = Economy.SupplyCap - Economy.Supply;

        UpdateCounts();
    }

    void SetUnits(const std::vector<const Unit*>& NewUnits) {
        UnitContainer.SetUnits(NewUnits);
    }

    void UpdateCounts() {
        UpdateUnitCounts();
        UpdateUnitsInConstructionCounts();
        UpdateBuildingCounts();
        UpdateBuildingsInConstructionCounts();
        Units.Update();
    }

    void UpdateUnitCounts() {
        for (const UNIT_TYPEID UnitType : TERRAN_UNIT_TYPES) {
            Units.SetUnitCount(UnitType, 0);
        }

        const std::vector<const sc2::Unit*>& SelectedUnits = UnitContainer.GetUnits();
        if (SelectedUnits.empty()) { return; }

        for (const Unit* CurrentUnit : SelectedUnits) {
            const UNIT_TYPEID UnitType = CurrentUnit->unit_type.ToType();
            if (IsTerranUnit(UnitType)) {
                Units.IncrementUnitCount(UnitType);
            }
        }
    }

    void UpdateUnitsInConstructionCounts() {
        for (const UNIT_TYPEID UnitType : TERRAN_UNIT_TYPES) {
            Units.SetUnitsInConstruction(UnitType, 0);
        }

        const std::vector<const sc2::Unit*>& SelectedBuildings = UnitContainer.GetBuildings();
        for (const sc2::Unit* SelectedBuilding : SelectedBuildings) {
            if (!SelectedBuilding->orders.empty()) {
                const UnitOrder& SelectedOrder = SelectedBuilding->orders[0];
                if (IsTrainTerranUnit(SelectedOrder.ability_id)) {
                    Units.IncrementUnitsInConstruction(TerranUnitTrainToUnitType(SelectedOrder.ability_id));
                    continue;
                }
                continue;
            }
        }
    }

    void UpdateBuildingCounts() {
        for (const UNIT_TYPEID UnitType : TERRAN_BUILDING_TYPES) {
            Buildings.SetBuildingCount(UnitType, 0);
        }

        const std::vector<const sc2::Unit*>& FinishedBuildings = UnitContainer.GetBuildings();
        for (const sc2::Unit* const Unit : FinishedBuildings) {
            const UNIT_TYPEID UnitType = Unit->unit_type.ToType();
            if (IsTerranBuilding(UnitType)) {
                Buildings.IncrementBuildingCount(UnitType);
            }
        }
    }

    void UpdateBuildingsInConstructionCounts() {
        for (const UNIT_TYPEID UnitType : TERRAN_BUILDING_TYPES) {
            Buildings.SetCurrentlyInConstruction(UnitType, 0);
        }

        const std::vector<const sc2::Unit*>& UnfinishedBuildings = UnitContainer.GetBuildingsInConstruction();
        for (const sc2::Unit* const Unit : UnfinishedBuildings) {
            const UNIT_TYPEID UnitType = Unit->unit_type.ToType();
            if (IsTerranBuilding(UnitType)) {
                Buildings.IncrementCurrentlyInConstruction(UnitType);
            }
        }
    }
};

}  // namespace sc2
