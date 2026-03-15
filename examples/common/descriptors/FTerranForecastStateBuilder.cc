#include "common/descriptors/FTerranForecastStateBuilder.h"

#include <algorithm>
#include <unordered_map>

#include "common/bot_status_models.h"
#include "common/descriptors/FEconomyStateDescriptor.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/descriptors/FProductionStateDescriptor.h"
#include "common/descriptors/FSchedulerOutlookDescriptor.h"
#include "common/economic_models.h"
#include "common/planning/FCommandAuthoritySchedulingState.h"

namespace sc2
{
namespace
{

uint32_t GetBuildingCountFromObservedArray(const std::array<uint16_t, NUM_TERRAN_BUILDINGS>& BuildingCountsValue,
                                           const UNIT_TYPEID BuildingTypeIdValue)
{
    switch (BuildingTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
            return static_cast<uint32_t>(
                       BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_COMMANDCENTER)]) +
                   static_cast<uint32_t>(
                       BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_COMMANDCENTERFLYING)]) +
                   static_cast<uint32_t>(
                       BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_ORBITALCOMMAND)]) +
                   static_cast<uint32_t>(
                       BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING)]) +
                   static_cast<uint32_t>(
                       BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_PLANETARYFORTRESS)]);
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
            return static_cast<uint32_t>(
                       BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_ORBITALCOMMAND)]) +
                   static_cast<uint32_t>(
                       BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING)]);
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOT:
            return static_cast<uint32_t>(
                       BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_SUPPLYDEPOT)]) +
                   static_cast<uint32_t>(
                       BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED)]);
        case UNIT_TYPEID::TERRAN_BARRACKS:
            return static_cast<uint32_t>(
                       BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_BARRACKS)]) +
                   static_cast<uint32_t>(
                       BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_BARRACKSFLYING)]);
        case UNIT_TYPEID::TERRAN_FACTORY:
            return static_cast<uint32_t>(
                       BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_FACTORY)]) +
                   static_cast<uint32_t>(
                       BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_FACTORYFLYING)]);
        case UNIT_TYPEID::TERRAN_STARPORT:
            return static_cast<uint32_t>(
                       BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_STARPORT)]) +
                   static_cast<uint32_t>(
                       BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_STARPORTFLYING)]);
        case UNIT_TYPEID::TERRAN_REFINERY:
            return static_cast<uint32_t>(
                       BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_REFINERY)]) +
                   static_cast<uint32_t>(
                       BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_REFINERYRICH)]);
        default:
        {
            const size_t BuildingTypeIndexValue = GetTerranBuildingTypeIndex(BuildingTypeIdValue);
            return IsTerranBuildingTypeIndexValid(BuildingTypeIndexValue)
                       ? static_cast<uint32_t>(BuildingCountsValue[BuildingTypeIndexValue])
                       : 0U;
        }
    }
}

uint32_t GetUnitCountFromObservedArray(const std::array<uint16_t, NUM_TERRAN_UNITS>& UnitCountsValue,
                                       const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_HELLION:
            return static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_HELLION)]) +
                   static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_HELLIONTANK)]);
        case UNIT_TYPEID::TERRAN_LIBERATOR:
            return static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_LIBERATOR)]) +
                   static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_LIBERATORAG)]);
        case UNIT_TYPEID::TERRAN_SIEGETANK:
            return static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_SIEGETANK)]) +
                   static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_SIEGETANKSIEGED)]);
        case UNIT_TYPEID::TERRAN_VIKINGFIGHTER:
            return static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_VIKINGFIGHTER)]) +
                   static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_VIKINGASSAULT)]);
        case UNIT_TYPEID::TERRAN_WIDOWMINE:
            return static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_WIDOWMINE)]) +
                   static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_WIDOWMINEBURROWED)]);
        case UNIT_TYPEID::TERRAN_THOR:
            return static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_THOR)]) +
                   static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_THORAP)]);
        default:
        {
            const size_t UnitTypeIndexValue = GetTerranUnitTypeIndex(UnitTypeIdValue);
            return IsTerranUnitTypeIndexValid(UnitTypeIndexValue)
                       ? static_cast<uint32_t>(UnitCountsValue[UnitTypeIndexValue])
                       : 0U;
        }
    }
}

uint32_t GetObservedCountForOrder(const FBuildPlanningState& BuildPlanningStateValue,
                                  const FCommandOrderRecord& CommandOrderRecordValue)
{
    if (CommandOrderRecordValue.UpgradeId.ToType() != UPGRADE_ID::INVALID)
    {
        const size_t UpgradeTypeIndexValue = GetTerranUpgradeTypeIndex(CommandOrderRecordValue.UpgradeId);
        return IsTerranUpgradeTypeIndexValid(UpgradeTypeIndexValue)
                   ? static_cast<uint32_t>(BuildPlanningStateValue.ObservedCompletedUpgradeCounts[UpgradeTypeIndexValue])
                   : 0U;
    }

    switch (CommandOrderRecordValue.ResultUnitTypeId)
    {
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
            return BuildPlanningStateValue.ObservedTownHallCount;
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
            return BuildPlanningStateValue.ObservedOrbitalCommandCount;
        default:
            break;
    }

    if (IsTerranBuilding(CommandOrderRecordValue.ResultUnitTypeId))
    {
        return GetBuildingCountFromObservedArray(BuildPlanningStateValue.ObservedBuildingCounts,
                                                 CommandOrderRecordValue.ResultUnitTypeId);
    }

    return GetUnitCountFromObservedArray(BuildPlanningStateValue.ObservedUnitCounts,
                                         CommandOrderRecordValue.ResultUnitTypeId);
}

