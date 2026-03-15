#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "common/economy/EconomyForecastConstants.h"

namespace sc2
{

struct FEconomyStateDescriptor
{
public:
    FEconomyStateDescriptor();

    void Reset();
    uint32_t GetProjectedMineralsAtHorizon(size_t HorizonIndexValue) const;
    uint32_t GetProjectedVespeneAtHorizon(size_t HorizonIndexValue) const;
    uint32_t GetProjectedSupplyAvailableAtHorizon(size_t HorizonIndexValue) const;
    uint32_t GetProjectedAvailableMineralsAtHorizon(size_t HorizonIndexValue) const;
    uint32_t GetProjectedAvailableVespeneAtHorizon(size_t HorizonIndexValue) const;
    uint32_t GetProjectedAvailableSupplyAtHorizon(size_t HorizonIndexValue) const;

public:
    uint32_t CurrentMinerals;
    uint32_t CurrentVespene;
    uint32_t CurrentSupplyUsed;
    uint32_t CurrentSupplyCap;
    uint32_t CurrentSupplyAvailable;
    uint32_t BudgetedMinerals;
    uint32_t BudgetedVespene;
    uint32_t BudgetedSupplyAvailable;
    uint32_t ReservedMinerals;
    uint32_t ReservedVespene;
    uint32_t ReservedSupply;
    uint32_t CommittedMinerals;
    uint32_t CommittedVespene;
    uint32_t CommittedSupply;
    std::array<uint64_t, ForecastHorizonCountValue> HorizonGameLoops;
    std::array<uint32_t, ForecastHorizonCountValue> GrossMineralIncomeByHorizon;
    std::array<uint32_t, ForecastHorizonCountValue> GrossVespeneIncomeByHorizon;
    std::array<int32_t, ForecastHorizonCountValue> NetMineralDeltaByHorizon;
    std::array<int32_t, ForecastHorizonCountValue> NetVespeneDeltaByHorizon;
    std::array<float, ForecastHorizonCountValue> GrossMineralIncomeAverageByHorizon;
    std::array<float, ForecastHorizonCountValue> GrossVespeneIncomeAverageByHorizon;
    std::array<float, ForecastHorizonCountValue> NetMineralDeltaAverageByHorizon;
    std::array<float, ForecastHorizonCountValue> NetVespeneDeltaAverageByHorizon;
    std::array<uint32_t, ForecastHorizonCountValue> ProjectedMineralsByHorizon;
    std::array<uint32_t, ForecastHorizonCountValue> ProjectedVespeneByHorizon;
    std::array<uint32_t, ForecastHorizonCountValue> ProjectedSupplyAvailableByHorizon;
    std::array<uint32_t, ForecastHorizonCountValue> ProjectedAvailableMineralsByHorizon;
    std::array<uint32_t, ForecastHorizonCountValue> ProjectedAvailableVespeneByHorizon;
    std::array<uint32_t, ForecastHorizonCountValue> ProjectedAvailableSupplyByHorizon;
};

}  // namespace sc2
