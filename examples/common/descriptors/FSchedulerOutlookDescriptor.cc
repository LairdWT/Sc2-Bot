#include "common/descriptors/FSchedulerOutlookDescriptor.h"

namespace sc2
{
namespace
{

uint32_t GetUnitCountFromArray(const std::array<uint32_t, NUM_TERRAN_UNITS>& UnitCountsValue,
                               const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_HELLION:
            return UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_HELLION)] +
                   UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_HELLIONTANK)];
        case UNIT_TYPEID::TERRAN_LIBERATOR:
            return UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_LIBERATOR)] +
                   UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_LIBERATORAG)];
        case UNIT_TYPEID::TERRAN_SIEGETANK:
            return UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_SIEGETANK)] +
                   UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_SIEGETANKSIEGED)];
        default:
        {
            const size_t UnitTypeIndexValue = GetTerranUnitTypeIndex(UnitTypeIdValue);
            return IsTerranUnitTypeIndexValid(UnitTypeIndexValue) ? UnitCountsValue[UnitTypeIndexValue] : 0U;
        }
    }
}

uint32_t GetBuildingCountFromArray(const std::array<uint32_t, NUM_TERRAN_BUILDINGS>& BuildingCountsValue,
                                   const UNIT_TYPEID BuildingTypeIdValue)
{
    switch (BuildingTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
            return BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_COMMANDCENTER)] +
                   BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_COMMANDCENTERFLYING)] +
                   BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_ORBITALCOMMAND)] +
                   BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING)] +
                   BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_PLANETARYFORTRESS)];
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOT:
            return BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_SUPPLYDEPOT)] +
                   BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED)];
        case UNIT_TYPEID::TERRAN_BARRACKS:
            return BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_BARRACKS)] +
                   BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_BARRACKSFLYING)];
        case UNIT_TYPEID::TERRAN_FACTORY:
            return BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_FACTORY)] +
                   BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_FACTORYFLYING)];
        case UNIT_TYPEID::TERRAN_STARPORT:
            return BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_STARPORT)] +
                   BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_STARPORTFLYING)];
        case UNIT_TYPEID::TERRAN_REFINERY:
            return BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_REFINERY)] +
                   BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_REFINERYRICH)];
        default:
        {
            const size_t BuildingTypeIndexValue = GetTerranBuildingTypeIndex(BuildingTypeIdValue);
            return IsTerranBuildingTypeIndexValid(BuildingTypeIndexValue) ? BuildingCountsValue[BuildingTypeIndexValue]
                                                                          : 0U;
        }
    }
}

}  // namespace

FSchedulerOutlookDescriptor::FSchedulerOutlookDescriptor()
{
    Reset();
}

void FSchedulerOutlookDescriptor::Reset()
{
    ActiveLeafOrderCount = 0U;
    ReadyOrderCount = 0U;
    DeferredOrderCount = 0U;
    DispatchedOrderCount = 0U;
    InFlightOrderCount = 0U;
    ExpectedSupplyCapDelta = 0U;
    ExpectedSupplyUsedDelta = 0U;
    ExpectedSupplyAvailableDelta = 0;
    ExpectedWorkerCountDelta = 0U;
    ExpectedTownHallCapacityDelta = 0U;
    ExpectedBarracksCapacityDelta = 0U;
    ExpectedFactoryCapacityDelta = 0U;
    ExpectedStarportCapacityDelta = 0U;
    ScheduledUnitCounts.fill(0U);
    ScheduledBuildingCounts.fill(0U);
    ScheduledUpgradeCounts.fill(0U);
}

uint32_t FSchedulerOutlookDescriptor::GetScheduledUnitCount(const UNIT_TYPEID UnitTypeIdValue) const
{
    return GetUnitCountFromArray(ScheduledUnitCounts, UnitTypeIdValue);
}

uint32_t FSchedulerOutlookDescriptor::GetScheduledBuildingCount(const UNIT_TYPEID BuildingTypeIdValue) const
{
    return GetBuildingCountFromArray(ScheduledBuildingCounts, BuildingTypeIdValue);
}

uint32_t FSchedulerOutlookDescriptor::GetScheduledUpgradeCount(const UpgradeID UpgradeIdValue) const
{
    const size_t UpgradeTypeIndexValue = GetTerranUpgradeTypeIndex(UpgradeIdValue);
    return IsTerranUpgradeTypeIndexValid(UpgradeTypeIndexValue) ? ScheduledUpgradeCounts[UpgradeTypeIndexValue] : 0U;
}

}  // namespace sc2