uint32_t GetObservedInConstructionCountForOrder(const FBuildPlanningState& BuildPlanningStateValue,
                                                const FCommandOrderRecord& CommandOrderRecordValue)
{
    if (CommandOrderRecordValue.UpgradeId.ToType() != UPGRADE_ID::INVALID)
    {
        return 0U;
    }

    if (IsTerranBuilding(CommandOrderRecordValue.ResultUnitTypeId))
    {
        return GetBuildingCountFromObservedArray(BuildPlanningStateValue.ObservedBuildingsInConstruction,
                                                 CommandOrderRecordValue.ResultUnitTypeId);
    }

    return GetUnitCountFromObservedArray(BuildPlanningStateValue.ObservedUnitsInConstruction,
                                         CommandOrderRecordValue.ResultUnitTypeId);
}

bool HasActiveChildOrder(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                         const uint32_t ParentOrderIdValue)
{
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.ParentOrderIds[OrderIndexValue] != ParentOrderIdValue ||
            IsTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        return true;
    }

    return false;
}

bool HasOrderMaterializedIntoObservation(const FBuildPlanningState& BuildPlanningStateValue,
                                         const FCommandOrderRecord& CommandOrderRecordValue)
{
    if (CommandOrderRecordValue.DispatchAttemptCount == 0U)
    {
        return false;
    }

    const uint32_t CurrentObservedCountValue = GetObservedCountForOrder(BuildPlanningStateValue, CommandOrderRecordValue);
    const uint32_t CurrentObservedInConstructionCountValue =
        GetObservedInConstructionCountForOrder(BuildPlanningStateValue, CommandOrderRecordValue);

    return CurrentObservedCountValue > CommandOrderRecordValue.ObservedCountAtDispatch ||
           CurrentObservedInConstructionCountValue > CommandOrderRecordValue.ObservedInConstructionCountAtDispatch;
}

uint32_t GetSupplyCapContributionForBuildingType(const UNIT_TYPEID BuildingTypeIdValue)
{
    switch (BuildingTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOT:
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED:
            return 8U;
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
        case UNIT_TYPEID::TERRAN_COMMANDCENTERFLYING:
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
        case UNIT_TYPEID::TERRAN_ORBITALCOMMANDFLYING:
        case UNIT_TYPEID::TERRAN_PLANETARYFORTRESS:
            return 15U;
        default:
            return 0U;
    }
}

bool IsTownHallProducer(const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
            return true;
        default:
            return false;
    }
}

bool IsBarracksProducer(const UNIT_TYPEID UnitTypeIdValue)
{
    return UnitTypeIdValue == UNIT_TYPEID::TERRAN_BARRACKS;
}

bool IsFactoryProducer(const UNIT_TYPEID UnitTypeIdValue)
{
    return UnitTypeIdValue == UNIT_TYPEID::TERRAN_FACTORY;
}

bool IsStarportProducer(const UNIT_TYPEID UnitTypeIdValue)
{
    return UnitTypeIdValue == UNIT_TYPEID::TERRAN_STARPORT;
}

bool IsReactorUnitType(const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_REACTOR:
        case UNIT_TYPEID::TERRAN_BARRACKSREACTOR:
        case UNIT_TYPEID::TERRAN_FACTORYREACTOR:
        case UNIT_TYPEID::TERRAN_STARPORTREACTOR:
            return true;
        default:
            return false;
    }
}

uint32_t GetProducerQueueCapacity(const Unit& ProducerUnitValue,
                                  const std::unordered_map<Tag, const Unit*>& UnitByTagValue)
{
    if (ProducerUnitValue.add_on_tag != NullTag)
    {
        const std::unordered_map<Tag, const Unit*>::const_iterator AddonIteratorValue =
            UnitByTagValue.find(ProducerUnitValue.add_on_tag);
        if (AddonIteratorValue != UnitByTagValue.end() && AddonIteratorValue->second != nullptr &&
            IsReactorUnitType(AddonIteratorValue->second->unit_type.ToType()))
        {
            return 2U;
        }
    }

    return 1U;
}

bool ShouldCountScheduledLeafOrder(const FBuildPlanningState& BuildPlanningStateValue,
                                   const FCommandOrderRecord& CommandOrderRecordValue)
{
    if (CommandOrderRecordValue.ResultUnitTypeId == UNIT_TYPEID::INVALID &&
        CommandOrderRecordValue.UpgradeId.ToType() == UPGRADE_ID::INVALID)
    {
        return false;
    }

    return !HasOrderMaterializedIntoObservation(BuildPlanningStateValue, CommandOrderRecordValue);
}

uint64_t RoundPositiveFloatToUint64(const float Value)
{
    return Value > 0.0f ? static_cast<uint64_t>(Value + 0.5f) : 0U;
}

