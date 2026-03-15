#include "common/descriptors/FEconomyStateDescriptor.h"

namespace sc2
{

FEconomyStateDescriptor::FEconomyStateDescriptor()
{
    Reset();
}

void FEconomyStateDescriptor::Reset()
{
    CurrentMinerals = 0U;
    CurrentVespene = 0U;
    CurrentSupplyUsed = 0U;
    CurrentSupplyCap = 0U;
    CurrentSupplyAvailable = 0U;
    BudgetedMinerals = 0U;
    BudgetedVespene = 0U;
    BudgetedSupplyAvailable = 0U;
    ReservedMinerals = 0U;
    ReservedVespene = 0U;
    ReservedSupply = 0U;
    CommittedMinerals = 0U;
    CommittedVespene = 0U;
    CommittedSupply = 0U;
    HorizonGameLoops = ForecastHorizonGameLoopsValue;
    GrossMineralIncomeByHorizon.fill(0U);
    GrossVespeneIncomeByHorizon.fill(0U);
    NetMineralDeltaByHorizon.fill(0);
    NetVespeneDeltaByHorizon.fill(0);
    GrossMineralIncomeAverageByHorizon.fill(0.0f);
    GrossVespeneIncomeAverageByHorizon.fill(0.0f);
    NetMineralDeltaAverageByHorizon.fill(0.0f);
    NetVespeneDeltaAverageByHorizon.fill(0.0f);
    ProjectedMineralsByHorizon.fill(0U);
    ProjectedVespeneByHorizon.fill(0U);
    ProjectedSupplyAvailableByHorizon.fill(0U);
    ProjectedAvailableMineralsByHorizon.fill(0U);
    ProjectedAvailableVespeneByHorizon.fill(0U);
    ProjectedAvailableSupplyByHorizon.fill(0U);
}

uint32_t FEconomyStateDescriptor::GetProjectedMineralsAtHorizon(const size_t HorizonIndexValue) const
{
    return HorizonIndexValue < ForecastHorizonCountValue ? ProjectedMineralsByHorizon[HorizonIndexValue] : 0U;
}

uint32_t FEconomyStateDescriptor::GetProjectedVespeneAtHorizon(const size_t HorizonIndexValue) const
{
    return HorizonIndexValue < ForecastHorizonCountValue ? ProjectedVespeneByHorizon[HorizonIndexValue] : 0U;
}

uint32_t FEconomyStateDescriptor::GetProjectedSupplyAvailableAtHorizon(const size_t HorizonIndexValue) const
{
    return HorizonIndexValue < ForecastHorizonCountValue ? ProjectedSupplyAvailableByHorizon[HorizonIndexValue] : 0U;
}

uint32_t FEconomyStateDescriptor::GetProjectedAvailableMineralsAtHorizon(const size_t HorizonIndexValue) const
{
    return HorizonIndexValue < ForecastHorizonCountValue ? ProjectedAvailableMineralsByHorizon[HorizonIndexValue]
                                                         : 0U;
}

uint32_t FEconomyStateDescriptor::GetProjectedAvailableVespeneAtHorizon(const size_t HorizonIndexValue) const
{
    return HorizonIndexValue < ForecastHorizonCountValue ? ProjectedAvailableVespeneByHorizon[HorizonIndexValue]
                                                         : 0U;
}

uint32_t FEconomyStateDescriptor::GetProjectedAvailableSupplyAtHorizon(const size_t HorizonIndexValue) const
{
    return HorizonIndexValue < ForecastHorizonCountValue ? ProjectedAvailableSupplyByHorizon[HorizonIndexValue] : 0U;
}

}  // namespace sc2
