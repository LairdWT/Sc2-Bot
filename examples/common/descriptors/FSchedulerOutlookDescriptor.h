#pragma once

#include <array>
#include <cstdint>

#include "common/terran_models.h"

namespace sc2
{

struct FSchedulerOutlookDescriptor
{
public:
    FSchedulerOutlookDescriptor();

    void Reset();
    uint32_t GetScheduledUnitCount(UNIT_TYPEID UnitTypeIdValue) const;
    uint32_t GetScheduledBuildingCount(UNIT_TYPEID BuildingTypeIdValue) const;
    uint32_t GetScheduledUpgradeCount(UpgradeID UpgradeIdValue) const;

public:
    uint32_t ActiveLeafOrderCount;
    uint32_t ReadyOrderCount;
    uint32_t DeferredOrderCount;
    uint32_t DispatchedOrderCount;
    uint32_t InFlightOrderCount;
    uint32_t ExpectedSupplyCapDelta;
    uint32_t ExpectedSupplyUsedDelta;
    int32_t ExpectedSupplyAvailableDelta;
    uint32_t ExpectedWorkerCountDelta;
    uint32_t ExpectedTownHallCapacityDelta;
    uint32_t ExpectedBarracksCapacityDelta;
    uint32_t ExpectedFactoryCapacityDelta;
    uint32_t ExpectedStarportCapacityDelta;
    std::array<uint32_t, NUM_TERRAN_UNITS> ScheduledUnitCounts;
    std::array<uint32_t, NUM_TERRAN_BUILDINGS> ScheduledBuildingCounts;
    std::array<uint32_t, NUM_TERRAN_UPGRADES> ScheduledUpgradeCounts;
};

}  // namespace sc2
