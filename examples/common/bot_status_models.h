#pragma once

#include <cstdint>
#include <string>
#include <iostream>

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

struct FAgentEconomyResources {
    uint8_t Workers;
    uint8_t SupplyDepots;
    uint8_t CommandCenters;
    uint32_t Minerals;
    uint32_t Vespene;
    uint8_t Supply;
    uint8_t SupplyCap;
    uint8_t SupplyAvailable;

    FAgentEconomyResources()
        : Workers(0),
          SupplyDepots(0),
          CommandCenters(0),
          Minerals(0),
          Vespene(0),
          Supply(0),
          SupplyCap(0),
          SupplyAvailable(0) {
    }

    FAgentEconomyResources(const uint8_t InWorkers, const uint8_t InSupplyDepots, const uint8_t InCommandCenters,
                           const uint32_t InMinerals, const uint32_t InVespene, const uint8_t InSupply,
                           const uint8_t InSupplyCap, const uint8_t InSupplyAvailable)
        : Workers(InWorkers),
          SupplyDepots(InSupplyDepots),
          CommandCenters(InCommandCenters),
          Minerals(InMinerals),
          Vespene(InVespene),
          Supply(InSupply),
          SupplyCap(InSupplyCap),
          SupplyAvailable(InSupplyAvailable) {
    }
};

struct FAgentMilitaryResources {
    uint32_t ArmyCount;

    uint32_t ArmyValue;
    uint32_t ArmySupply;

    uint8_t Barracks;
    uint8_t Factories;
    uint8_t Starports;

    uint32_t Marines;
    uint32_t Marauders;
    uint32_t Medivacs;

    FAgentMilitaryResources()
        : ArmyCount(0),
          ArmyValue(0),
          ArmySupply(0),
          Barracks(0),
          Factories(0),
          Starports(0),
          Marines(0),
          Marauders(0),
          Medivacs(0) {
    }

    FAgentMilitaryResources(const uint32_t InArmyCount, const uint32_t InArmyValue, const uint32_t InArmySupply,
                            const uint8_t InBarracks, const uint8_t InFactories, const uint8_t InStarports,
                            const uint32_t InMarines, const uint32_t InMarauders, const uint32_t InMedivacs)
        : ArmyCount(InArmyCount),
          ArmyValue(InArmyValue),
          ArmySupply(InArmySupply),
          Barracks(InBarracks),
          Factories(InFactories),
          Starports(InStarports),
          Marines(InMarines),
          Marauders(InMarauders),
          Medivacs(InMedivacs) {
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

    FAgentEconomyResources EconomyResources;
    FAgentMilitaryResources MilitaryResources;

    FAgentState()
        : GameStateProgression(EGameStateProgression::Opening),
          Assessments(FAgentAssessments()),
          Strategy(FAgentStrategy()),
          EconomyResources(FAgentEconomyResources()),
          MilitaryResources(FAgentMilitaryResources()) {
    }

    FAgentState(const EGameStateProgression InGameStateProgression, const FAgentAssessments InAssessments,
                const FAgentStrategy InStrategy)
        : GameStateProgression(InGameStateProgression),
          Assessments(InAssessments),
          Strategy(InStrategy),
          EconomyResources(FAgentEconomyResources()),
          MilitaryResources(FAgentMilitaryResources()) {
    }

    // Utility functions to print the formatted agent state data in-place.
    void PrintStatus() const {
        // ANSI escape sequence to clear the screen and move the cursor to the top-left corner.
        std::cout << "\033[2J\033[H";

        // Self Assessment
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

        // Economy Resources
        std::cout << "Economy Resources: \n";
        std::cout << "Workers: " << static_cast<int>(EconomyResources.Workers)
                  << " | Minerals: " << EconomyResources.Minerals << " | Vespene: " << EconomyResources.Vespene
                  << " | Supply: " << static_cast<int>(EconomyResources.Supply) << "/"
                  << static_cast<int>(EconomyResources.SupplyCap)
                  << " | Available: " << static_cast<int>(EconomyResources.SupplyAvailable) << "\n";
        std::cout << std::endl;

        // Military Resources
        std::cout << "Military Resources: \n";
        std::cout << "Army Count: " << MilitaryResources.ArmyCount << " | Army Value: " << MilitaryResources.ArmyValue
                  << " | Army Supply: " << MilitaryResources.ArmySupply << "\n";
        std::cout << "Marines: " << MilitaryResources.Marines << " | Marauders: " << MilitaryResources.Marauders
                  << " | Medivacs: " << MilitaryResources.Medivacs << "\n";
        std::cout << "Barracks: " << static_cast<int>(MilitaryResources.Barracks)
                  << " | Factories: " << static_cast<int>(MilitaryResources.Factories)
                  << " | Starports: " << static_cast<int>(MilitaryResources.Starports) << "\n";
        std::cout << std::endl;

        // Opponent Assessment
        std::cout << "Opponent Assessment: \n";
        std::cout << "Threat Level: " << Assessments.Opponent.GetThreatLevelAsString() << "\n";
        std::cout << "Economic Strength: " << Assessments.Opponent.GetEconomicStrengthAsString() << "\n";
        std::cout << "Military Strength: " << Assessments.Opponent.GetMilitaryStrengthAsString() << "\n";
        std::cout << "Tech Level: " << Assessments.Opponent.GetTechLevelAsString() << "\n";
        std::cout << "Map Control: " << Assessments.Opponent.GetMapControlAsString() << "\n";
        std::cout << "Map Vision: " << Assessments.Opponent.GetMapVisionAsString() << "\n";
        std::cout << std::endl;
    }
};

}  // namespace sc2
