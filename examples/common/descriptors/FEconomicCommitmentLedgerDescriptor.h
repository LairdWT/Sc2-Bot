#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "common/economy/EconomyForecastConstants.h"
#include "common/planning/ECommandCommitmentClass.h"

namespace sc2
{

struct FEconomicCommitmentLedgerDescriptor
{
public:
    FEconomicCommitmentLedgerDescriptor();

    void Reset();
    void AddReservedResources(ECommandCommitmentClass CommandCommitmentClassValue, uint32_t MineralsCostValue,
                              uint32_t VespeneCostValue, uint32_t SupplyCostValue);
    void AddCommittedResources(ECommandCommitmentClass CommandCommitmentClassValue, uint32_t MineralsCostValue,
                               uint32_t VespeneCostValue, uint32_t SupplyCostValue);
    void AddProducerOccupancy(ECommandCommitmentClass CommandCommitmentClassValue, uint32_t TownHallCountValue,
                              uint32_t BarracksCountValue, uint32_t FactoryCountValue, uint32_t StarportCountValue);
    uint32_t GetReservedMinerals(ECommandCommitmentClass CommandCommitmentClassValue) const;
    uint32_t GetReservedVespene(ECommandCommitmentClass CommandCommitmentClassValue) const;
    uint32_t GetReservedSupply(ECommandCommitmentClass CommandCommitmentClassValue) const;
    uint32_t GetCommittedMinerals(ECommandCommitmentClass CommandCommitmentClassValue) const;
    uint32_t GetCommittedVespene(ECommandCommitmentClass CommandCommitmentClassValue) const;
    uint32_t GetCommittedSupply(ECommandCommitmentClass CommandCommitmentClassValue) const;
    uint32_t GetProjectedAvailableAfterMandatoryMinerals(size_t HorizonIndexValue) const;
    uint32_t GetProjectedAvailableAfterMandatoryVespene(size_t HorizonIndexValue) const;
    uint32_t GetProjectedAvailableAfterMandatorySupply(size_t HorizonIndexValue) const;
    uint32_t GetProjectedDiscretionaryMinerals(size_t HorizonIndexValue) const;
    uint32_t GetProjectedDiscretionaryVespene(size_t HorizonIndexValue) const;
    uint32_t GetProjectedDiscretionarySupply(size_t HorizonIndexValue) const;

public:
    std::array<uint32_t, CommandCommitmentClassCountValue> ReservedMineralsByCommitmentClass;
    std::array<uint32_t, CommandCommitmentClassCountValue> ReservedVespeneByCommitmentClass;
    std::array<uint32_t, CommandCommitmentClassCountValue> ReservedSupplyByCommitmentClass;
    std::array<uint32_t, CommandCommitmentClassCountValue> CommittedMineralsByCommitmentClass;
    std::array<uint32_t, CommandCommitmentClassCountValue> CommittedVespeneByCommitmentClass;
    std::array<uint32_t, CommandCommitmentClassCountValue> CommittedSupplyByCommitmentClass;
    std::array<uint32_t, CommandCommitmentClassCountValue> TownHallOccupancyByCommitmentClass;
    std::array<uint32_t, CommandCommitmentClassCountValue> BarracksOccupancyByCommitmentClass;
    std::array<uint32_t, CommandCommitmentClassCountValue> FactoryOccupancyByCommitmentClass;
    std::array<uint32_t, CommandCommitmentClassCountValue> StarportOccupancyByCommitmentClass;
    std::array<uint32_t, ForecastHorizonCountValue> ProjectedAvailableAfterMandatoryMineralsByHorizon;
    std::array<uint32_t, ForecastHorizonCountValue> ProjectedAvailableAfterMandatoryVespeneByHorizon;
    std::array<uint32_t, ForecastHorizonCountValue> ProjectedAvailableAfterMandatorySupplyByHorizon;
    std::array<uint32_t, ForecastHorizonCountValue> ProjectedDiscretionaryMineralsByHorizon;
    std::array<uint32_t, ForecastHorizonCountValue> ProjectedDiscretionaryVespeneByHorizon;
    std::array<uint32_t, ForecastHorizonCountValue> ProjectedDiscretionarySupplyByHorizon;
};

}  // namespace sc2
