#include "common/descriptors/FEconomyStateDescriptor.h"

namespace sc2
{
namespace
{

uint32_t SubtractBudgetFloor(const uint32_t TotalValue, const uint32_t ConsumedValue)
{
    return TotalValue > ConsumedValue ? TotalValue - ConsumedValue : 0U;
}

}  // namespace

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

uint32_t FEconomyStateDescriptor::GetCurrentSpendableMineralsAfterMandatory(
    const FEconomicCommitmentLedgerDescriptor& EconomicCommitmentLedgerDescriptorValue) const
{
    return SubtractBudgetFloor(CurrentMinerals,
                               EconomicCommitmentLedgerDescriptorValue.GetReservedMinerals(
                                   ECommandCommitmentClass::MandatoryOpening) +
                                   EconomicCommitmentLedgerDescriptorValue.GetReservedMinerals(
                                       ECommandCommitmentClass::MandatoryRecovery) +
                                   EconomicCommitmentLedgerDescriptorValue.GetCommittedMinerals(
                                       ECommandCommitmentClass::MandatoryOpening) +
                                   EconomicCommitmentLedgerDescriptorValue.GetCommittedMinerals(
                                       ECommandCommitmentClass::MandatoryRecovery));
}

uint32_t FEconomyStateDescriptor::GetCurrentSpendableVespeneAfterMandatory(
    const FEconomicCommitmentLedgerDescriptor& EconomicCommitmentLedgerDescriptorValue) const
{
    return SubtractBudgetFloor(CurrentVespene,
                               EconomicCommitmentLedgerDescriptorValue.GetReservedVespene(
                                   ECommandCommitmentClass::MandatoryOpening) +
                                   EconomicCommitmentLedgerDescriptorValue.GetReservedVespene(
                                       ECommandCommitmentClass::MandatoryRecovery) +
                                   EconomicCommitmentLedgerDescriptorValue.GetCommittedVespene(
                                       ECommandCommitmentClass::MandatoryOpening) +
                                   EconomicCommitmentLedgerDescriptorValue.GetCommittedVespene(
                                       ECommandCommitmentClass::MandatoryRecovery));
}

uint32_t FEconomyStateDescriptor::GetCurrentSpendableSupplyAfterMandatory(
    const FEconomicCommitmentLedgerDescriptor& EconomicCommitmentLedgerDescriptorValue) const
{
    return SubtractBudgetFloor(CurrentSupplyAvailable,
                               EconomicCommitmentLedgerDescriptorValue.GetReservedSupply(
                                   ECommandCommitmentClass::MandatoryOpening) +
                                   EconomicCommitmentLedgerDescriptorValue.GetReservedSupply(
                                       ECommandCommitmentClass::MandatoryRecovery) +
                                   EconomicCommitmentLedgerDescriptorValue.GetCommittedSupply(
                                       ECommandCommitmentClass::MandatoryOpening) +
                                   EconomicCommitmentLedgerDescriptorValue.GetCommittedSupply(
                                       ECommandCommitmentClass::MandatoryRecovery));
}

uint32_t FEconomyStateDescriptor::GetCurrentDiscretionaryMinerals(
    const FEconomicCommitmentLedgerDescriptor& EconomicCommitmentLedgerDescriptorValue) const
{
    return SubtractBudgetFloor(
        GetCurrentSpendableMineralsAfterMandatory(EconomicCommitmentLedgerDescriptorValue),
        EconomicCommitmentLedgerDescriptorValue.GetReservedMinerals(ECommandCommitmentClass::FlexibleMacro) +
            EconomicCommitmentLedgerDescriptorValue.GetReservedMinerals(ECommandCommitmentClass::Opportunistic) +
            EconomicCommitmentLedgerDescriptorValue.GetCommittedMinerals(ECommandCommitmentClass::FlexibleMacro) +
            EconomicCommitmentLedgerDescriptorValue.GetCommittedMinerals(ECommandCommitmentClass::Opportunistic));
}

uint32_t FEconomyStateDescriptor::GetCurrentDiscretionaryVespene(
    const FEconomicCommitmentLedgerDescriptor& EconomicCommitmentLedgerDescriptorValue) const
{
    return SubtractBudgetFloor(
        GetCurrentSpendableVespeneAfterMandatory(EconomicCommitmentLedgerDescriptorValue),
        EconomicCommitmentLedgerDescriptorValue.GetReservedVespene(ECommandCommitmentClass::FlexibleMacro) +
            EconomicCommitmentLedgerDescriptorValue.GetReservedVespene(ECommandCommitmentClass::Opportunistic) +
            EconomicCommitmentLedgerDescriptorValue.GetCommittedVespene(ECommandCommitmentClass::FlexibleMacro) +
            EconomicCommitmentLedgerDescriptorValue.GetCommittedVespene(ECommandCommitmentClass::Opportunistic));
}

uint32_t FEconomyStateDescriptor::GetCurrentDiscretionarySupply(
    const FEconomicCommitmentLedgerDescriptor& EconomicCommitmentLedgerDescriptorValue) const
{
    return SubtractBudgetFloor(
        GetCurrentSpendableSupplyAfterMandatory(EconomicCommitmentLedgerDescriptorValue),
        EconomicCommitmentLedgerDescriptorValue.GetReservedSupply(ECommandCommitmentClass::FlexibleMacro) +
            EconomicCommitmentLedgerDescriptorValue.GetReservedSupply(ECommandCommitmentClass::Opportunistic) +
            EconomicCommitmentLedgerDescriptorValue.GetCommittedSupply(ECommandCommitmentClass::FlexibleMacro) +
            EconomicCommitmentLedgerDescriptorValue.GetCommittedSupply(ECommandCommitmentClass::Opportunistic));
}

uint32_t FEconomyStateDescriptor::GetProjectedDiscretionaryMineralsAtHorizon(
    const FEconomicCommitmentLedgerDescriptor& EconomicCommitmentLedgerDescriptorValue,
    const size_t HorizonIndexValue) const
{
    return EconomicCommitmentLedgerDescriptorValue.GetProjectedDiscretionaryMinerals(HorizonIndexValue);
}

uint32_t FEconomyStateDescriptor::GetProjectedDiscretionaryVespeneAtHorizon(
    const FEconomicCommitmentLedgerDescriptor& EconomicCommitmentLedgerDescriptorValue,
    const size_t HorizonIndexValue) const
{
    return EconomicCommitmentLedgerDescriptorValue.GetProjectedDiscretionaryVespene(HorizonIndexValue);
}

uint32_t FEconomyStateDescriptor::GetProjectedDiscretionarySupplyAtHorizon(
    const FEconomicCommitmentLedgerDescriptor& EconomicCommitmentLedgerDescriptorValue,
    const size_t HorizonIndexValue) const
{
    return EconomicCommitmentLedgerDescriptorValue.GetProjectedDiscretionarySupply(HorizonIndexValue);
}

}  // namespace sc2
