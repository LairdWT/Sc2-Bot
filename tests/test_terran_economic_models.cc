#include "test_terran_economic_models.h"

#include <iostream>

#include "common/bot_status_models.h"
#include "common/descriptors/FTerranForecastStateBuilder.h"
#include "common/descriptors/FTerranGameStateDescriptorBuilder.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/economic_models.h"
#include "common/economy/EconomyForecastConstants.h"
#include "common/economy/FEconomyDomainState.h"
#include "common/terran_models.h"

namespace sc2
{
namespace
{

bool Check(const bool ConditionValue, bool& SuccessValue, const char* MessageValue)
{
    if (!ConditionValue)
    {
        SuccessValue = false;
        std::cerr << "    " << MessageValue << std::endl;
    }

    return ConditionValue;
}

}  // namespace

bool TestTerranEconomicModels(int ArgC, char** ArgV)
{
    (void)ArgC;
    (void)ArgV;

    bool SuccessValue = true;

    for (const UNIT_TYPEID UnitTypeValue : TERRAN_UNIT_TYPES)
    {
        size_t UnitTypeIndexValue = INVALID_TERRAN_UNIT_TYPE_INDEX;
        const bool FoundValue = TryGetTerranUnitTypeIndex(UnitTypeValue, UnitTypeIndexValue);
        Check(FoundValue, SuccessValue, "Every Terran unit type should resolve to a valid index.");
        Check(IsTerranUnitTypeIndexValid(UnitTypeIndexValue), SuccessValue,
              "Resolved Terran unit index should remain inside the unit table.");

        const FUnitCostData& UnitCostDataValue = TERRAN_ECONOMIC_DATA.GetUnitCostData(UnitTypeValue);
        Check(UnitCostDataValue.UnitType == UnitTypeValue, SuccessValue,
              "Resolved Terran unit cost data should preserve the requested unit type.");
    }

    for (const UNIT_TYPEID BuildingTypeValue : TERRAN_BUILDING_TYPES)
    {
        size_t BuildingTypeIndexValue = INVALID_TERRAN_BUILDING_TYPE_INDEX;
        const bool FoundValue = TryGetTerranBuildingTypeIndex(BuildingTypeValue, BuildingTypeIndexValue);
        Check(FoundValue, SuccessValue, "Every Terran building type should resolve to a valid index.");
        Check(IsTerranBuildingTypeIndexValid(BuildingTypeIndexValue), SuccessValue,
              "Resolved Terran building index should remain inside the building table.");

        const FBuildingCostData& BuildingCostDataValue = TERRAN_ECONOMIC_DATA.GetBuildingCostData(BuildingTypeValue);
        Check(BuildingCostDataValue.BuildingType == BuildingTypeValue, SuccessValue,
              "Resolved Terran building cost data should preserve the requested building type.");
    }

    for (const ABILITY_ID UpgradeTypeValue : TERRAN_RESEARCH_UPGRADE_TYPES)
    {
        size_t UpgradeTypeIndexValue = INVALID_TERRAN_UPGRADE_TYPE_INDEX;
        const bool FoundValue = TryGetTerranUpgradeTypeIndex(UpgradeTypeValue, UpgradeTypeIndexValue);
        Check(FoundValue, SuccessValue, "Every Terran upgrade type should resolve to a valid index.");
        Check(IsTerranUpgradeTypeIndexValid(UpgradeTypeIndexValue), SuccessValue,
              "Resolved Terran upgrade index should remain inside the upgrade table.");

        const FUpgradeCostData& UpgradeCostDataValue = TERRAN_ECONOMIC_DATA.GetUpgradeCostData(UpgradeTypeValue);
        Check(UpgradeCostDataValue.UpgradeType == UpgradeTypeValue, SuccessValue,
              "Resolved Terran upgrade cost data should preserve the requested upgrade type.");
    }

    const UNIT_TYPEID InvalidUnitTypeValue = UNIT_TYPEID::PROTOSS_ZEALOT;
    const UNIT_TYPEID InvalidBuildingTypeValue = UNIT_TYPEID::PROTOSS_PYLON;
    const ABILITY_ID InvalidUpgradeTypeValue = ABILITY_ID::INVALID;

    Check(GetTerranUnitTypeIndex(InvalidUnitTypeValue) == INVALID_TERRAN_UNIT_TYPE_INDEX, SuccessValue,
          "Invalid unit type should return the invalid unit index sentinel.");
    Check(GetTerranBuildingTypeIndex(InvalidBuildingTypeValue) == INVALID_TERRAN_BUILDING_TYPE_INDEX, SuccessValue,
          "Invalid building type should return the invalid building index sentinel.");
    Check(GetTerranUpgradeTypeIndex(InvalidUpgradeTypeValue) == INVALID_TERRAN_UPGRADE_TYPE_INDEX, SuccessValue,
          "Invalid upgrade type should return the invalid upgrade index sentinel.");

    const FUnitCostData& InvalidUnitCostDataValue = TERRAN_ECONOMIC_DATA.GetUnitCostData(InvalidUnitTypeValue);
    Check(InvalidUnitCostDataValue.UnitType == UNIT_TYPEID::INVALID, SuccessValue,
          "Invalid unit type should return empty unit cost data.");
    Check(InvalidUnitCostDataValue.CostData.Minerals == 0U && InvalidUnitCostDataValue.CostData.Vespine == 0U &&
          InvalidUnitCostDataValue.CostData.Supply == 0U && InvalidUnitCostDataValue.CostData.BuildTime == 0U,
          SuccessValue, "Invalid unit cost data should reset to zero.");

    const FBuildingCostData& InvalidBuildingCostDataValue = TERRAN_ECONOMIC_DATA.GetBuildingCostData(InvalidBuildingTypeValue);
    Check(InvalidBuildingCostDataValue.BuildingType == UNIT_TYPEID::INVALID, SuccessValue,
          "Invalid building type should return empty building cost data.");

    const FUpgradeCostData& InvalidUpgradeCostDataValue = TERRAN_ECONOMIC_DATA.GetUpgradeCostData(InvalidUpgradeTypeValue);
    Check(InvalidUpgradeCostDataValue.UpgradeType == ABILITY_ID::INVALID, SuccessValue,
          "Invalid upgrade type should return empty upgrade cost data.");

    const FUnitCostData& MarineCostDataValue = TERRAN_ECONOMIC_DATA.GetUnitCostData(UNIT_TYPEID::TERRAN_MARINE);
    Check(MarineCostDataValue.CostData.Minerals == 50U && MarineCostDataValue.CostData.Vespine == 0U &&
          MarineCostDataValue.CostData.Supply == 1U, SuccessValue,
          "Marine cost data should match the Terran unit table.");

    const FUnitCostData& MedivacCostDataValue = TERRAN_ECONOMIC_DATA.GetUnitCostData(UNIT_TYPEID::TERRAN_MEDIVAC);
    Check(MedivacCostDataValue.CostData.Minerals == 100U && MedivacCostDataValue.CostData.Vespine == 100U &&
          MedivacCostDataValue.CostData.Supply == 2U, SuccessValue,
          "Medivac cost data should match the Terran unit table.");

    const FUnitCostData& WidowMineCostDataValue = TERRAN_ECONOMIC_DATA.GetUnitCostData(UNIT_TYPEID::TERRAN_WIDOWMINE);
    const FUnitCostData& WidowMineBurrowedCostDataValue =
        TERRAN_ECONOMIC_DATA.GetUnitCostData(UNIT_TYPEID::TERRAN_WIDOWMINEBURROWED);
    Check(WidowMineCostDataValue.CostData.Minerals == WidowMineBurrowedCostDataValue.CostData.Minerals &&
          WidowMineCostDataValue.CostData.Vespine == WidowMineBurrowedCostDataValue.CostData.Vespine &&
          WidowMineCostDataValue.CostData.Supply == WidowMineBurrowedCostDataValue.CostData.Supply,
          SuccessValue, "Widow Mine and burrowed Widow Mine should share economic data.");

    const FBuildingCostData& SupplyDepotCostDataValue =
        TERRAN_ECONOMIC_DATA.GetBuildingCostData(UNIT_TYPEID::TERRAN_SUPPLYDEPOT);
    const FBuildingCostData& SupplyDepotLoweredCostDataValue =
        TERRAN_ECONOMIC_DATA.GetBuildingCostData(UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED);
    Check(SupplyDepotCostDataValue.CostData.Minerals == 100U && SupplyDepotCostDataValue.CostData.Vespine == 0U,
          SuccessValue, "Supply Depot cost data should match the Terran building table.");
    Check(SupplyDepotCostDataValue.CostData.Minerals == SupplyDepotLoweredCostDataValue.CostData.Minerals &&
          SupplyDepotCostDataValue.CostData.Vespine == SupplyDepotLoweredCostDataValue.CostData.Vespine,
          SuccessValue, "Lowered Supply Depot should share economic data with Supply Depot.");

    FAgentUnits AgentUnitsValue;
    AgentUnitsValue.SetUnitCount(InvalidUnitTypeValue, 7U);
    AgentUnitsValue.IncrementUnitCount(InvalidUnitTypeValue);
    AgentUnitsValue.IncrementUnitsInConstruction(InvalidUnitTypeValue);
    Check(AgentUnitsValue.GetUnitCount(InvalidUnitTypeValue) == 0U, SuccessValue,
          "Invalid unit type should not mutate unit counts.");
    Check(AgentUnitsValue.GetUnitsInConstruction(InvalidUnitTypeValue) == 0U, SuccessValue,
          "Invalid unit type should not mutate units-in-construction counts.");

    for (const UNIT_TYPEID UnitTypeValue : TERRAN_UNIT_TYPES)
    {
        AgentUnitsValue.SetUnitCount(UnitTypeValue, 1U);
    }

    AgentUnitsValue.UpdateArmyUnitCounts();
    AgentUnitsValue.UpdateArmyUnitValues();
    Check(AgentUnitsValue.GetArmyCount() == static_cast<uint16_t>(NUM_TERRAN_UNITS - 2U), SuccessValue,
          "Army unit counting should iterate all Terran unit types without invalid indexing.");

    FAgentBuildings AgentBuildingsValue;
    AgentBuildingsValue.SetBuildingCount(InvalidBuildingTypeValue, 3U);
    AgentBuildingsValue.IncrementBuildingCount(InvalidBuildingTypeValue);
    AgentBuildingsValue.IncrementCurrentlyInConstruction(InvalidBuildingTypeValue);
    Check(AgentBuildingsValue.GetBuildingCount(InvalidBuildingTypeValue) == 0U, SuccessValue,
          "Invalid building type should not mutate building counts.");
    Check(AgentBuildingsValue.GetCurrentlyInConstruction(InvalidBuildingTypeValue) == 0U, SuccessValue,
          "Invalid building type should not mutate in-construction building counts.");

    {
        FTerranGameStateDescriptorBuilder GameStateDescriptorBuilderValue;
        FTerranForecastStateBuilder ForecastStateBuilderValue;
        FEconomyDomainState EconomyDomainStateValue;

        FAgentState SeedAgentStateValue;
        SeedAgentStateValue.Economy.Minerals = 50U;
        SeedAgentStateValue.Economy.Vespene = 0U;
        SeedAgentStateValue.Economy.Supply = 14U;
        SeedAgentStateValue.Economy.SupplyCap = 23U;
        SeedAgentStateValue.Economy.SupplyAvailable = 9U;
        SeedAgentStateValue.Units.SetUnitCount(UNIT_TYPEID::TERRAN_SCV, 14U);
        SeedAgentStateValue.Units.Update();
        SeedAgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_COMMANDCENTER, 1U);
        SeedAgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 1U);