void PopulateSchedulerOutlook(const FBuildPlanningState& BuildPlanningStateValue,
                              const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                              FSchedulerOutlookDescriptor& SchedulerOutlookDescriptorValue)
{
    SchedulerOutlookDescriptorValue.Reset();

    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        const EOrderLifecycleState LifecycleStateValue =
            CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue];
        if (IsTerminalLifecycleState(LifecycleStateValue))
        {
            continue;
        }

        if (CommandAuthoritySchedulingStateValue.LastDeferralReasons[OrderIndexValue] !=
            ECommandOrderDeferralReason::None)
        {
            ++SchedulerOutlookDescriptorValue.DeferredOrderCount;
        }

        switch (LifecycleStateValue)
        {
            case EOrderLifecycleState::Ready:
                ++SchedulerOutlookDescriptorValue.ReadyOrderCount;
                break;
            case EOrderLifecycleState::Dispatched:
                ++SchedulerOutlookDescriptorValue.DispatchedOrderCount;
                break;
            default:
                break;
        }

        if (CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] == ECommandAuthorityLayer::UnitExecution)
        {
            ++SchedulerOutlookDescriptorValue.InFlightOrderCount;
        }

        const uint32_t OrderIdValue = CommandAuthoritySchedulingStateValue.OrderIds[OrderIndexValue];
        if (HasActiveChildOrder(CommandAuthoritySchedulingStateValue, OrderIdValue))
        {
            continue;
        }

        const FCommandOrderRecord CommandOrderRecordValue =
            CommandAuthoritySchedulingStateValue.GetOrderRecord(OrderIndexValue);
        if (!ShouldCountScheduledLeafOrder(BuildPlanningStateValue, CommandOrderRecordValue))
        {
            continue;
        }

        ++SchedulerOutlookDescriptorValue.ActiveLeafOrderCount;

        if (CommandOrderRecordValue.UpgradeId.ToType() != UPGRADE_ID::INVALID)
        {
            const size_t UpgradeTypeIndexValue = GetTerranUpgradeTypeIndex(CommandOrderRecordValue.UpgradeId);
            if (IsTerranUpgradeTypeIndexValid(UpgradeTypeIndexValue))
            {
                ++SchedulerOutlookDescriptorValue.ScheduledUpgradeCounts[UpgradeTypeIndexValue];
            }
            continue;
        }

        if (IsTerranBuilding(CommandOrderRecordValue.ResultUnitTypeId))
        {
            const size_t BuildingTypeIndexValue = GetTerranBuildingTypeIndex(CommandOrderRecordValue.ResultUnitTypeId);
            if (IsTerranBuildingTypeIndexValid(BuildingTypeIndexValue))
            {
                ++SchedulerOutlookDescriptorValue.ScheduledBuildingCounts[BuildingTypeIndexValue];
            }

            SchedulerOutlookDescriptorValue.ExpectedSupplyCapDelta +=
                GetSupplyCapContributionForBuildingType(CommandOrderRecordValue.ResultUnitTypeId);

            switch (CommandOrderRecordValue.ResultUnitTypeId)
            {
                case UNIT_TYPEID::TERRAN_COMMANDCENTER:
                    ++SchedulerOutlookDescriptorValue.ExpectedTownHallCapacityDelta;
                    break;
                case UNIT_TYPEID::TERRAN_BARRACKS:
                case UNIT_TYPEID::TERRAN_BARRACKSREACTOR:
                    ++SchedulerOutlookDescriptorValue.ExpectedBarracksCapacityDelta;
                    break;
                case UNIT_TYPEID::TERRAN_FACTORY:
                case UNIT_TYPEID::TERRAN_FACTORYREACTOR:
                    ++SchedulerOutlookDescriptorValue.ExpectedFactoryCapacityDelta;
                    break;
                case UNIT_TYPEID::TERRAN_STARPORT:
                case UNIT_TYPEID::TERRAN_STARPORTREACTOR:
                    ++SchedulerOutlookDescriptorValue.ExpectedStarportCapacityDelta;
                    break;
                default:
                    break;
            }
            continue;
        }

        const size_t UnitTypeIndexValue = GetTerranUnitTypeIndex(CommandOrderRecordValue.ResultUnitTypeId);
        if (IsTerranUnitTypeIndexValid(UnitTypeIndexValue))
        {
            ++SchedulerOutlookDescriptorValue.ScheduledUnitCounts[UnitTypeIndexValue];
            const FUnitCostData& UnitCostDataValue =
                TERRAN_ECONOMIC_DATA.GetUnitCostData(CommandOrderRecordValue.ResultUnitTypeId);
            SchedulerOutlookDescriptorValue.ExpectedSupplyUsedDelta += UnitCostDataValue.CostData.Supply;
            if (CommandOrderRecordValue.ResultUnitTypeId == UNIT_TYPEID::TERRAN_SCV)
            {
                ++SchedulerOutlookDescriptorValue.ExpectedWorkerCountDelta;
            }
        }
    }

    SchedulerOutlookDescriptorValue.ExpectedSupplyAvailableDelta =
        static_cast<int32_t>(SchedulerOutlookDescriptorValue.ExpectedSupplyCapDelta) -
        static_cast<int32_t>(SchedulerOutlookDescriptorValue.ExpectedSupplyUsedDelta);
}

