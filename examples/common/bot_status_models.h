#pragma once

#include <array>
#include <cstdint>
#include <iostream>
#include <string>

#include "sc2api/sc2_api.h"

#include "common/agent_framework.h"
#include "common/economic_models.h"
#include "common/terran_models.h"
#include "common/terran_unit_container.h"

namespace sc2
{

enum class EGameStateProgression : uint8_t
{
    Opening,
    EarlyGame,
    MidGame,
    LateGame,
    EndGame,
};

enum class EThreatLevel : uint8_t
{
    Unknown,
    Low,
    Medium,
    High,
    Maximum,
};

enum class EEconomicStrength : uint8_t
{
    Unknown,
    Dire,
    Weak,
    Good,
    Strong,
    Maximum,
};

enum class EMilitaryStrength : uint8_t
{
    Unknown,
    Dire,
    Weak,
    Good,
    Strong,
    Maximum,
};

enum class ETechLevel : uint8_t
{
    Unknown,
    None,
    Low,
    Medium,
    High,
    Maximum,
};

enum class EMapControl : uint8_t
{
    Unknown,
    None,
    Low,
    Medium,
    High,
    Maximum,
};

enum class EMapVision : uint8_t
{
    Unknown,
    None,
    Low,
    Medium,
    High,
    Maximum,
};

struct FAgentAssessment
{
    EThreatLevel ThreatLevel;
    EEconomicStrength EconomicStrength;
    EMilitaryStrength MilitaryStrength;
    ETechLevel TechLevel;
    EMapControl MapControl;
    EMapVision MapVision;

    FAgentAssessment()
        : ThreatLevel(EThreatLevel::Unknown),
          EconomicStrength(EEconomicStrength::Unknown),
          MilitaryStrength(EMilitaryStrength::Unknown),
          TechLevel(ETechLevel::Unknown),
          MapControl(EMapControl::Unknown),
          MapVision(EMapVision::Unknown)
    {
    }

    FAgentAssessment(EThreatLevel InThreatLevel, EEconomicStrength InEconomicStrength,
                     EMilitaryStrength InMilitaryStrength, ETechLevel InTechLevel, EMapControl InMapControl,
                     EMapVision InMapVision)
        : ThreatLevel(InThreatLevel),
          EconomicStrength(InEconomicStrength),
          MilitaryStrength(InMilitaryStrength),
          TechLevel(InTechLevel),
          MapControl(InMapControl),
          MapVision(InMapVision)
    {
    }

