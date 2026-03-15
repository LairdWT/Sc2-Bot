#include "common/descriptors/FEconomicCommitmentLedgerDescriptor.h"

namespace sc2
{

FEconomicCommitmentLedgerDescriptor::FEconomicCommitmentLedgerDescriptor()
{
    Reset();
}

void FEconomicCommitmentLedgerDescriptor::Reset()
{
    ReservedMineralsByCommitmentClass.fill(0U);
    ReservedVespeneByCommitmentClass.fill(0U);
    ReservedSupplyByCommitmentClass.fill(0U);
    CommittedMineralsByCommitmentClass.fill(0U);
    CommittedVespeneByCommitmentClass.fill(0U);
    CommittedSupplyByCommitmentClass.fill(0U);
    TownHallOccupancyByCommitmentClass.fill(0U);
    BarracksOccupancyByCommitmentClass.fill(0U);
    FactoryOccupancyByCommitmentClass.fill(0U);
    StarportOccupancyByCommitmentClass.fill(0U);
    ProjectedAvailableAfterMandatoryMineralsByHorizon.fill(0U);
    ProjectedAvailableAfterMandatoryVespeneByHorizon.fill(0U);
    ProjectedAvailableAfterMandatorySupplyByHorizon.fill(0U);
    ProjectedDiscretionaryMineralsByHorizon.fill(0U);
    ProjectedDiscretionaryVespeneByHorizon.fill(0U);
    ProjectedDiscretionarySupplyByHorizon.fill(0U);
}

void FEconomicCommitmentLedgerDescriptor::AddReservedResources(
    const ECommandCommitmentClass CommandCommitmentClassValue, const uint32_t MineralsCostValue,
    const uint32_t VespeneCostValue, const uint32_t SupplyCostValue)
{
    const size_t CommitmentClassIndexValue = GetCommandCommitmentClassIndex(CommandCommitmentClassValue);
    ReservedMineralsByCommitmentClass[CommitmentClassIndexValue] += MineralsCostValue;
    ReservedVespeneByCommitmentClass[CommitmentClassIndexValue] += VespeneCostValue;
    ReservedSupplyByCommitmentClass[CommitmentClassIndexValue] += SupplyCostValue;
}

void FEconomicCommitmentLedgerDescriptor::AddCommittedResources(
    const ECommandCommitmentClass CommandCommitmentClassValue, const uint32_t MineralsCostValue,
    const uint32_t VespeneCostValue, const uint32_t SupplyCostValue)
{
    const size_t CommitmentClassIndexValue = GetCommandCommitmentClassIndex(CommandCommitmentClassValue);
    CommittedMineralsByCommitmentClass[CommitmentClassIndexValue] += MineralsCostValue;
    CommittedVespeneByCommitmentClass[CommitmentClassIndexValue] += VespeneCostValue;
    CommittedSupplyByCommitmentClass[CommitmentClassIndexValue] += SupplyCostValue;
}

void FEconomicCommitmentLedgerDescriptor::AddProducerOccupancy(
    const ECommandCommitmentClass CommandCommitmentClassValue, const uint32_t TownHallCountValue,
    const uint32_t BarracksCountValue, const uint32_t FactoryCountValue, const uint32_t StarportCountValue)
{
    const size_t CommitmentClassIndexValue = GetCommandCommitmentClassIndex(CommandCommitmentClassValue);
    TownHallOccupancyByCommitmentClass[CommitmentClassIndexValue] += TownHallCountValue;
    BarracksOccupancyByCommitmentClass[CommitmentClassIndexValue] += BarracksCountValue;
    FactoryOccupancyByCommitmentClass[CommitmentClassIndexValue] += FactoryCountValue;
    StarportOccupancyByCommitmentClass[CommitmentClassIndexValue] += StarportCountValue;
}

uint32_t FEconomicCommitmentLedgerDescriptor::GetReservedMinerals(
    const ECommandCommitmentClass CommandCommitmentClassValue) const
{
    return ReservedMineralsByCommitmentClass[GetCommandCommitmentClassIndex(CommandCommitmentClassValue)];
}

uint32_t FEconomicCommitmentLedgerDescriptor::GetReservedVespene(
    const ECommandCommitmentClass CommandCommitmentClassValue) const
{
    return ReservedVespeneByCommitmentClass[GetCommandCommitmentClassIndex(CommandCommitmentClassValue)];
}

uint32_t FEconomicCommitmentLedgerDescriptor::GetReservedSupply(
    const ECommandCommitmentClass CommandCommitmentClassValue) const
{
    return ReservedSupplyByCommitmentClass[GetCommandCommitmentClassIndex(CommandCommitmentClassValue)];
}

uint32_t FEconomicCommitmentLedgerDescriptor::GetCommittedMinerals(
    const ECommandCommitmentClass CommandCommitmentClassValue) const
{
    return CommittedMineralsByCommitmentClass[GetCommandCommitmentClassIndex(CommandCommitmentClassValue)];
}

uint32_t FEconomicCommitmentLedgerDescriptor::GetCommittedVespene(
    const ECommandCommitmentClass CommandCommitmentClassValue) const
{
    return CommittedVespeneByCommitmentClass[GetCommandCommitmentClassIndex(CommandCommitmentClassValue)];
}

uint32_t FEconomicCommitmentLedgerDescriptor::GetCommittedSupply(
    const ECommandCommitmentClass CommandCommitmentClassValue) const
{
    return CommittedSupplyByCommitmentClass[GetCommandCommitmentClassIndex(CommandCommitmentClassValue)];
}

uint32_t FEconomicCommitmentLedgerDescriptor::GetProjectedAvailableAfterMandatoryMinerals(
    const size_t HorizonIndexValue) const
{
    return HorizonIndexValue < ForecastHorizonCountValue
               ? ProjectedAvailableAfterMandatoryMineralsByHorizon[HorizonIndexValue]
               : 0U;
}

uint32_t FEconomicCommitmentLedgerDescriptor::GetProjectedAvailableAfterMandatoryVespene(
    const size_t HorizonIndexValue) const
{
    return HorizonIndexValue < ForecastHorizonCountValue
               ? ProjectedAvailableAfterMandatoryVespeneByHorizon[HorizonIndexValue]
               : 0U;
}

uint32_t FEconomicCommitmentLedgerDescriptor::GetProjectedAvailableAfterMandatorySupply(
    const size_t HorizonIndexValue) const
{
    return HorizonIndexValue < ForecastHorizonCountValue
               ? ProjectedAvailableAfterMandatorySupplyByHorizon[HorizonIndexValue]
               : 0U;
}

uint32_t FEconomicCommitmentLedgerDescriptor::GetProjectedDiscretionaryMinerals(
    const size_t HorizonIndexValue) const
{
    return HorizonIndexValue < ForecastHorizonCountValue ? ProjectedDiscretionaryMineralsByHorizon[HorizonIndexValue]
                                                         : 0U;
}

uint32_t FEconomicCommitmentLedgerDescriptor::GetProjectedDiscretionaryVespene(
    const size_t HorizonIndexValue) const
{
    return HorizonIndexValue < ForecastHorizonCountValue ? ProjectedDiscretionaryVespeneByHorizon[HorizonIndexValue]
                                                         : 0U;
}

uint32_t FEconomicCommitmentLedgerDescriptor::GetProjectedDiscretionarySupply(
    const size_t HorizonIndexValue) const
{
    return HorizonIndexValue < ForecastHorizonCountValue ? ProjectedDiscretionarySupplyByHorizon[HorizonIndexValue]
                                                         : 0U;
}

}  // namespace sc2
