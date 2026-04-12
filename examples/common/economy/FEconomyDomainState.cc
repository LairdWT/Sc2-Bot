#include "common/economy/FEconomyDomainState.h"

#include <algorithm>
#include <string>

#include "common/bot_status_models.h"
#include "common/economic_models.h"
#include "common/logging.h"

namespace sc2
{
namespace
{

std::array<uint16_t, NUM_TERRAN_UPGRADES> GetObservedActiveUpgradeCounts(const FAgentState& AgentStateValue)
{
    std::array<uint16_t, NUM_TERRAN_UPGRADES> ObservedActiveUpgradeCountsValue;
    ObservedActiveUpgradeCountsValue.fill(0U);

    for (const Unit* ControlledUnitValue : AgentStateValue.UnitContainer.ControlledUnits)
    {
        if (ControlledUnitValue == nullptr || !ControlledUnitValue->is_building)
        {
            continue;
        }

        for (const UnitOrder& UnitOrderValue : ControlledUnitValue->orders)
        {
            size_t UpgradeTypeIndexValue = INVALID_TERRAN_UPGRADE_TYPE_INDEX;
            const bool IsTrackedUpgradeValue =
                TryGetTerranUpgradeTypeIndex(UnitOrderValue.ability_id, UpgradeTypeIndexValue);
            if (!IsTrackedUpgradeValue || !IsTerranUpgradeTypeIndexValid(UpgradeTypeIndexValue))
            {
                continue;
            }

            ++ObservedActiveUpgradeCountsValue[UpgradeTypeIndexValue];
        }
    }

    return ObservedActiveUpgradeCountsValue;
}

uint64_t GetConfirmedMineralSpend(const FAgentState& AgentStateValue,
                                  const std::array<uint16_t, NUM_TERRAN_UNITS>& LastObservedUnitsInConstructionValue,
                                  const std::array<uint16_t, NUM_TERRAN_BUILDINGS>&
                                      LastObservedBuildingsInConstructionValue)
{
    uint64_t ConfirmedMineralSpendValue = 0U;

    for (const UNIT_TYPEID UnitTypeIdValue : TERRAN_UNIT_TYPES)
    {
        const size_t UnitTypeIndexValue = GetTerranUnitTypeIndex(UnitTypeIdValue);
        if (!IsTerranUnitTypeIndexValid(UnitTypeIndexValue))
        {
            continue;
        }

        const uint16_t CurrentInConstructionCountValue = AgentStateValue.Units.UnitsInConstruction[UnitTypeIndexValue];
        const uint16_t LastInConstructionCountValue = LastObservedUnitsInConstructionValue[UnitTypeIndexValue];
        if (CurrentInConstructionCountValue <= LastInConstructionCountValue)
        {
            continue;
        }

        const uint16_t StartedCountValue = CurrentInConstructionCountValue - LastInConstructionCountValue;
        const FUnitCostData& UnitCostDataValue = TERRAN_ECONOMIC_DATA.GetUnitCostData(UnitTypeIdValue);
        ConfirmedMineralSpendValue +=
            static_cast<uint64_t>(StartedCountValue) * static_cast<uint64_t>(UnitCostDataValue.CostData.Minerals);
    }

    for (const UNIT_TYPEID BuildingTypeIdValue : TERRAN_BUILDING_TYPES)
    {
        const size_t BuildingTypeIndexValue = GetTerranBuildingTypeIndex(BuildingTypeIdValue);
        if (!IsTerranBuildingTypeIndexValid(BuildingTypeIndexValue))
        {
            continue;
        }

        const uint16_t CurrentInConstructionCountValue =
            AgentStateValue.Buildings.CurrentlyInConstruction[BuildingTypeIndexValue];
        const uint16_t LastInConstructionCountValue = LastObservedBuildingsInConstructionValue[BuildingTypeIndexValue];
        if (CurrentInConstructionCountValue <= LastInConstructionCountValue)
        {
            continue;
        }

        const uint16_t StartedCountValue = CurrentInConstructionCountValue - LastInConstructionCountValue;
        const FBuildingCostData& BuildingCostDataValue = TERRAN_ECONOMIC_DATA.GetBuildingCostData(BuildingTypeIdValue);
        ConfirmedMineralSpendValue +=
            static_cast<uint64_t>(StartedCountValue) * static_cast<uint64_t>(BuildingCostDataValue.CostData.Minerals);
    }

    return ConfirmedMineralSpendValue;
}

uint64_t GetConfirmedVespeneSpend(const FAgentState& AgentStateValue,
                                  const std::array<uint16_t, NUM_TERRAN_UNITS>& LastObservedUnitsInConstructionValue,
                                  const std::array<uint16_t, NUM_TERRAN_BUILDINGS>&
                                      LastObservedBuildingsInConstructionValue)
{
    uint64_t ConfirmedVespeneSpendValue = 0U;

    for (const UNIT_TYPEID UnitTypeIdValue : TERRAN_UNIT_TYPES)
    {
        const size_t UnitTypeIndexValue = GetTerranUnitTypeIndex(UnitTypeIdValue);
        if (!IsTerranUnitTypeIndexValid(UnitTypeIndexValue))
        {
            continue;
        }

        const uint16_t CurrentInConstructionCountValue = AgentStateValue.Units.UnitsInConstruction[UnitTypeIndexValue];
        const uint16_t LastInConstructionCountValue = LastObservedUnitsInConstructionValue[UnitTypeIndexValue];
        if (CurrentInConstructionCountValue <= LastInConstructionCountValue)
        {
            continue;
        }

        const uint16_t StartedCountValue = CurrentInConstructionCountValue - LastInConstructionCountValue;
        const FUnitCostData& UnitCostDataValue = TERRAN_ECONOMIC_DATA.GetUnitCostData(UnitTypeIdValue);
        ConfirmedVespeneSpendValue +=
            static_cast<uint64_t>(StartedCountValue) * static_cast<uint64_t>(UnitCostDataValue.CostData.Vespine);
    }

    for (const UNIT_TYPEID BuildingTypeIdValue : TERRAN_BUILDING_TYPES)
    {
        const size_t BuildingTypeIndexValue = GetTerranBuildingTypeIndex(BuildingTypeIdValue);
        if (!IsTerranBuildingTypeIndexValid(BuildingTypeIndexValue))
        {
            continue;
        }

        const uint16_t CurrentInConstructionCountValue =
            AgentStateValue.Buildings.CurrentlyInConstruction[BuildingTypeIndexValue];
        const uint16_t LastInConstructionCountValue = LastObservedBuildingsInConstructionValue[BuildingTypeIndexValue];
        if (CurrentInConstructionCountValue <= LastInConstructionCountValue)
        {
            continue;
        }

        const uint16_t StartedCountValue = CurrentInConstructionCountValue - LastInConstructionCountValue;
        const FBuildingCostData& BuildingCostDataValue = TERRAN_ECONOMIC_DATA.GetBuildingCostData(BuildingTypeIdValue);
        ConfirmedVespeneSpendValue +=
            static_cast<uint64_t>(StartedCountValue) * static_cast<uint64_t>(BuildingCostDataValue.CostData.Vespine);
    }

    return ConfirmedVespeneSpendValue;
}

void AccumulateConfirmedUpgradeSpendAndRefund(
    const std::array<uint16_t, NUM_TERRAN_UPGRADES>& CurrentObservedActiveUpgradeCountsValue,
    const std::array<uint16_t, NUM_TERRAN_UPGRADES>& LastObservedActiveUpgradeCountsValue,
    const std::array<uint8_t, NUM_TERRAN_UPGRADES>& CurrentObservedCompletedUpgradeCountsValue,
    const std::array<uint8_t, NUM_TERRAN_UPGRADES>& LastObservedCompletedUpgradeCountsValue,
    uint64_t& ConfirmedMineralSpendValue, uint64_t& ConfirmedVespeneSpendValue,
    uint64_t& ConfirmedMineralRefundValue, uint64_t& ConfirmedVespeneRefundValue)
{
    for (const ABILITY_ID UpgradeTypeValue : TERRAN_RESEARCH_UPGRADE_TYPES)
    {
        const size_t UpgradeTypeIndexValue = GetTerranUpgradeTypeIndex(UpgradeTypeValue);
        if (!IsTerranUpgradeTypeIndexValid(UpgradeTypeIndexValue))
        {
            continue;
        }

        const uint16_t CurrentActiveUpgradeCountValue = CurrentObservedActiveUpgradeCountsValue[UpgradeTypeIndexValue];
        const uint16_t LastActiveUpgradeCountValue = LastObservedActiveUpgradeCountsValue[UpgradeTypeIndexValue];
        const uint8_t CurrentCompletedUpgradeCountValue =
            CurrentObservedCompletedUpgradeCountsValue[UpgradeTypeIndexValue];
        const uint8_t LastCompletedUpgradeCountValue =
            LastObservedCompletedUpgradeCountsValue[UpgradeTypeIndexValue];
        const FUpgradeCostData& UpgradeCostDataValue = TERRAN_ECONOMIC_DATA.GetUpgradeCostData(UpgradeTypeValue);

        if (CurrentActiveUpgradeCountValue > LastActiveUpgradeCountValue)
        {
            const uint16_t StartedUpgradeCountValue = CurrentActiveUpgradeCountValue - LastActiveUpgradeCountValue;
            ConfirmedMineralSpendValue += static_cast<uint64_t>(StartedUpgradeCountValue) *
                                          static_cast<uint64_t>(UpgradeCostDataValue.CostData.Minerals);
            ConfirmedVespeneSpendValue += static_cast<uint64_t>(StartedUpgradeCountValue) *
                                          static_cast<uint64_t>(UpgradeCostDataValue.CostData.Vespine);
        }

        const uint16_t CompletedUpgradeDeltaValue =
            CurrentCompletedUpgradeCountValue > LastCompletedUpgradeCountValue
                ? static_cast<uint16_t>(CurrentCompletedUpgradeCountValue - LastCompletedUpgradeCountValue)
                : 0U;
        const uint16_t ResolvedActiveUpgradeCountValue =
            CurrentActiveUpgradeCountValue + CompletedUpgradeDeltaValue;
        if (ResolvedActiveUpgradeCountValue >= LastActiveUpgradeCountValue)
        {
            continue;
        }

        const uint16_t RefundedUpgradeCountValue = LastActiveUpgradeCountValue - ResolvedActiveUpgradeCountValue;
        ConfirmedMineralRefundValue += static_cast<uint64_t>(RefundedUpgradeCountValue) *
                                       static_cast<uint64_t>(UpgradeCostDataValue.CostData.Minerals);
        ConfirmedVespeneRefundValue += static_cast<uint64_t>(RefundedUpgradeCountValue) *
                                       static_cast<uint64_t>(UpgradeCostDataValue.CostData.Vespine);
    }
}

}  // namespace

FEconomyDomainState::FEconomyDomainState()
{
    Reset();
}

void FEconomyDomainState::Reset()
{
    CurrentGameLoop = 0U;
    bHasObservedState = false;
    LastObservedMinerals = 0U;
    LastObservedVespene = 0U;
    LastObservedUnitCounts.fill(0U);
    LastObservedUnitsInConstruction.fill(0U);
    LastObservedBuildingCounts.fill(0U);
    LastObservedBuildingsInConstruction.fill(0U);
    LastObservedActiveUpgradeCounts.fill(0U);
    LastObservedCompletedUpgradeCounts.fill(0U);
    CumulativeGrossMineralIncome = 0U;
    CumulativeGrossVespeneIncome = 0U;
    CumulativeUnitCompletionCounts.fill(0U);
    CumulativeBuildingCompletionCounts.fill(0U);
    SampleGameLoops.clear();
    SampleMineralBanks.clear();
    SampleVespeneBanks.clear();
    SampleCumulativeGrossMineralIncome.clear();
    SampleCumulativeGrossVespeneIncome.clear();
    SampleCumulativeUnitCompletionCounts.clear();
    SampleCumulativeBuildingCompletionCounts.clear();
}

void FEconomyDomainState::Update(const FAgentState& AgentStateValue, const uint64_t CurrentGameLoopValue)
{
    CurrentGameLoop = CurrentGameLoopValue;

    const uint32_t CurrentMineralsValue = AgentStateValue.Economy.Minerals;
    const uint32_t CurrentVespeneValue = AgentStateValue.Economy.Vespene;
    const std::array<uint16_t, NUM_TERRAN_UPGRADES> CurrentObservedActiveUpgradeCountsValue =
        GetObservedActiveUpgradeCounts(AgentStateValue);
    if (!bHasObservedState)
    {
        bHasObservedState = true;
        LastObservedMinerals = CurrentMineralsValue;
        LastObservedVespene = CurrentVespeneValue;
        LastObservedUnitCounts = AgentStateValue.Units.UnitCounts;
        LastObservedUnitsInConstruction = AgentStateValue.Units.UnitsInConstruction;
        LastObservedBuildingCounts = AgentStateValue.Buildings.BuildingCounts;
        LastObservedBuildingsInConstruction = AgentStateValue.Buildings.CurrentlyInConstruction;
        LastObservedActiveUpgradeCounts = CurrentObservedActiveUpgradeCountsValue;
        LastObservedCompletedUpgradeCounts = AgentStateValue.CompletedUpgradeCounts;
        RecordCurrentSample(CurrentMineralsValue, CurrentVespeneValue);
        return;
    }

    uint64_t ConfirmedMineralSpendValue =
        GetConfirmedMineralSpend(AgentStateValue, LastObservedUnitsInConstruction, LastObservedBuildingsInConstruction);
    uint64_t ConfirmedVespeneSpendValue =
        GetConfirmedVespeneSpend(AgentStateValue, LastObservedUnitsInConstruction, LastObservedBuildingsInConstruction);
    uint64_t ConfirmedMineralRefundValue = 0U;
    uint64_t ConfirmedVespeneRefundValue = 0U;
    AccumulateConfirmedUpgradeSpendAndRefund(CurrentObservedActiveUpgradeCountsValue, LastObservedActiveUpgradeCounts,
                                             AgentStateValue.CompletedUpgradeCounts,
                                             LastObservedCompletedUpgradeCounts, ConfirmedMineralSpendValue,
                                             ConfirmedVespeneSpendValue, ConfirmedMineralRefundValue,
                                             ConfirmedVespeneRefundValue);

    const int64_t MineralBankDeltaValue =
        static_cast<int64_t>(CurrentMineralsValue) - static_cast<int64_t>(LastObservedMinerals);
    const int64_t VespeneBankDeltaValue =
        static_cast<int64_t>(CurrentVespeneValue) - static_cast<int64_t>(LastObservedVespene);

    const int64_t GrossMineralIncomeValue =
        MineralBankDeltaValue + static_cast<int64_t>(ConfirmedMineralSpendValue) -
        static_cast<int64_t>(ConfirmedMineralRefundValue);
    const int64_t GrossVespeneIncomeValue =
        VespeneBankDeltaValue + static_cast<int64_t>(ConfirmedVespeneSpendValue) -
        static_cast<int64_t>(ConfirmedVespeneRefundValue);

    if (GrossMineralIncomeValue > 0)
    {
        CumulativeGrossMineralIncome += static_cast<uint64_t>(GrossMineralIncomeValue);
    }
    if (GrossVespeneIncomeValue > 0)
    {
        CumulativeGrossVespeneIncome += static_cast<uint64_t>(GrossVespeneIncomeValue);
    }

    for (const UNIT_TYPEID UnitTypeIdValue : TERRAN_UNIT_TYPES)
    {
        const size_t UnitTypeIndexValue = GetTerranUnitTypeIndex(UnitTypeIdValue);
        if (!IsTerranUnitTypeIndexValid(UnitTypeIndexValue))
        {
            continue;
        }

        const uint16_t CurrentObservedCountValue = AgentStateValue.Units.UnitCounts[UnitTypeIndexValue];
        const uint16_t LastObservedCountValue = LastObservedUnitCounts[UnitTypeIndexValue];
        if (CurrentObservedCountValue > LastObservedCountValue)
        {
            CumulativeUnitCompletionCounts[UnitTypeIndexValue] +=
                static_cast<uint64_t>(CurrentObservedCountValue - LastObservedCountValue);
        }
    }

    for (const UNIT_TYPEID BuildingTypeIdValue : TERRAN_BUILDING_TYPES)
    {
        const size_t BuildingTypeIndexValue = GetTerranBuildingTypeIndex(BuildingTypeIdValue);
        if (!IsTerranBuildingTypeIndexValid(BuildingTypeIndexValue))
        {
            continue;
        }

        const uint16_t CurrentObservedCountValue = AgentStateValue.Buildings.BuildingCounts[BuildingTypeIndexValue];
        const uint16_t LastObservedCountValue = LastObservedBuildingCounts[BuildingTypeIndexValue];
        if (CurrentObservedCountValue > LastObservedCountValue)
        {
            CumulativeBuildingCompletionCounts[BuildingTypeIndexValue] +=
                static_cast<uint64_t>(CurrentObservedCountValue - LastObservedCountValue);
        }
    }

    LastObservedMinerals = CurrentMineralsValue;
    LastObservedVespene = CurrentVespeneValue;
    LastObservedUnitCounts = AgentStateValue.Units.UnitCounts;
    LastObservedUnitsInConstruction = AgentStateValue.Units.UnitsInConstruction;
    LastObservedBuildingCounts = AgentStateValue.Buildings.BuildingCounts;
    LastObservedBuildingsInConstruction = AgentStateValue.Buildings.CurrentlyInConstruction;
    LastObservedActiveUpgradeCounts = CurrentObservedActiveUpgradeCountsValue;
    LastObservedCompletedUpgradeCounts = AgentStateValue.CompletedUpgradeCounts;

    RecordCurrentSample(CurrentMineralsValue, CurrentVespeneValue);
    TrimHistory();
}

size_t FEconomyDomainState::GetSampleCount() const
{
    return SampleGameLoops.size();
}

bool FEconomyDomainState::HasSynchronizedSampleSizes() const
{
    const size_t ExpectedSizeValue = SampleGameLoops.size();
    return SampleMineralBanks.size() == ExpectedSizeValue &&
           SampleVespeneBanks.size() == ExpectedSizeValue &&
           SampleCumulativeGrossMineralIncome.size() == ExpectedSizeValue &&
           SampleCumulativeGrossVespeneIncome.size() == ExpectedSizeValue &&
           SampleCumulativeUnitCompletionCounts.size() == ExpectedSizeValue &&
           SampleCumulativeBuildingCompletionCounts.size() == ExpectedSizeValue;
}

void FEconomyDomainState::AssertSynchronizedSampleSizes() const
{
    if (!HasSynchronizedSampleSizes())
    {
        SCLOG(LoggingVerbosity::error,
              "INVARIANT VIOLATION: FEconomyDomainState sample vector sizes desynchronized at SampleCount=" +
                  std::to_string(SampleGameLoops.size()));
    }
}

uint64_t FEconomyDomainState::GetElapsedGameLoopsForHorizon(const size_t HorizonIndexValue) const
{
    if (HorizonIndexValue >= ForecastHorizonCountValue || SampleGameLoops.empty())
    {
        return 0U;
    }

    const size_t SampleIndexValue = GetSampleIndexForHorizon(HorizonIndexValue);
    return CurrentGameLoop >= SampleGameLoops[SampleIndexValue] ? CurrentGameLoop - SampleGameLoops[SampleIndexValue]
                                                                : 0U;
}

uint64_t FEconomyDomainState::GetGrossMineralIncomeForHorizon(const size_t HorizonIndexValue) const
{
    if (HorizonIndexValue >= ForecastHorizonCountValue || SampleGameLoops.empty())
    {
        return 0U;
    }

    const size_t SampleIndexValue = GetSampleIndexForHorizon(HorizonIndexValue);
    return CumulativeGrossMineralIncome >= SampleCumulativeGrossMineralIncome[SampleIndexValue]
               ? CumulativeGrossMineralIncome - SampleCumulativeGrossMineralIncome[SampleIndexValue]
               : 0U;
}

uint64_t FEconomyDomainState::GetGrossVespeneIncomeForHorizon(const size_t HorizonIndexValue) const
{
    if (HorizonIndexValue >= ForecastHorizonCountValue || SampleGameLoops.empty())
    {
        return 0U;
    }

    const size_t SampleIndexValue = GetSampleIndexForHorizon(HorizonIndexValue);
    return CumulativeGrossVespeneIncome >= SampleCumulativeGrossVespeneIncome[SampleIndexValue]
               ? CumulativeGrossVespeneIncome - SampleCumulativeGrossVespeneIncome[SampleIndexValue]
               : 0U;
}

int64_t FEconomyDomainState::GetNetMineralDeltaForHorizon(const size_t HorizonIndexValue) const
{
    if (HorizonIndexValue >= ForecastHorizonCountValue || SampleGameLoops.empty())
    {
        return 0;
    }

    const size_t SampleIndexValue = GetSampleIndexForHorizon(HorizonIndexValue);
    return static_cast<int64_t>(LastObservedMinerals) - static_cast<int64_t>(SampleMineralBanks[SampleIndexValue]);
}

int64_t FEconomyDomainState::GetNetVespeneDeltaForHorizon(const size_t HorizonIndexValue) const
{
    if (HorizonIndexValue >= ForecastHorizonCountValue || SampleGameLoops.empty())
    {
        return 0;
    }

    const size_t SampleIndexValue = GetSampleIndexForHorizon(HorizonIndexValue);
    return static_cast<int64_t>(LastObservedVespene) - static_cast<int64_t>(SampleVespeneBanks[SampleIndexValue]);
}

uint64_t FEconomyDomainState::GetUnitCompletionCountForHorizon(const UNIT_TYPEID UnitTypeIdValue,
                                                               const size_t HorizonIndexValue) const
{
    const size_t UnitTypeIndexValue = GetTerranUnitTypeIndex(UnitTypeIdValue);
    if (!IsTerranUnitTypeIndexValid(UnitTypeIndexValue) || HorizonIndexValue >= ForecastHorizonCountValue ||
        SampleGameLoops.empty())
    {
        return 0U;
    }

    const size_t SampleIndexValue = GetSampleIndexForHorizon(HorizonIndexValue);
    return CumulativeUnitCompletionCounts[UnitTypeIndexValue] >=
                   SampleCumulativeUnitCompletionCounts[SampleIndexValue][UnitTypeIndexValue]
               ? CumulativeUnitCompletionCounts[UnitTypeIndexValue] -
                     SampleCumulativeUnitCompletionCounts[SampleIndexValue][UnitTypeIndexValue]
               : 0U;
}

uint64_t FEconomyDomainState::GetBuildingCompletionCountForHorizon(const UNIT_TYPEID BuildingTypeIdValue,
                                                                   const size_t HorizonIndexValue) const
{
    const size_t BuildingTypeIndexValue = GetTerranBuildingTypeIndex(BuildingTypeIdValue);
    if (!IsTerranBuildingTypeIndexValid(BuildingTypeIndexValue) || HorizonIndexValue >= ForecastHorizonCountValue ||
        SampleGameLoops.empty())
    {
        return 0U;
    }

    const size_t SampleIndexValue = GetSampleIndexForHorizon(HorizonIndexValue);
    return CumulativeBuildingCompletionCounts[BuildingTypeIndexValue] >=
                   SampleCumulativeBuildingCompletionCounts[SampleIndexValue][BuildingTypeIndexValue]
               ? CumulativeBuildingCompletionCounts[BuildingTypeIndexValue] -
                     SampleCumulativeBuildingCompletionCounts[SampleIndexValue][BuildingTypeIndexValue]
               : 0U;
}

size_t FEconomyDomainState::GetSampleIndexForHorizon(const size_t HorizonIndexValue) const
{
    if (SampleGameLoops.empty())
    {
        return 0U;
    }

    const uint64_t HorizonGameLoopsValue = ForecastHorizonGameLoopsValue[HorizonIndexValue];
    const uint64_t TargetGameLoopValue =
        CurrentGameLoop >= HorizonGameLoopsValue ? CurrentGameLoop - HorizonGameLoopsValue : 0U;

    for (size_t SampleIndexValue = SampleGameLoops.size(); SampleIndexValue > 0U; --SampleIndexValue)
    {
        const size_t CandidateSampleIndexValue = SampleIndexValue - 1U;
        if (SampleGameLoops[CandidateSampleIndexValue] <= TargetGameLoopValue)
        {
            return CandidateSampleIndexValue;
        }
    }

    return 0U;
}

void FEconomyDomainState::TrimHistory()
{
    const uint64_t MinimumRetainedGameLoopValue =
        CurrentGameLoop > ForecastHorizonGameLoopsValue[LongForecastHorizonIndexValue]
            ? CurrentGameLoop - ForecastHorizonGameLoopsValue[LongForecastHorizonIndexValue]
            : 0U;

    while (SampleGameLoops.size() > 1U && SampleGameLoops[1U] <= MinimumRetainedGameLoopValue)
    {
        SampleGameLoops.erase(SampleGameLoops.begin());
        SampleMineralBanks.erase(SampleMineralBanks.begin());
        SampleVespeneBanks.erase(SampleVespeneBanks.begin());
        SampleCumulativeGrossMineralIncome.erase(SampleCumulativeGrossMineralIncome.begin());
        SampleCumulativeGrossVespeneIncome.erase(SampleCumulativeGrossVespeneIncome.begin());
        SampleCumulativeUnitCompletionCounts.erase(SampleCumulativeUnitCompletionCounts.begin());
        SampleCumulativeBuildingCompletionCounts.erase(SampleCumulativeBuildingCompletionCounts.begin());
    }

    while (SampleGameLoops.size() > MaxSampleHistoryCountValue)
    {
        SampleGameLoops.erase(SampleGameLoops.begin());
        SampleMineralBanks.erase(SampleMineralBanks.begin());
        SampleVespeneBanks.erase(SampleVespeneBanks.begin());
        SampleCumulativeGrossMineralIncome.erase(SampleCumulativeGrossMineralIncome.begin());
        SampleCumulativeGrossVespeneIncome.erase(SampleCumulativeGrossVespeneIncome.begin());
        SampleCumulativeUnitCompletionCounts.erase(SampleCumulativeUnitCompletionCounts.begin());
        SampleCumulativeBuildingCompletionCounts.erase(SampleCumulativeBuildingCompletionCounts.begin());
    }

    AssertSynchronizedSampleSizes();
}

void FEconomyDomainState::RecordCurrentSample(const uint32_t CurrentMineralsValue, const uint32_t CurrentVespeneValue)
{
    if (!SampleGameLoops.empty() && SampleGameLoops.back() == CurrentGameLoop)
    {
        SampleMineralBanks.back() = CurrentMineralsValue;
        SampleVespeneBanks.back() = CurrentVespeneValue;
        SampleCumulativeGrossMineralIncome.back() = CumulativeGrossMineralIncome;
        SampleCumulativeGrossVespeneIncome.back() = CumulativeGrossVespeneIncome;
        SampleCumulativeUnitCompletionCounts.back() = CumulativeUnitCompletionCounts;
        SampleCumulativeBuildingCompletionCounts.back() = CumulativeBuildingCompletionCounts;
        return;
    }

    SampleGameLoops.push_back(CurrentGameLoop);
    SampleMineralBanks.push_back(CurrentMineralsValue);
    SampleVespeneBanks.push_back(CurrentVespeneValue);
    SampleCumulativeGrossMineralIncome.push_back(CumulativeGrossMineralIncome);
    SampleCumulativeGrossVespeneIncome.push_back(CumulativeGrossVespeneIncome);
    SampleCumulativeUnitCompletionCounts.push_back(CumulativeUnitCompletionCounts);
    SampleCumulativeBuildingCompletionCounts.push_back(CumulativeBuildingCompletionCounts);
    AssertSynchronizedSampleSizes();
}

}  // namespace sc2