    std::string GetThreatLevelAsString() const
    {
        switch (ThreatLevel)
        {
            case EThreatLevel::Unknown:
                return "Unknown";
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

    std::string GetEconomicStrengthAsString() const
    {
        switch (EconomicStrength)
        {
            case EEconomicStrength::Unknown:
                return "Unknown";
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

    std::string GetMilitaryStrengthAsString() const
    {
        switch (MilitaryStrength)
        {
            case EMilitaryStrength::Unknown:
                return "Unknown";
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

    std::string GetTechLevelAsString() const
    {
        switch (TechLevel)
        {
            case ETechLevel::Unknown:
                return "Unknown";
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

    std::string GetMapControlAsString() const
    {
        switch (MapControl)
        {
            case EMapControl::Unknown:
                return "Unknown";
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

    std::string GetMapVisionAsString() const
    {
        switch (MapVision)
        {
            case EMapVision::Unknown:
                return "Unknown";
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

struct FAgentAssessments
{
    FAgentAssessment Self;
    FAgentAssessment Opponent;

    FAgentAssessments()
        : Self(FAgentAssessment()), Opponent(FAgentAssessment())
    {
    }

    FAgentAssessments(const FAgentAssessment InBotAssessment, const FAgentAssessment InOpponentAssessment)
        : Self(InBotAssessment), Opponent(InOpponentAssessment)
    {
    }
};

struct FAgentEconomy
{
    uint32_t Minerals;
    uint32_t Vespene;
    uint8_t Supply;
    uint8_t SupplyCap;
    uint8_t SupplyAvailable;

    FAgentEconomy()
        : Minerals(0), Vespene(0), Supply(0), SupplyCap(0), SupplyAvailable(0)
    {
    }

    FAgentEconomy(uint32_t InMinerals, uint32_t InVespene, uint8_t InSupply, uint8_t InSupplyCap,
                  uint8_t InSupplyAvailable)
        : Minerals(InMinerals),
          Vespene(InVespene),
          Supply(InSupply),
          SupplyCap(InSupplyCap),
          SupplyAvailable(InSupplyAvailable)
    {
    }
};

struct FAgentUnits
{
    uint32_t ArmyCount;
    uint32_t ArmyValueMinerals;
    uint32_t ArmyValueVespene;
    uint32_t ArmySupply;

    std::array<uint16_t, NUM_TERRAN_UNITS> UnitCounts;
    std::array<uint16_t, NUM_TERRAN_UNITS> UnitsInConstruction;

    FAgentUnits()
        : ArmyCount(0), ArmyValueMinerals(0), ArmyValueVespene(0), ArmySupply(0), UnitCounts{}, UnitsInConstruction{}
    {
    }

    void Update()
    {
        UpdateArmyUnitCounts();
        UpdateArmyUnitValues();
    }

    void UpdateArmyUnitCounts()
    {
        ArmyCount = 0;
        for (const UNIT_TYPEID UnitType : TERRAN_UNIT_TYPES)
        {
            switch (UnitType)
            {
                case UNIT_TYPEID::TERRAN_SCV:
                case UNIT_TYPEID::TERRAN_MULE:
                    continue;
                default:
                    ArmyCount += GetUnitCount(UnitType);
                    break;
            }
        }
    }

    void UpdateArmyUnitValues()
    {
        ArmyValueMinerals = 0;
        ArmyValueVespene = 0;
        ArmySupply = 0;
        for (const UNIT_TYPEID UnitType : TERRAN_UNIT_TYPES)
        {
            switch (UnitType)
            {
                case UNIT_TYPEID::TERRAN_SCV:
                case UNIT_TYPEID::TERRAN_MULE:
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

    uint16_t GetArmyCount() const
    {
        return ArmyCount;
    }

    uint16_t GetUnitCount(UNIT_TYPEID UnitType) const
    {
        return UnitCounts[GetTerranUnitTypeIndex(UnitType)];
    }

    void SetUnitCount(UNIT_TYPEID UnitType, uint16_t Count)
    {
        UnitCounts[GetTerranUnitTypeIndex(UnitType)] = Count;
    }

    void IncrementUnitCount(UNIT_TYPEID UnitType)
    {
        UnitCounts[GetTerranUnitTypeIndex(UnitType)]++;
    }

    void DecrementUnitCount(UNIT_TYPEID UnitType)
    {
        const size_t Index = GetTerranUnitTypeIndex(UnitType);
        if (UnitCounts[Index] > 0)
        {
            UnitCounts[Index]--;
        }
    }

    void SetUnitsInConstruction(UNIT_TYPEID UnitType, uint16_t Count)
    {
        UnitsInConstruction[GetTerranUnitTypeIndex(UnitType)] = Count;
    }

    void IncrementUnitsInConstruction(UNIT_TYPEID UnitType)
    {
        UnitsInConstruction[GetTerranUnitTypeIndex(UnitType)]++;
    }

    uint16_t GetUnitsInConstruction(UNIT_TYPEID UnitType) const
    {
        return UnitsInConstruction[GetTerranUnitTypeIndex(UnitType)];
    }

    uint16_t GetWorkerCount() const
    {
        return GetUnitCount(UNIT_TYPEID::TERRAN_SCV);
    }
};

struct FAgentBuildings
{
    std::array<uint16_t, NUM_TERRAN_BUILDINGS> BuildingCounts;
    std::array<uint16_t, NUM_TERRAN_BUILDINGS> CurrentlyInConstruction;

    FAgentBuildings()
        : BuildingCounts{}, CurrentlyInConstruction{}
    {
        for (uint16_t& Count : BuildingCounts)
        {
            Count = 0;
        }
        for (uint16_t& Count : CurrentlyInConstruction)
        {
            Count = 0;
        }
    }

    uint16_t GetBuildingCount(UNIT_TYPEID BuildingType) const
    {
        return BuildingCounts[GetTerranBuildingTypeIndex(BuildingType)];
    }

    void SetBuildingCount(UNIT_TYPEID BuildingType, uint16_t Count)
    {
        BuildingCounts[GetTerranBuildingTypeIndex(BuildingType)] = Count;
    }

    void IncrementBuildingCount(UNIT_TYPEID BuildingType)
    {
        BuildingCounts[GetTerranBuildingTypeIndex(BuildingType)]++;
    }

    void DecrementBuildingCount(UNIT_TYPEID BuildingType)
    {
        const size_t Index = GetTerranBuildingTypeIndex(BuildingType);
        if (BuildingCounts[Index] > 0)
        {
            BuildingCounts[Index]--;
        }
    }

    uint16_t GetCurrentlyInConstruction(UNIT_TYPEID BuildingType) const
    {
        return CurrentlyInConstruction[GetTerranBuildingTypeIndex(BuildingType)];
    }

    void SetCurrentlyInConstruction(UNIT_TYPEID BuildingType, uint16_t Count)
    {
        CurrentlyInConstruction[GetTerranBuildingTypeIndex(BuildingType)] = Count;
    }

    void IncrementCurrentlyInConstruction(UNIT_TYPEID BuildingType)
    {
        CurrentlyInConstruction[GetTerranBuildingTypeIndex(BuildingType)]++;
    }

    uint16_t GetTownHallCount() const
    {
        return GetBuildingCount(UNIT_TYPEID::TERRAN_COMMANDCENTER) +
               GetBuildingCount(UNIT_TYPEID::TERRAN_COMMANDCENTERFLYING) +
               GetBuildingCount(UNIT_TYPEID::TERRAN_ORBITALCOMMAND) +
               GetBuildingCount(UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING) +
               GetBuildingCount(UNIT_TYPEID::TERRAN_PLANETARYFORTRESS);
    }

    uint16_t GetSupplyDepotCount() const
    {
        return GetBuildingCount(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) +
               GetBuildingCount(UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED);
    }

    uint16_t GetBarracksCount() const
    {
        return GetBuildingCount(UNIT_TYPEID::TERRAN_BARRACKS) +
               GetBuildingCount(UNIT_TYPEID::TERRAN_BARRACKSFLYING);
    }

    uint16_t GetBarracksAddonCount() const
    {
        return GetBuildingCount(UNIT_TYPEID::TERRAN_BARRACKSREACTOR) +
               GetBuildingCount(UNIT_TYPEID::TERRAN_BARRACKSTECHLAB);
    }

    uint16_t GetFactoryCount() const
    {
        return GetBuildingCount(UNIT_TYPEID::TERRAN_FACTORY) +
               GetBuildingCount(UNIT_TYPEID::TERRAN_FACTORYFLYING);
    }

    uint16_t GetFactoryAddonCount() const
    {
        return GetBuildingCount(UNIT_TYPEID::TERRAN_FACTORYREACTOR) +
               GetBuildingCount(UNIT_TYPEID::TERRAN_FACTORYTECHLAB);
    }

    uint16_t GetStarportCount() const
    {
        return GetBuildingCount(UNIT_TYPEID::TERRAN_STARPORT) +
               GetBuildingCount(UNIT_TYPEID::TERRAN_STARPORTFLYING);
    }

    uint16_t GetStarportAddonCount() const
    {
        return GetBuildingCount(UNIT_TYPEID::TERRAN_STARPORTREACTOR) +
               GetBuildingCount(UNIT_TYPEID::TERRAN_STARPORTTECHLAB);
    }
};

enum class EStrategicFocus : uint8_t
{
    Unknown,
    Economic,
    Defensive,
    Balanced,
    Aggressive,
    AllIn,
};

enum class EProductionStrategy : uint8_t
{
    Unknown,
    Workers,
    Expansion,
    ArmyBuildings,
    ArmyUnits,
    ArmyUpgrades,
    TechUp,
};

enum class EMilitaryStrategy : uint8_t
{
    Unknown,
    MapControl,
    Harass,
    Defend,
    AttackNatural,
    AttackMain,
    AllIn,
};

struct FAgentStrategy
{
    EStrategicFocus StrategicFocus;
    EProductionStrategy ProductionStrategy;
    EMilitaryStrategy MilitaryStrategy;

    FAgentStrategy()
        : StrategicFocus(EStrategicFocus::Unknown),
          ProductionStrategy(EProductionStrategy::Unknown),
          MilitaryStrategy(EMilitaryStrategy::Unknown)
    {
    }

    FAgentStrategy(EStrategicFocus InStrategicFocus, EProductionStrategy InProductionStrategy,
                   EMilitaryStrategy InMilitaryStrategy)
        : StrategicFocus(InStrategicFocus),
          ProductionStrategy(InProductionStrategy),
          MilitaryStrategy(InMilitaryStrategy)
    {
    }

    std::string GetStrategicFocusAsString() const
    {
        switch (StrategicFocus)
        {
            case EStrategicFocus::Unknown:
                return "Unknown";
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

    std::string GetProductionStrategyAsString() const
    {
        switch (ProductionStrategy)
        {
            case EProductionStrategy::Unknown:
                return "Unknown";
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

    std::string GetMilitaryStrategyAsString() const
    {
        switch (MilitaryStrategy)
        {
            case EMilitaryStrategy::Unknown:
                return "Unknown";
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

struct FAgentState
{
    EGameStateProgression GameStateProgression;
    FAgentAssessments Assessments;
    FAgentStrategy Strategy;

    FTerranUnitContainer UnitContainer;

    FAgentEconomy Economy;
    FAgentUnits Units;
    FAgentBuildings Buildings;
    FAgentSpatialChannels SpatialChannels;
    FAgentSpatialMetrics SpatialMetrics;

    FAgentState()
        : GameStateProgression(EGameStateProgression::Opening),
          Assessments(FAgentAssessments()),
          Strategy(FAgentStrategy()),
          Economy(FAgentEconomy()),
          Units(FAgentUnits()),
          Buildings(FAgentBuildings()),
          SpatialChannels(FAgentSpatialChannels()),
          SpatialMetrics(FAgentSpatialMetrics())
    {
    }

    FAgentState(EGameStateProgression InGameStateProgression, FAgentAssessments InAssessments,
                FAgentStrategy InStrategy)
        : GameStateProgression(InGameStateProgression),
          Assessments(InAssessments),
          Strategy(InStrategy),
          Economy(FAgentEconomy()),
          Units(FAgentUnits()),
          Buildings(FAgentBuildings()),
          SpatialChannels(FAgentSpatialChannels()),
          SpatialMetrics(FAgentSpatialMetrics())
    {
    }

    void PrintStatus() const
    {
        std::cout << "\033[2J\033[H";

        std::cout << "Economy Resources: \n";
        std::cout << "Workers: " << static_cast<int>(Units.GetUnitCount(UNIT_TYPEID::TERRAN_SCV))
                  << " | Minerals: " << Economy.Minerals << " | Vespene: " << Economy.Vespene
                  << " | Supply: " << static_cast<int>(Economy.Supply) << "/"
                  << static_cast<int>(Economy.SupplyCap)
                  << " | Supply Available: " << static_cast<int>(Economy.SupplyAvailable) << "\n";
        std::cout << std::endl;

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

        std::cout << "Currently Constructing Units: \n";
        std::cout << "Workers: " << static_cast<int>(Units.GetUnitsInConstruction(UNIT_TYPEID::TERRAN_SCV))
                  << " | Marines: " << static_cast<int>(Units.GetUnitsInConstruction(UNIT_TYPEID::TERRAN_MARINE))
                  << " | Marauders: " << static_cast<int>(Units.GetUnitsInConstruction(UNIT_TYPEID::TERRAN_MARAUDER))
                  << " | Medivacs: " << static_cast<int>(Units.GetUnitsInConstruction(UNIT_TYPEID::TERRAN_MEDIVAC))
                  << "\n";
        std::cout << std::endl;

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

        std::cout << "Spatial Metrics: \n";
        std::cout << "Feature Layers: " << (SpatialMetrics.Valid ? "Valid" : "Unavailable")
                  << " | Minimap Enemy Seen: " << (SpatialMetrics.Minimap.HasEnemy ? "Yes" : "No")
                  << " | Map Friendly Seen: " << (SpatialMetrics.Map.HasSelf ? "Yes" : "No") << "\n";
        std::cout << std::endl;
    }

    void Update(const FFrameContext& Frame)
    {
        if (!Frame.Observation)
        {
            return;
        }

        UnitContainer.SetUnits(Frame.Observation->GetUnits(Unit::Alliance::Self));

        Economy.Minerals = static_cast<uint32_t>(Frame.Observation->GetMinerals());
        Economy.Vespene = static_cast<uint32_t>(Frame.Observation->GetVespene());
        Economy.Supply = static_cast<uint8_t>(Frame.Observation->GetFoodUsed());
        Economy.SupplyCap = static_cast<uint8_t>(Frame.Observation->GetFoodCap());
        Economy.SupplyAvailable = static_cast<uint8_t>(Economy.SupplyCap - Economy.Supply);

        SpatialChannels.Update(Frame);
        SpatialMetrics.Update(SpatialChannels);

        UpdateCounts();
    }

    void Update(const ObservationInterface* ObservationPtr)
    {
        Update(FFrameContext::Create(ObservationPtr, nullptr, 0));
    }

    void SetUnits(const std::vector<const Unit*>& NewUnits)
    {
        UnitContainer.SetUnits(NewUnits);
    }

    void UpdateCounts()
    {
        UpdateUnitCounts();
        UpdateUnitsInConstructionCounts();
        UpdateBuildingCounts();
        UpdateBuildingsInConstructionCounts();
        Units.Update();
    }

    void UpdateUnitCounts()
    {
        for (const UNIT_TYPEID UnitType : TERRAN_UNIT_TYPES)
        {
            Units.SetUnitCount(UnitType, 0);
        }

        const std::vector<const Unit*>& SelectedUnits = UnitContainer.GetUnits();
        for (const Unit* CurrentUnit : SelectedUnits)
        {
            const UNIT_TYPEID UnitType = CurrentUnit->unit_type.ToType();
            if (IsTerranUnit(UnitType))
            {
                Units.IncrementUnitCount(UnitType);
            }
        }
    }

    void UpdateUnitsInConstructionCounts()
    {
        for (const UNIT_TYPEID UnitType : TERRAN_UNIT_TYPES)
        {
            Units.SetUnitsInConstruction(UnitType, 0);
        }

        const std::vector<const Unit*>& SelectedBuildings = UnitContainer.GetBuildings();
        for (const Unit* SelectedBuilding : SelectedBuildings)
        {
            if (SelectedBuilding->orders.empty())
            {
                continue;
            }

            const UnitOrder& SelectedOrder = SelectedBuilding->orders.front();
            if (IsTrainTerranUnit(SelectedOrder.ability_id))
            {
                Units.IncrementUnitsInConstruction(TerranUnitTrainToUnitType(SelectedOrder.ability_id));
            }
        }
    }

    void UpdateBuildingCounts()
    {
        for (const UNIT_TYPEID UnitType : TERRAN_BUILDING_TYPES)
        {
            Buildings.SetBuildingCount(UnitType, 0);
        }

        const std::vector<const Unit*>& FinishedBuildings = UnitContainer.GetBuildings();
        for (const Unit* BuildingUnit : FinishedBuildings)
        {
            const UNIT_TYPEID UnitType = BuildingUnit->unit_type.ToType();
            if (IsTerranBuilding(UnitType))
            {
                Buildings.IncrementBuildingCount(UnitType);
            }
        }
    }

    void UpdateBuildingsInConstructionCounts()
    {
        for (const UNIT_TYPEID UnitType : TERRAN_BUILDING_TYPES)
        {
            Buildings.SetCurrentlyInConstruction(UnitType, 0);
        }

        const std::vector<const Unit*>& UnfinishedBuildings = UnitContainer.GetBuildingsInConstruction();
        for (const Unit* BuildingUnit : UnfinishedBuildings)
        {
            const UNIT_TYPEID UnitType = BuildingUnit->unit_type.ToType();
            if (IsTerranBuilding(UnitType))
            {
                Buildings.IncrementCurrentlyInConstruction(UnitType);
            }
        }
    }
};

}  // namespace sc2
