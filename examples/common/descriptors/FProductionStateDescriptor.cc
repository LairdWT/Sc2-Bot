#include "common/descriptors/FProductionStateDescriptor.h"

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
        case UNIT_TYPEID::TERRAN_WIDOWMINE:
            return UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_WIDOWMINE)] +
                   UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_WIDOWMINEBURROWED)];
        case UNIT_TYPEID::TERRAN_VIKINGFIGHTER:
            return UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_VIKINGFIGHTER)] +
                   UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_VIKINGASSAULT)];
        case UNIT_TYPEID::TERRAN_THOR:
            return UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_THOR)] +
                   UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_THORAP)];
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
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
            return BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_ORBITALCOMMAND)] +
                   BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING)];
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

FProductionStateDescriptor::FProductionStateDescriptor()
{
    Reset();
}

void FProductionStateDescriptor::Reset()
{
    ObservedUnitCounts.fill(0U);
    InProgressUnitCounts.fill(0U);
    ScheduledUnitCounts.fill(0U);
    ProjectedUnitCounts.fill(0U);
    ObservedBuildingCounts.fill(0U);
    InProgressBuildingCounts.fill(0U);
    ScheduledBuildingCounts.fill(0U);
    ProjectedBuildingCounts.fill(0U);
    for (std::array<uint32_t, ForecastHorizonCountValue>& CompletionCountsValue : UnitCompletionCountsByHorizon)
    {
        CompletionCountsValue.fill(0U);
    }
    for (std::array<float, ForecastHorizonCountValue>& CompletionAveragesValue : UnitCompletionAveragesByHorizon)
    {
        CompletionAveragesValue.fill(0.0f);
    }
    for (std::array<uint32_t, ForecastHorizonCountValue>& CompletionCountsValue : BuildingCompletionCountsByHorizon)
    {
        CompletionCountsValue.fill(0U);
    }
    for (std::array<float, ForecastHorizonCountValue>& CompletionAveragesValue : BuildingCompletionAveragesByHorizon)
    {
        CompletionAveragesValue.fill(0.0f);
    }
    CurrentTownHallCapacity = 0U;
    CurrentTownHallOccupancy = 0U;
    NearTermTownHallCapacity = 0U;
    CurrentBarracksCapacity = 0U;
    CurrentBarracksOccupancy = 0U;
    NearTermBarracksCapacity = 0U;
    CurrentFactoryCapacity = 0U;
    CurrentFactoryOccupancy = 0U;
    NearTermFactoryCapacity = 0U;
    CurrentStarportCapacity = 0U;
    CurrentStarportOccupancy = 0U;
    NearTermStarportCapacity = 0U;
    TownHallThroughputAveragesByHorizon.fill(0.0f);
    BarracksThroughputAveragesByHorizon.fill(0.0f);
    FactoryThroughputAveragesByHorizon.fill(0.0f);
    StarportThroughputAveragesByHorizon.fill(0.0f);
}

uint32_t FProductionStateDescriptor::GetObservedAndInProgressUnitCount(const UNIT_TYPEID UnitTypeIdValue) const
{
    return GetUnitCountFromArray(ObservedUnitCounts, UnitTypeIdValue) +
           GetUnitCountFromArray(InProgressUnitCounts, UnitTypeIdValue);
}

uint32_t FProductionStateDescriptor::GetObservedAndInProgressBuildingCount(const UNIT_TYPEID BuildingTypeIdValue) const
{
    return GetBuildingCountFromArray(ObservedBuildingCounts, BuildingTypeIdValue) +
           GetBuildingCountFromArray(InProgressBuildingCounts, BuildingTypeIdValue);
}

uint32_t FProductionStateDescriptor::GetScheduledUnitCount(const UNIT_TYPEID UnitTypeIdValue) const
{
    return GetUnitCountFromArray(ScheduledUnitCounts, UnitTypeIdValue);
}

uint32_t FProductionStateDescriptor::GetScheduledBuildingCount(const UNIT_TYPEID BuildingTypeIdValue) const
{
    return GetBuildingCountFromArray(ScheduledBuildingCounts, BuildingTypeIdValue);
}

uint32_t FProductionStateDescriptor::GetProjectedUnitCount(const UNIT_TYPEID UnitTypeIdValue) const
{
    return GetUnitCountFromArray(ProjectedUnitCounts, UnitTypeIdValue);
}

uint32_t FProductionStateDescriptor::GetProjectedBuildingCount(const UNIT_TYPEID BuildingTypeIdValue) const
{
    return GetBuildingCountFromArray(ProjectedBuildingCounts, BuildingTypeIdValue);
}

uint32_t FProductionStateDescriptor::GetCurrentProducerCapacity(const UNIT_TYPEID ProducerUnitTypeIdValue) const
{
    switch (ProducerUnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
            return CurrentTownHallCapacity;
        case UNIT_TYPEID::TERRAN_BARRACKS:
            return CurrentBarracksCapacity;
        case UNIT_TYPEID::TERRAN_FACTORY:
            return CurrentFactoryCapacity;
        case UNIT_TYPEID::TERRAN_STARPORT:
            return CurrentStarportCapacity;
        default:
            return 0U;
    }
}

uint32_t FProductionStateDescriptor::GetCurrentProducerOccupancy(const UNIT_TYPEID ProducerUnitTypeIdValue) const
{
    switch (ProducerUnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
            return CurrentTownHallOccupancy;
        case UNIT_TYPEID::TERRAN_BARRACKS:
            return CurrentBarracksOccupancy;
        case UNIT_TYPEID::TERRAN_FACTORY:
            return CurrentFactoryOccupancy;
        case UNIT_TYPEID::TERRAN_STARPORT:
            return CurrentStarportOccupancy;
        default:
            return 0U;
    }
}

uint32_t FProductionStateDescriptor::GetNearTermProducerCapacity(const UNIT_TYPEID ProducerUnitTypeIdValue) const
{
    switch (ProducerUnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
            return NearTermTownHallCapacity;
        case UNIT_TYPEID::TERRAN_BARRACKS:
            return NearTermBarracksCapacity;
        case UNIT_TYPEID::TERRAN_FACTORY:
            return NearTermFactoryCapacity;
        case UNIT_TYPEID::TERRAN_STARPORT:
            return NearTermStarportCapacity;
        default:
            return 0U;
    }
}

float FProductionStateDescriptor::GetProducerThroughputAverageAtHorizon(const UNIT_TYPEID ProducerUnitTypeIdValue,
                                                                        const size_t HorizonIndexValue) const
{
    if (HorizonIndexValue >= ForecastHorizonCountValue)
    {
        return 0.0f;
    }

    switch (ProducerUnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
            return TownHallThroughputAveragesByHorizon[HorizonIndexValue];
        case UNIT_TYPEID::TERRAN_BARRACKS:
            return BarracksThroughputAveragesByHorizon[HorizonIndexValue];
        case UNIT_TYPEID::TERRAN_FACTORY:
            return FactoryThroughputAveragesByHorizon[HorizonIndexValue];
        case UNIT_TYPEID::TERRAN_STARPORT:
            return StarportThroughputAveragesByHorizon[HorizonIndexValue];
        default:
            return 0.0f;
    }
}

}  // namespace sc2