void PopulateProductionState(const FAgentState& AgentStateValue, const FEconomyDomainState& EconomyDomainStateValue,
                             const FBuildPlanningState& BuildPlanningStateValue,
                             const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                             const FSchedulerOutlookDescriptor& SchedulerOutlookDescriptorValue,
                             FProductionStateDescriptor& ProductionStateDescriptorValue)
{
    ProductionStateDescriptorValue.Reset();

    for (const UNIT_TYPEID UnitTypeIdValue : TERRAN_UNIT_TYPES)
    {
        const size_t UnitTypeIndexValue = GetTerranUnitTypeIndex(UnitTypeIdValue);
        if (!IsTerranUnitTypeIndexValid(UnitTypeIndexValue))
        {
            continue;
        }

        ProductionStateDescriptorValue.ObservedUnitCounts[UnitTypeIndexValue] =
            static_cast<uint32_t>(BuildPlanningStateValue.ObservedUnitCounts[UnitTypeIndexValue]);
        ProductionStateDescriptorValue.InProgressUnitCounts[UnitTypeIndexValue] =
            static_cast<uint32_t>(BuildPlanningStateValue.ObservedUnitsInConstruction[UnitTypeIndexValue]);
        ProductionStateDescriptorValue.ScheduledUnitCounts[UnitTypeIndexValue] =
            SchedulerOutlookDescriptorValue.ScheduledUnitCounts[UnitTypeIndexValue];
        ProductionStateDescriptorValue.ProjectedUnitCounts[UnitTypeIndexValue] =
            ProductionStateDescriptorValue.ObservedUnitCounts[UnitTypeIndexValue] +
            ProductionStateDescriptorValue.InProgressUnitCounts[UnitTypeIndexValue] +
            ProductionStateDescriptorValue.ScheduledUnitCounts[UnitTypeIndexValue];

        for (size_t HorizonIndexValue = 0U; HorizonIndexValue < ForecastHorizonCountValue; ++HorizonIndexValue)
        {
            const uint64_t UnitCompletionCountValue =
                EconomyDomainStateValue.GetUnitCompletionCountForHorizon(UnitTypeIdValue, HorizonIndexValue);
            const uint64_t ElapsedGameLoopsValue =
                EconomyDomainStateValue.GetElapsedGameLoopsForHorizon(HorizonIndexValue);
            ProductionStateDescriptorValue.UnitCompletionCountsByHorizon[UnitTypeIndexValue][HorizonIndexValue] =
                static_cast<uint32_t>(UnitCompletionCountValue);
            ProductionStateDescriptorValue.UnitCompletionAveragesByHorizon[UnitTypeIndexValue][HorizonIndexValue] =
                ElapsedGameLoopsValue > 0U ? static_cast<float>(UnitCompletionCountValue) /
                                                 static_cast<float>(ElapsedGameLoopsValue)
                                           : 0.0f;
        }
    }

    for (const UNIT_TYPEID BuildingTypeIdValue : TERRAN_BUILDING_TYPES)
    {
        const size_t BuildingTypeIndexValue = GetTerranBuildingTypeIndex(BuildingTypeIdValue);
        if (!IsTerranBuildingTypeIndexValid(BuildingTypeIndexValue))
        {
            continue;
        }

        ProductionStateDescriptorValue.ObservedBuildingCounts[BuildingTypeIndexValue] =
            static_cast<uint32_t>(BuildPlanningStateValue.ObservedBuildingCounts[BuildingTypeIndexValue]);
        ProductionStateDescriptorValue.InProgressBuildingCounts[BuildingTypeIndexValue] =
            static_cast<uint32_t>(BuildPlanningStateValue.ObservedBuildingsInConstruction[BuildingTypeIndexValue]);
        ProductionStateDescriptorValue.ScheduledBuildingCounts[BuildingTypeIndexValue] =
            SchedulerOutlookDescriptorValue.ScheduledBuildingCounts[BuildingTypeIndexValue];
        ProductionStateDescriptorValue.ProjectedBuildingCounts[BuildingTypeIndexValue] =
            ProductionStateDescriptorValue.ObservedBuildingCounts[BuildingTypeIndexValue] +
            ProductionStateDescriptorValue.InProgressBuildingCounts[BuildingTypeIndexValue] +
            ProductionStateDescriptorValue.ScheduledBuildingCounts[BuildingTypeIndexValue];

        for (size_t HorizonIndexValue = 0U; HorizonIndexValue < ForecastHorizonCountValue; ++HorizonIndexValue)
        {
            const uint64_t BuildingCompletionCountValue =
                EconomyDomainStateValue.GetBuildingCompletionCountForHorizon(BuildingTypeIdValue, HorizonIndexValue);
            const uint64_t ElapsedGameLoopsValue =
                EconomyDomainStateValue.GetElapsedGameLoopsForHorizon(HorizonIndexValue);
            ProductionStateDescriptorValue.BuildingCompletionCountsByHorizon[BuildingTypeIndexValue][HorizonIndexValue] =
                static_cast<uint32_t>(BuildingCompletionCountValue);
            ProductionStateDescriptorValue.BuildingCompletionAveragesByHorizon[BuildingTypeIndexValue][HorizonIndexValue] =
                ElapsedGameLoopsValue > 0U ? static_cast<float>(BuildingCompletionCountValue) /
                                                 static_cast<float>(ElapsedGameLoopsValue)
                                           : 0.0f;
        }
    }

    std::unordered_map<Tag, const Unit*> UnitByTagValue;
    UnitByTagValue.reserve(AgentStateValue.UnitContainer.ControlledUnits.size());
    for (const Unit* ControlledUnitValue : AgentStateValue.UnitContainer.ControlledUnits)
    {
        if (ControlledUnitValue == nullptr)
        {
            continue;
        }

        UnitByTagValue.emplace(ControlledUnitValue->tag, ControlledUnitValue);
    }

    std::unordered_map<Tag, uint32_t> ScheduledUnitProductionCountByActorTagValue;
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (IsTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]) ||
            CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] != ECommandAuthorityLayer::UnitExecution ||
            CommandAuthoritySchedulingStateValue.IntentDomains[OrderIndexValue] != EIntentDomain::UnitProduction)
        {
            continue;
        }

        const uint32_t OrderIdValue = CommandAuthoritySchedulingStateValue.OrderIds[OrderIndexValue];
        if (HasActiveChildOrder(CommandAuthoritySchedulingStateValue, OrderIdValue))
        {
            continue;
        }

        const FCommandOrderRecord CommandOrderRecordValue =
            CommandAuthoritySchedulingStateValue.GetOrderRecord(OrderIndexValue);
        if (!ShouldCountScheduledLeafOrder(BuildPlanningStateValue, CommandOrderRecordValue))
        {
            continue;
        }

        ++ScheduledUnitProductionCountByActorTagValue[CommandOrderRecordValue.ActorTag];
    }

    for (const Unit* ControlledUnitValue : AgentStateValue.UnitContainer.ControlledUnits)
    {
        if (ControlledUnitValue == nullptr || ControlledUnitValue->build_progress < 1.0f ||
            !ControlledUnitValue->is_building || ControlledUnitValue->is_flying)
        {
            continue;
        }

        const UNIT_TYPEID UnitTypeIdValue = ControlledUnitValue->unit_type.ToType();
        const uint32_t ScheduledOccupancyCountValue =
            ScheduledUnitProductionCountByActorTagValue.count(ControlledUnitValue->tag) > 0U
                ? ScheduledUnitProductionCountByActorTagValue[ControlledUnitValue->tag]
                : 0U;

        if (IsTownHallProducer(UnitTypeIdValue))
        {
            ++ProductionStateDescriptorValue.CurrentTownHallCapacity;
            ProductionStateDescriptorValue.CurrentTownHallOccupancy +=
                static_cast<uint32_t>(ControlledUnitValue->orders.size()) + ScheduledOccupancyCountValue;
            continue;
        }

        if (IsBarracksProducer(UnitTypeIdValue))
        {
            const uint32_t ProducerCapacityValue = GetProducerQueueCapacity(*ControlledUnitValue, UnitByTagValue);
            ProductionStateDescriptorValue.CurrentBarracksCapacity += ProducerCapacityValue;
            ProductionStateDescriptorValue.CurrentBarracksOccupancy +=
                std::min<uint32_t>(ProducerCapacityValue,
                                   static_cast<uint32_t>(ControlledUnitValue->orders.size()) + ScheduledOccupancyCountValue);
            continue;
        }

        if (IsFactoryProducer(UnitTypeIdValue))
        {
            const uint32_t ProducerCapacityValue = GetProducerQueueCapacity(*ControlledUnitValue, UnitByTagValue);
            ProductionStateDescriptorValue.CurrentFactoryCapacity += ProducerCapacityValue;
            ProductionStateDescriptorValue.CurrentFactoryOccupancy +=
                std::min<uint32_t>(ProducerCapacityValue,
                                   static_cast<uint32_t>(ControlledUnitValue->orders.size()) + ScheduledOccupancyCountValue);
            continue;
        }

        if (IsStarportProducer(UnitTypeIdValue))
        {
            const uint32_t ProducerCapacityValue = GetProducerQueueCapacity(*ControlledUnitValue, UnitByTagValue);
            ProductionStateDescriptorValue.CurrentStarportCapacity += ProducerCapacityValue;
            ProductionStateDescriptorValue.CurrentStarportOccupancy +=
                std::min<uint32_t>(ProducerCapacityValue,
                                   static_cast<uint32_t>(ControlledUnitValue->orders.size()) + ScheduledOccupancyCountValue);
            continue;
        }
    }

    ProductionStateDescriptorValue.NearTermTownHallCapacity =
        ProductionStateDescriptorValue.CurrentTownHallCapacity +
        static_cast<uint32_t>(BuildPlanningStateValue.ObservedBuildingsInConstruction[GetTerranBuildingTypeIndex(
            UNIT_TYPEID::TERRAN_COMMANDCENTER)]);
    ProductionStateDescriptorValue.NearTermBarracksCapacity =
        ProductionStateDescriptorValue.CurrentBarracksCapacity +
        static_cast<uint32_t>(BuildPlanningStateValue.ObservedBuildingsInConstruction[GetTerranBuildingTypeIndex(
            UNIT_TYPEID::TERRAN_BARRACKS)]) +
        static_cast<uint32_t>(BuildPlanningStateValue.ObservedBuildingsInConstruction[GetTerranBuildingTypeIndex(
            UNIT_TYPEID::TERRAN_BARRACKSREACTOR)]);
    ProductionStateDescriptorValue.NearTermFactoryCapacity =
        ProductionStateDescriptorValue.CurrentFactoryCapacity +
        static_cast<uint32_t>(BuildPlanningStateValue.ObservedBuildingsInConstruction[GetTerranBuildingTypeIndex(
            UNIT_TYPEID::TERRAN_FACTORY)]) +
        static_cast<uint32_t>(BuildPlanningStateValue.ObservedBuildingsInConstruction[GetTerranBuildingTypeIndex(
            UNIT_TYPEID::TERRAN_FACTORYREACTOR)]);
    ProductionStateDescriptorValue.NearTermStarportCapacity =
        ProductionStateDescriptorValue.CurrentStarportCapacity +
        static_cast<uint32_t>(BuildPlanningStateValue.ObservedBuildingsInConstruction[GetTerranBuildingTypeIndex(
            UNIT_TYPEID::TERRAN_STARPORT)]) +
        static_cast<uint32_t>(BuildPlanningStateValue.ObservedBuildingsInConstruction[GetTerranBuildingTypeIndex(
            UNIT_TYPEID::TERRAN_STARPORTREACTOR)]);

    for (size_t HorizonIndexValue = 0U; HorizonIndexValue < ForecastHorizonCountValue; ++HorizonIndexValue)
    {
        ProductionStateDescriptorValue.TownHallThroughputAveragesByHorizon[HorizonIndexValue] =
            ProductionStateDescriptorValue.UnitCompletionAveragesByHorizon[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_SCV)]
                                                                [HorizonIndexValue];
        ProductionStateDescriptorValue.BarracksThroughputAveragesByHorizon[HorizonIndexValue] =
            ProductionStateDescriptorValue.UnitCompletionAveragesByHorizon[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_MARINE)]
                                                                [HorizonIndexValue] +
            ProductionStateDescriptorValue.UnitCompletionAveragesByHorizon[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_MARAUDER)]
                                                                [HorizonIndexValue] +
            ProductionStateDescriptorValue.UnitCompletionAveragesByHorizon[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_REAPER)]
                                                                [HorizonIndexValue] +
            ProductionStateDescriptorValue.UnitCompletionAveragesByHorizon[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_GHOST)]
                                                                [HorizonIndexValue];
        ProductionStateDescriptorValue.FactoryThroughputAveragesByHorizon[HorizonIndexValue] =
            ProductionStateDescriptorValue.UnitCompletionAveragesByHorizon[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_HELLION)]
                                                                [HorizonIndexValue] +
            ProductionStateDescriptorValue.UnitCompletionAveragesByHorizon[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_CYCLONE)]
                                                                [HorizonIndexValue] +
            ProductionStateDescriptorValue.UnitCompletionAveragesByHorizon[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_SIEGETANK)]
                                                                [HorizonIndexValue] +
            ProductionStateDescriptorValue.UnitCompletionAveragesByHorizon[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_WIDOWMINE)]
                                                                [HorizonIndexValue] +
            ProductionStateDescriptorValue.UnitCompletionAveragesByHorizon[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_THOR)]
                                                                [HorizonIndexValue];
        ProductionStateDescriptorValue.StarportThroughputAveragesByHorizon[HorizonIndexValue] =
            ProductionStateDescriptorValue.UnitCompletionAveragesByHorizon[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_MEDIVAC)]
                                                                [HorizonIndexValue] +
            ProductionStateDescriptorValue.UnitCompletionAveragesByHorizon[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_LIBERATOR)]
                                                                [HorizonIndexValue] +
            ProductionStateDescriptorValue.UnitCompletionAveragesByHorizon[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_VIKINGFIGHTER)]
                                                                [HorizonIndexValue] +
            ProductionStateDescriptorValue.UnitCompletionAveragesByHorizon[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_BANSHEE)]
                                                                [HorizonIndexValue] +
            ProductionStateDescriptorValue.UnitCompletionAveragesByHorizon[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_RAVEN)]
                                                                [HorizonIndexValue] +
            ProductionStateDescriptorValue.UnitCompletionAveragesByHorizon[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_BATTLECRUISER)]
                                                                [HorizonIndexValue];
    }
}

