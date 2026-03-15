#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "common/economy/EconomyForecastConstants.h"
#include "common/terran_models.h"

namespace sc2
{

struct FAgentState;

struct FEconomyDomainState
{
public:
    FEconomyDomainState();

    void Reset();
    void Update(const FAgentState& AgentStateValue, uint64_t CurrentGameLoopValue);
    size_t GetSampleCount() const;
    uint64_t GetElapsedGameLoopsForHorizon(size_t HorizonIndexValue) const;
    uint64_t GetGrossMineralIncomeForHorizon(size_t HorizonIndexValue) const;
    uint64_t GetGrossVespeneIncomeForHorizon(size_t HorizonIndexValue) const;
    int64_t GetNetMineralDeltaForHorizon(size_t HorizonIndexValue) const;
    int64_t GetNetVespeneDeltaForHorizon(size_t HorizonIndexValue) const;
    uint64_t GetUnitCompletionCountForHorizon(UNIT_TYPEID UnitTypeIdValue, size_t HorizonIndexValue) const;
    uint64_t GetBuildingCompletionCountForHorizon(UNIT_TYPEID BuildingTypeIdValue, size_t HorizonIndexValue) const;

public:
    uint64_t CurrentGameLoop;
    bool bHasObservedState;
    uint32_t LastObservedMinerals;
    uint32_t LastObservedVespene;
    std::array<uint16_t, NUM_TERRAN_UNITS> LastObservedUnitCounts;
    std::array<uint16_t, NUM_TERRAN_UNITS> LastObservedUnitsInConstruction;
    std::array<uint16_t, NUM_TERRAN_BUILDINGS> LastObservedBuildingCounts;
    std::array<uint16_t, NUM_TERRAN_BUILDINGS> LastObservedBuildingsInConstruction;
    std::array<uint16_t, NUM_TERRAN_UPGRADES> LastObservedActiveUpgradeCounts;
    std::array<uint8_t, NUM_TERRAN_UPGRADES> LastObservedCompletedUpgradeCounts;
    uint64_t CumulativeGrossMineralIncome;
    uint64_t CumulativeGrossVespeneIncome;
    std::array<uint64_t, NUM_TERRAN_UNITS> CumulativeUnitCompletionCounts;
    std::array<uint64_t, NUM_TERRAN_BUILDINGS> CumulativeBuildingCompletionCounts;
    std::vector<uint64_t> SampleGameLoops;
    std::vector<uint32_t> SampleMineralBanks;
    std::vector<uint32_t> SampleVespeneBanks;
    std::vector<uint64_t> SampleCumulativeGrossMineralIncome;
    std::vector<uint64_t> SampleCumulativeGrossVespeneIncome;
    std::vector<std::array<uint64_t, NUM_TERRAN_UNITS>> SampleCumulativeUnitCompletionCounts;
    std::vector<std::array<uint64_t, NUM_TERRAN_BUILDINGS>> SampleCumulativeBuildingCompletionCounts;

private:
    size_t GetSampleIndexForHorizon(size_t HorizonIndexValue) const;
    void TrimHistory();
    void RecordCurrentSample(uint32_t CurrentMineralsValue, uint32_t CurrentVespeneValue);
};

}  // namespace sc2