        FGameStateDescriptor SeedDescriptorValue;
        GameStateDescriptorBuilderValue.RebuildGameStateDescriptor(1U, 100U, SeedAgentStateValue, SeedDescriptorValue);
        ForecastStateBuilderValue.RebuildForecastState(SeedAgentStateValue, EconomyDomainStateValue,
                                                       SeedDescriptorValue);

        FAgentState SpendingAgentStateValue;
        SpendingAgentStateValue.Economy.Minerals = 40U;
        SpendingAgentStateValue.Economy.Vespene = 0U;
        SpendingAgentStateValue.Economy.Supply = 15U;
        SpendingAgentStateValue.Economy.SupplyCap = 23U;
        SpendingAgentStateValue.Economy.SupplyAvailable = 8U;
        SpendingAgentStateValue.Units.SetUnitCount(UNIT_TYPEID::TERRAN_SCV, 14U);
        SpendingAgentStateValue.Units.SetUnitsInConstruction(UNIT_TYPEID::TERRAN_SCV, 1U);
        SpendingAgentStateValue.Units.Update();
        SpendingAgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_COMMANDCENTER, 1U);
        SpendingAgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 1U);
        SpendingAgentStateValue.Buildings.SetCurrentlyInConstruction(UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 1U);
        EconomyDomainStateValue.Update(SpendingAgentStateValue, 196U);

        FAgentState CompletionAgentStateValue;
        CompletionAgentStateValue.Economy.Minerals = 80U;
        CompletionAgentStateValue.Economy.Vespene = 0U;
        CompletionAgentStateValue.Economy.Supply = 15U;
        CompletionAgentStateValue.Economy.SupplyCap = 31U;
        CompletionAgentStateValue.Economy.SupplyAvailable = 16U;
        CompletionAgentStateValue.Units.SetUnitCount(UNIT_TYPEID::TERRAN_SCV, 15U);
        CompletionAgentStateValue.Units.Update();
        CompletionAgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_COMMANDCENTER, 1U);
        CompletionAgentStateValue.Buildings.SetBuildingCount(UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 2U);

        FGameStateDescriptor CompletionDescriptorValue;
        GameStateDescriptorBuilderValue.RebuildGameStateDescriptor(2U, 292U, CompletionAgentStateValue,
                                                                  CompletionDescriptorValue);
        ForecastStateBuilderValue.RebuildForecastState(CompletionAgentStateValue, EconomyDomainStateValue,
                                                       CompletionDescriptorValue);

        Check(EconomyDomainStateValue.GetElapsedGameLoopsForHorizon(ShortForecastHorizonIndexValue) == 96U,
              SuccessValue, "Short-horizon economy windows should use the configured 96 game-loop span when history is available.");
        Check(EconomyDomainStateValue.GetElapsedGameLoopsForHorizon(MediumForecastHorizonIndexValue) == 192U,
              SuccessValue, "Medium-horizon economy windows should fall back to the oldest retained sample when history is shorter than the configured horizon.");
        Check(EconomyDomainStateValue.GetGrossMineralIncomeForHorizon(ShortForecastHorizonIndexValue) == 40U,
              SuccessValue, "Short-horizon gross mineral income should exclude earlier spend and gather samples.");
        Check(EconomyDomainStateValue.GetGrossMineralIncomeForHorizon(MediumForecastHorizonIndexValue) == 180U,
              SuccessValue, "Medium-horizon gross mineral income should follow bank delta plus confirmed spend accounting.");
        Check(EconomyDomainStateValue.GetNetMineralDeltaForHorizon(ShortForecastHorizonIndexValue) == 40,
              SuccessValue, "Short-horizon net mineral delta should track the observed bank change across the retained sample window.");
        Check(EconomyDomainStateValue.GetUnitCompletionCountForHorizon(UNIT_TYPEID::TERRAN_SCV,
                                                                       ShortForecastHorizonIndexValue) == 1U,
              SuccessValue, "Short-horizon unit completion counts should record completed SCVs.");
        Check(EconomyDomainStateValue.GetBuildingCompletionCountForHorizon(UNIT_TYPEID::TERRAN_SUPPLYDEPOT,
                                                                           ShortForecastHorizonIndexValue) == 1U,
              SuccessValue, "Short-horizon building completion counts should record completed supply depots.");
        Check(CompletionDescriptorValue.EconomyState.GrossMineralIncomeAverageByHorizon[ShortForecastHorizonIndexValue] >
                      0.4f &&
                  CompletionDescriptorValue.EconomyState.GrossMineralIncomeAverageByHorizon[
                      ShortForecastHorizonIndexValue] < 0.5f,
              SuccessValue, "Economy forecast should expose moving averages for gross mineral gather rates.");
        Check(CompletionDescriptorValue.ProductionState.UnitCompletionAveragesByHorizon[GetTerranUnitTypeIndex(
                  UNIT_TYPEID::TERRAN_SCV)][ShortForecastHorizonIndexValue] > 0.0f,
              SuccessValue, "Production forecast should expose moving averages for per-unit completion rates.");
        Check(CompletionDescriptorValue.ProductionState.BuildingCompletionAveragesByHorizon[GetTerranBuildingTypeIndex(
                  UNIT_TYPEID::TERRAN_SUPPLYDEPOT)][ShortForecastHorizonIndexValue] > 0.0f,
              SuccessValue, "Production forecast should expose moving averages for per-building completion rates.");
        Check(CompletionDescriptorValue.EconomyState.ProjectedMineralsByHorizon[ShortForecastHorizonIndexValue] >
                  CompletionDescriptorValue.EconomyState.CurrentMinerals,
              SuccessValue, "Economy forecast should project short-horizon minerals from the moving gross-gather average.");
    }

    return SuccessValue;
}

}  // namespace sc2