void PopulateEconomyState(const FAgentState& AgentStateValue, const FEconomyDomainState& EconomyDomainStateValue,
                          const FBuildPlanningState& BuildPlanningStateValue,
                          const FSchedulerOutlookDescriptor& SchedulerOutlookDescriptorValue,
                          FEconomyStateDescriptor& EconomyStateDescriptorValue)
{
    EconomyStateDescriptorValue.Reset();
    EconomyStateDescriptorValue.CurrentMinerals = AgentStateValue.Economy.Minerals;
    EconomyStateDescriptorValue.CurrentVespene = AgentStateValue.Economy.Vespene;
    EconomyStateDescriptorValue.CurrentSupplyUsed = AgentStateValue.Economy.Supply;
    EconomyStateDescriptorValue.CurrentSupplyCap = AgentStateValue.Economy.SupplyCap;
    EconomyStateDescriptorValue.CurrentSupplyAvailable = AgentStateValue.Economy.SupplyAvailable;
    EconomyStateDescriptorValue.ReservedMinerals = BuildPlanningStateValue.ReservedMinerals;
    EconomyStateDescriptorValue.ReservedVespene = BuildPlanningStateValue.ReservedVespene;
    EconomyStateDescriptorValue.ReservedSupply = BuildPlanningStateValue.ReservedSupply;
    EconomyStateDescriptorValue.CommittedMinerals = BuildPlanningStateValue.CommittedMinerals;
    EconomyStateDescriptorValue.CommittedVespene = BuildPlanningStateValue.CommittedVespene;
    EconomyStateDescriptorValue.CommittedSupply = BuildPlanningStateValue.CommittedSupply;
    EconomyStateDescriptorValue.BudgetedMinerals =
        EconomyStateDescriptorValue.CurrentMinerals >
                (EconomyStateDescriptorValue.ReservedMinerals + EconomyStateDescriptorValue.CommittedMinerals)
            ? EconomyStateDescriptorValue.CurrentMinerals -
                  (EconomyStateDescriptorValue.ReservedMinerals + EconomyStateDescriptorValue.CommittedMinerals)
            : 0U;
    EconomyStateDescriptorValue.BudgetedVespene =
        EconomyStateDescriptorValue.CurrentVespene >
                (EconomyStateDescriptorValue.ReservedVespene + EconomyStateDescriptorValue.CommittedVespene)
            ? EconomyStateDescriptorValue.CurrentVespene -
                  (EconomyStateDescriptorValue.ReservedVespene + EconomyStateDescriptorValue.CommittedVespene)
            : 0U;
    EconomyStateDescriptorValue.BudgetedSupplyAvailable =
        EconomyStateDescriptorValue.CurrentSupplyAvailable >
                (EconomyStateDescriptorValue.ReservedSupply + EconomyStateDescriptorValue.CommittedSupply)
            ? EconomyStateDescriptorValue.CurrentSupplyAvailable -
                  (EconomyStateDescriptorValue.ReservedSupply + EconomyStateDescriptorValue.CommittedSupply)
            : 0U;

    const uint32_t ObservedInProgressSupplyCapDeltaValue =
        (GetSupplyCapContributionForBuildingType(UNIT_TYPEID::TERRAN_SUPPLYDEPOT) *
         GetBuildingCountFromObservedArray(BuildPlanningStateValue.ObservedBuildingsInConstruction,
                                           UNIT_TYPEID::TERRAN_SUPPLYDEPOT)) +
        (GetSupplyCapContributionForBuildingType(UNIT_TYPEID::TERRAN_COMMANDCENTER) *
         static_cast<uint32_t>(BuildPlanningStateValue.ObservedBuildingsInConstruction[GetTerranBuildingTypeIndex(
             UNIT_TYPEID::TERRAN_COMMANDCENTER)]));

    for (size_t HorizonIndexValue = 0U; HorizonIndexValue < ForecastHorizonCountValue; ++HorizonIndexValue)
    {
        const uint64_t ElapsedGameLoopsValue =
            EconomyDomainStateValue.GetElapsedGameLoopsForHorizon(HorizonIndexValue);
        const uint64_t GrossMineralIncomeValue =
            EconomyDomainStateValue.GetGrossMineralIncomeForHorizon(HorizonIndexValue);
        const uint64_t GrossVespeneIncomeValue =
            EconomyDomainStateValue.GetGrossVespeneIncomeForHorizon(HorizonIndexValue);
        const int64_t NetMineralDeltaValue =
            EconomyDomainStateValue.GetNetMineralDeltaForHorizon(HorizonIndexValue);
        const int64_t NetVespeneDeltaValue =
            EconomyDomainStateValue.GetNetVespeneDeltaForHorizon(HorizonIndexValue);
        const float GrossMineralIncomeAverageValue =
            ElapsedGameLoopsValue > 0U
                ? static_cast<float>(GrossMineralIncomeValue) / static_cast<float>(ElapsedGameLoopsValue)
                : 0.0f;
        const float GrossVespeneIncomeAverageValue =
            ElapsedGameLoopsValue > 0U
                ? static_cast<float>(GrossVespeneIncomeValue) / static_cast<float>(ElapsedGameLoopsValue)
                : 0.0f;
        const float NetMineralDeltaAverageValue =
            ElapsedGameLoopsValue > 0U ? static_cast<float>(NetMineralDeltaValue) /
                                             static_cast<float>(ElapsedGameLoopsValue)
                                       : 0.0f;
        const float NetVespeneDeltaAverageValue =
            ElapsedGameLoopsValue > 0U ? static_cast<float>(NetVespeneDeltaValue) /
                                             static_cast<float>(ElapsedGameLoopsValue)
                                       : 0.0f;
        const uint64_t HorizonGameLoopsValue = ForecastHorizonGameLoopsValue[HorizonIndexValue];
        const uint64_t ProjectedGrossMineralIncomeValue =
            RoundPositiveFloatToUint64(GrossMineralIncomeAverageValue * static_cast<float>(HorizonGameLoopsValue));
        const uint64_t ProjectedGrossVespeneIncomeValue =
            RoundPositiveFloatToUint64(GrossVespeneIncomeAverageValue * static_cast<float>(HorizonGameLoopsValue));
        const uint64_t ProjectedMineralsValue = static_cast<uint64_t>(EconomyStateDescriptorValue.CurrentMinerals) +
                                                ProjectedGrossMineralIncomeValue;
        const uint64_t ProjectedVespeneValue = static_cast<uint64_t>(EconomyStateDescriptorValue.CurrentVespene) +
                                               ProjectedGrossVespeneIncomeValue;
        const uint64_t ProjectedSupplyAvailableValue =
            static_cast<uint64_t>(EconomyStateDescriptorValue.CurrentSupplyAvailable) +
            static_cast<uint64_t>(ObservedInProgressSupplyCapDeltaValue) +
            static_cast<uint64_t>(SchedulerOutlookDescriptorValue.ExpectedSupplyCapDelta);
        const uint64_t ProjectedSupplyDemandValue =
            static_cast<uint64_t>(SchedulerOutlookDescriptorValue.ExpectedSupplyUsedDelta);

        EconomyStateDescriptorValue.GrossMineralIncomeByHorizon[HorizonIndexValue] =
            static_cast<uint32_t>(GrossMineralIncomeValue);
        EconomyStateDescriptorValue.GrossVespeneIncomeByHorizon[HorizonIndexValue] =
            static_cast<uint32_t>(GrossVespeneIncomeValue);
        EconomyStateDescriptorValue.NetMineralDeltaByHorizon[HorizonIndexValue] =
            static_cast<int32_t>(NetMineralDeltaValue);
        EconomyStateDescriptorValue.NetVespeneDeltaByHorizon[HorizonIndexValue] =
            static_cast<int32_t>(NetVespeneDeltaValue);
        EconomyStateDescriptorValue.GrossMineralIncomeAverageByHorizon[HorizonIndexValue] =
            GrossMineralIncomeAverageValue;
        EconomyStateDescriptorValue.GrossVespeneIncomeAverageByHorizon[HorizonIndexValue] =
            GrossVespeneIncomeAverageValue;
        EconomyStateDescriptorValue.NetMineralDeltaAverageByHorizon[HorizonIndexValue] =
            NetMineralDeltaAverageValue;
        EconomyStateDescriptorValue.NetVespeneDeltaAverageByHorizon[HorizonIndexValue] =
            NetVespeneDeltaAverageValue;
        EconomyStateDescriptorValue.ProjectedMineralsByHorizon[HorizonIndexValue] =
            static_cast<uint32_t>(ProjectedMineralsValue);
        EconomyStateDescriptorValue.ProjectedVespeneByHorizon[HorizonIndexValue] =
            static_cast<uint32_t>(ProjectedVespeneValue);
        EconomyStateDescriptorValue.ProjectedSupplyAvailableByHorizon[HorizonIndexValue] =
            static_cast<uint32_t>(ProjectedSupplyAvailableValue > ProjectedSupplyDemandValue
                                      ? ProjectedSupplyAvailableValue - ProjectedSupplyDemandValue
                                      : 0U);
        EconomyStateDescriptorValue.ProjectedAvailableMineralsByHorizon[HorizonIndexValue] =
            ProjectedMineralsValue >
                    static_cast<uint64_t>(EconomyStateDescriptorValue.ReservedMinerals +
                                          EconomyStateDescriptorValue.CommittedMinerals)
                ? static_cast<uint32_t>(ProjectedMineralsValue -
                                        static_cast<uint64_t>(EconomyStateDescriptorValue.ReservedMinerals +
                                                              EconomyStateDescriptorValue.CommittedMinerals))
                : 0U;
        EconomyStateDescriptorValue.ProjectedAvailableVespeneByHorizon[HorizonIndexValue] =
            ProjectedVespeneValue >
                    static_cast<uint64_t>(EconomyStateDescriptorValue.ReservedVespene +
                                          EconomyStateDescriptorValue.CommittedVespene)
                ? static_cast<uint32_t>(ProjectedVespeneValue -
                                        static_cast<uint64_t>(EconomyStateDescriptorValue.ReservedVespene +
                                                              EconomyStateDescriptorValue.CommittedVespene))
                : 0U;
        EconomyStateDescriptorValue.ProjectedAvailableSupplyByHorizon[HorizonIndexValue] =
            EconomyStateDescriptorValue.ProjectedSupplyAvailableByHorizon[HorizonIndexValue] >
                    (EconomyStateDescriptorValue.ReservedSupply + EconomyStateDescriptorValue.CommittedSupply)
                ? EconomyStateDescriptorValue.ProjectedSupplyAvailableByHorizon[HorizonIndexValue] -
                      (EconomyStateDescriptorValue.ReservedSupply + EconomyStateDescriptorValue.CommittedSupply)
                : 0U;
    }
}

}  // namespace


void FTerranForecastStateBuilder::RebuildForecastState(const FAgentState& AgentStateValue,
                                                       FEconomyDomainState& EconomyDomainStateValue,
                                                       FGameStateDescriptor& GameStateDescriptorValue) const
{
    EconomyDomainStateValue.Update(AgentStateValue, GameStateDescriptorValue.CurrentGameLoop);
    PopulateSchedulerOutlook(GameStateDescriptorValue.BuildPlanning,
                             GameStateDescriptorValue.CommandAuthoritySchedulingState,
                             GameStateDescriptorValue.SchedulerOutlook);
    PopulateProductionState(AgentStateValue, EconomyDomainStateValue, GameStateDescriptorValue.BuildPlanning,
                            GameStateDescriptorValue.CommandAuthoritySchedulingState,
                            GameStateDescriptorValue.SchedulerOutlook, GameStateDescriptorValue.ProductionState);
    PopulateEconomyState(AgentStateValue, EconomyDomainStateValue, GameStateDescriptorValue.BuildPlanning,
                         GameStateDescriptorValue.SchedulerOutlook, GameStateDescriptorValue.EconomyState);
}

}  // namespace sc2
