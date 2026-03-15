#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "common/economy/EconomyForecastConstants.h"
#include "common/terran_models.h"

namespace sc2
{

struct FProductionStateDescriptor
{
public:
    FProductionStateDescriptor();

    void Reset();
    uint32_t GetObservedAndInProgressUnitCount(UNIT_TYPEID UnitTypeIdValue) const;
    uint32_t GetObservedAndInProgressBuildingCount(UNIT_TYPEID BuildingTypeIdValue) const;
    uint32_t GetScheduledUnitCount(UNIT_TYPEID UnitTypeIdValue) const;
    uint32_t GetScheduledBuildingCount(UNIT_TYPEID BuildingTypeIdValue) const;
    uint32_t GetProjectedUnitCount(UNIT_TYPEID UnitTypeIdValue) const;
    uint32_t GetProjectedBuildingCount(UNIT_TYPEID BuildingTypeIdValue) const;
    uint32_t GetCurrentProducerCapacity(UNIT_TYPEID ProducerUnitTypeIdValue) const;
    uint32_t GetCurrentProducerOccupancy(UNIT_TYPEID ProducerUnitTypeIdValue) const;
    uint32_t GetNearTermProducerCapacity(UNIT_TYPEID ProducerUnitTypeIdValue) const;
    float GetProducerThroughputAverageAtHorizon(UNIT_TYPEID ProducerUnitTypeIdValue, size_t HorizonIndexValue) const;

public:
    std::array<uint32_t, NUM_TERRAN_UNITS> ObservedUnitCounts;
    std::array<uint32_t, NUM_TERRAN_UNITS> InProgressUnitCounts;
    std::array<uint32_t, NUM_TERRAN_UNITS> ScheduledUnitCounts;
    std::array<uint32_t, NUM_TERRAN_UNITS> ProjectedUnitCounts;
    std::array<uint32_t, NUM_TERRAN_BUILDINGS> ObservedBuildingCounts;
    std::array<uint32_t, NUM_TERRAN_BUILDINGS> InProgressBuildingCounts;
    std::array<uint32_t, NUM_TERRAN_BUILDINGS> ScheduledBuildingCounts;
    std::array<uint32_t, NUM_TERRAN_BUILDINGS> ProjectedBuildingCounts;
    std::array<std::array<uint32_t, ForecastHorizonCountValue>, NUM_TERRAN_UNITS> UnitCompletionCountsByHorizon;
    std::array<std::array<float, ForecastHorizonCountValue>, NUM_TERRAN_UNITS> UnitCompletionAveragesByHorizon;
    std::array<std::array<uint32_t, ForecastHorizonCountValue>, NUM_TERRAN_BUILDINGS> BuildingCompletionCountsByHorizon;
    std::array<std::array<float, ForecastHorizonCountValue>, NUM_TERRAN_BUILDINGS> BuildingCompletionAveragesByHorizon;
    uint32_t CurrentTownHallCapacity;
    uint32_t CurrentTownHallOccupancy;
    uint32_t NearTermTownHallCapacity;
    uint32_t CurrentBarracksCapacity;
    uint32_t CurrentBarracksOccupancy;
    uint32_t NearTermBarracksCapacity;
    uint32_t CurrentFactoryCapacity;
    uint32_t CurrentFactoryOccupancy;
    uint32_t NearTermFactoryCapacity;
    uint32_t CurrentStarportCapacity;
    uint32_t CurrentStarportOccupancy;
    uint32_t NearTermStarportCapacity;
    std::array<float, ForecastHorizonCountValue> TownHallThroughputAveragesByHorizon;
    std::array<float, ForecastHorizonCountValue> BarracksThroughputAveragesByHorizon;
    std::array<float, ForecastHorizonCountValue> FactoryThroughputAveragesByHorizon;
    std::array<float, ForecastHorizonCountValue> StarportThroughputAveragesByHorizon;
};

}  // namespace sc2
