#include "common/services/FTerranEconomicService.h"

#include "common/build_planning/FBuildPlanningState.h"
#include "common/bot_status_models.h"
#include "common/descriptors/FEconomicCommitmentLedgerDescriptor.h"
#include "common/descriptors/FEconomyStateDescriptor.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/economic_models.h"
#include "common/planning/FBlockedTaskRingBuffer.h"
#include "common/planning/FCommandAuthoritySchedulingState.h"

namespace sc2
{
namespace
{

struct FCommandResourceCost
{
    uint32_t Minerals;
    uint32_t Vespene;
    uint32_t Supply;
};

FCommandResourceCost GetCommandResourceCostFromOrder(const FCommandOrderRecord& CommandOrderRecordValue)
{
    switch (CommandOrderRecordValue.AbilityId.ToType())
    {
        case ABILITY_ID::MORPH_ORBITALCOMMAND:
            return {150U, 0U, 0U};
        default:
            break;
    }

    if (CommandOrderRecordValue.UpgradeId.ToType() != UPGRADE_ID::INVALID)
    {
        const FUpgradeCostData& UpgradeCostDataValue =
            TERRAN_ECONOMIC_DATA.GetUpgradeCostData(CommandOrderRecordValue.AbilityId);
        return {UpgradeCostDataValue.CostData.Minerals, UpgradeCostDataValue.CostData.Vespine, 0U};
    }

    if (CommandOrderRecordValue.ResultUnitTypeId == UNIT_TYPEID::INVALID)
    {
        return {0U, 0U, 0U};
    }

    if (IsTerranBuilding(CommandOrderRecordValue.ResultUnitTypeId) ||
        CommandOrderRecordValue.ResultUnitTypeId == UNIT_TYPEID::TERRAN_COMMANDCENTER)
    {
        const FBuildingCostData& BuildingCostDataValue =
            TERRAN_ECONOMIC_DATA.GetBuildingCostData(CommandOrderRecordValue.ResultUnitTypeId);
        return {BuildingCostDataValue.CostData.Minerals, BuildingCostDataValue.CostData.Vespine, 0U};
    }

    const FUnitCostData& UnitCostDataValue = TERRAN_ECONOMIC_DATA.GetUnitCostData(CommandOrderRecordValue.ResultUnitTypeId);
    return {UnitCostDataValue.CostData.Minerals, UnitCostDataValue.CostData.Vespine, UnitCostDataValue.CostData.Supply};
}

FCommandResourceCost GetCommandResourceCostFromBlockedTask(const FBlockedTaskRecord& BlockedTaskRecordValue)
{
    FCommandOrderRecord CommandOrderRecordValue;
    CommandOrderRecordValue.AbilityId = BlockedTaskRecordValue.AbilityId;
    CommandOrderRecordValue.ResultUnitTypeId = BlockedTaskRecordValue.ResultUnitTypeId;
    CommandOrderRecordValue.UpgradeId = BlockedTaskRecordValue.UpgradeId;
    return GetCommandResourceCostFromOrder(CommandOrderRecordValue);
}

bool ShouldCountCommittedBudgetForOrder(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                        const size_t OrderIndexValue)
{
    if (IsTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
    {
        return false;
    }

    switch (CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue])
    {
        case ECommandAuthorityLayer::Army:
        case ECommandAuthorityLayer::Squad:
        case ECommandAuthorityLayer::UnitExecution:
            return false;
        default:
            break;
    }

    if (CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] == ECommandAuthorityLayer::EconomyAndProduction &&
        CommandAuthoritySchedulingStateValue.ParentOrderIds[OrderIndexValue] != 0U)
    {
        size_t ParentOrderIndexValue = 0U;
        if (CommandAuthoritySchedulingStateValue.TryGetOrderIndex(
                CommandAuthoritySchedulingStateValue.ParentOrderIds[OrderIndexValue], ParentOrderIndexValue) &&
            !IsTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[ParentOrderIndexValue]))
        {
            return false;
        }
    }

    return true;
}

void AddProducerOccupancyForProducerType(FEconomicCommitmentLedgerDescriptor& EconomicCommitmentLedgerDescriptorValue,
                                         const ECommandCommitmentClass CommandCommitmentClassValue,
                                         const UNIT_TYPEID ProducerUnitTypeIdValue)
{
    switch (ProducerUnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
            EconomicCommitmentLedgerDescriptorValue.AddProducerOccupancy(CommandCommitmentClassValue, 1U, 0U, 0U, 0U);
            break;
        case UNIT_TYPEID::TERRAN_BARRACKS:
        case UNIT_TYPEID::TERRAN_BARRACKSREACTOR:
        case UNIT_TYPEID::TERRAN_BARRACKSTECHLAB:
            EconomicCommitmentLedgerDescriptorValue.AddProducerOccupancy(CommandCommitmentClassValue, 0U, 1U, 0U, 0U);
            break;
        case UNIT_TYPEID::TERRAN_FACTORY:
        case UNIT_TYPEID::TERRAN_FACTORYREACTOR:
        case UNIT_TYPEID::TERRAN_FACTORYTECHLAB:
            EconomicCommitmentLedgerDescriptorValue.AddProducerOccupancy(CommandCommitmentClassValue, 0U, 0U, 1U, 0U);
            break;
        case UNIT_TYPEID::TERRAN_STARPORT:
        case UNIT_TYPEID::TERRAN_STARPORTREACTOR:
        case UNIT_TYPEID::TERRAN_STARPORTTECHLAB:
            EconomicCommitmentLedgerDescriptorValue.AddProducerOccupancy(CommandCommitmentClassValue, 0U, 0U, 0U, 1U);
            break;
        default:
            break;
    }
}

void AccumulateCommittedOrderBudget(const FCommandOrderRecord& CommandOrderRecordValue,
                                    FEconomicCommitmentLedgerDescriptor& EconomicCommitmentLedgerDescriptorValue,
                                    FBuildPlanningState& BuildPlanningStateValue)
{
    const FCommandResourceCost CommandResourceCostValue = GetCommandResourceCostFromOrder(CommandOrderRecordValue);
    EconomicCommitmentLedgerDescriptorValue.AddCommittedResources(
        CommandOrderRecordValue.CommitmentClass, CommandResourceCostValue.Minerals, CommandResourceCostValue.Vespene,
        CommandResourceCostValue.Supply);
    BuildPlanningStateValue.AddCommittedResources(CommandOrderRecordValue.CommitmentClass, CommandResourceCostValue.Minerals,
                                                  CommandResourceCostValue.Vespene, CommandResourceCostValue.Supply);

    AddProducerOccupancyForProducerType(EconomicCommitmentLedgerDescriptorValue, CommandOrderRecordValue.CommitmentClass,
                                        CommandOrderRecordValue.ProducerUnitTypeId);
}

void AccumulateBlockedTaskBudget(const FBlockedTaskRecord& BlockedTaskRecordValue,
                                 FEconomicCommitmentLedgerDescriptor& EconomicCommitmentLedgerDescriptorValue,
                                 FBuildPlanningState& BuildPlanningStateValue)
{
    const FCommandResourceCost CommandResourceCostValue = GetCommandResourceCostFromBlockedTask(BlockedTaskRecordValue);
    EconomicCommitmentLedgerDescriptorValue.AddCommittedResources(
        BlockedTaskRecordValue.CommitmentClass, CommandResourceCostValue.Minerals, CommandResourceCostValue.Vespene,
        CommandResourceCostValue.Supply);
    BuildPlanningStateValue.AddCommittedResources(BlockedTaskRecordValue.CommitmentClass, CommandResourceCostValue.Minerals,
                                                  CommandResourceCostValue.Vespene, CommandResourceCostValue.Supply);

    AddProducerOccupancyForProducerType(EconomicCommitmentLedgerDescriptorValue, BlockedTaskRecordValue.CommitmentClass,
                                        BlockedTaskRecordValue.ProducerUnitTypeId);
}

void AccumulateBlockedRingBufferBudget(const FBlockedTaskRingBuffer& BlockedTaskRingBufferValue,
                                       FEconomicCommitmentLedgerDescriptor& EconomicCommitmentLedgerDescriptorValue,
                                       FBuildPlanningState& BuildPlanningStateValue)
{
    const size_t BlockedRecordCountValue = BlockedTaskRingBufferValue.GetCount();
    for (size_t BlockedRecordIndexValue = 0U; BlockedRecordIndexValue < BlockedRecordCountValue;
         ++BlockedRecordIndexValue)
    {
        const FBlockedTaskRecord* BlockedTaskRecordPtrValue =
            BlockedTaskRingBufferValue.GetRecordAtOrderedIndex(BlockedRecordIndexValue);
        if (BlockedTaskRecordPtrValue == nullptr)
        {
            continue;
        }

        AccumulateBlockedTaskBudget(*BlockedTaskRecordPtrValue, EconomicCommitmentLedgerDescriptorValue,
                                    BuildPlanningStateValue);
    }
}

uint32_t SubtractBudgetFloor(const uint32_t TotalValue, const uint32_t ConsumedValue)
{
    return TotalValue > ConsumedValue ? TotalValue - ConsumedValue : 0U;
}

}  // namespace

void FTerranEconomicService::RebuildEconomicState(const FAgentState& AgentStateValue,
                                                  const FEconomyDomainState& EconomyDomainStateValue,
                                                  FGameStateDescriptor& GameStateDescriptorValue) const
{
    (void)EconomyDomainStateValue;

    FEconomicCommitmentLedgerDescriptor& EconomicCommitmentLedgerDescriptorValue =
        GameStateDescriptorValue.CommitmentLedger;
    FEconomyStateDescriptor& EconomyStateDescriptorValue = GameStateDescriptorValue.EconomyState;
    FBuildPlanningState& BuildPlanningStateValue = GameStateDescriptorValue.BuildPlanning;
    const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState;

    EconomicCommitmentLedgerDescriptorValue.Reset();
    BuildPlanningStateValue.ResetCommittedResources();

    for (size_t CommitmentClassIndexValue = 0U; CommitmentClassIndexValue < CommandCommitmentClassCountValue;
         ++CommitmentClassIndexValue)
    {
        const ECommandCommitmentClass CommandCommitmentClassValue =
            static_cast<ECommandCommitmentClass>(CommitmentClassIndexValue);
        EconomicCommitmentLedgerDescriptorValue.AddReservedResources(
            CommandCommitmentClassValue, BuildPlanningStateValue.ReservedMineralsByCommitmentClass[CommitmentClassIndexValue],
            BuildPlanningStateValue.ReservedVespeneByCommitmentClass[CommitmentClassIndexValue],
            BuildPlanningStateValue.ReservedSupplyByCommitmentClass[CommitmentClassIndexValue]);
    }

    const size_t OrderCountValue = CommandAuthoritySchedulingStateValue.OrderIds.size();
    for (size_t OrderIndexValue = 0U; OrderIndexValue < OrderCountValue; ++OrderIndexValue)
    {
        if (!ShouldCountCommittedBudgetForOrder(CommandAuthoritySchedulingStateValue, OrderIndexValue))
        {
            continue;
        }

        const FCommandOrderRecord CommandOrderRecordValue =
            CommandAuthoritySchedulingStateValue.GetOrderRecord(OrderIndexValue);
        AccumulateCommittedOrderBudget(CommandOrderRecordValue, EconomicCommitmentLedgerDescriptorValue,
                                       BuildPlanningStateValue);
    }

    AccumulateBlockedRingBufferBudget(CommandAuthoritySchedulingStateValue.BlockedStrategicTasks,
                                      EconomicCommitmentLedgerDescriptorValue, BuildPlanningStateValue);
    AccumulateBlockedRingBufferBudget(CommandAuthoritySchedulingStateValue.BlockedPlanningTasks,
                                      EconomicCommitmentLedgerDescriptorValue, BuildPlanningStateValue);

    BuildPlanningStateValue.RebuildAggregateCommitmentTotals();

    const uint32_t MandatoryReservedMineralsValue =
        EconomicCommitmentLedgerDescriptorValue.GetReservedMinerals(ECommandCommitmentClass::MandatoryOpening) +
        EconomicCommitmentLedgerDescriptorValue.GetReservedMinerals(ECommandCommitmentClass::MandatoryRecovery);
    const uint32_t MandatoryReservedVespeneValue =
        EconomicCommitmentLedgerDescriptorValue.GetReservedVespene(ECommandCommitmentClass::MandatoryOpening) +
        EconomicCommitmentLedgerDescriptorValue.GetReservedVespene(ECommandCommitmentClass::MandatoryRecovery);
    const uint32_t MandatoryReservedSupplyValue =
        EconomicCommitmentLedgerDescriptorValue.GetReservedSupply(ECommandCommitmentClass::MandatoryOpening) +
        EconomicCommitmentLedgerDescriptorValue.GetReservedSupply(ECommandCommitmentClass::MandatoryRecovery);
    const uint32_t MandatoryCommittedMineralsValue =
        EconomicCommitmentLedgerDescriptorValue.GetCommittedMinerals(ECommandCommitmentClass::MandatoryOpening) +
        EconomicCommitmentLedgerDescriptorValue.GetCommittedMinerals(ECommandCommitmentClass::MandatoryRecovery);
    const uint32_t MandatoryCommittedVespeneValue =
        EconomicCommitmentLedgerDescriptorValue.GetCommittedVespene(ECommandCommitmentClass::MandatoryOpening) +
        EconomicCommitmentLedgerDescriptorValue.GetCommittedVespene(ECommandCommitmentClass::MandatoryRecovery);
    const uint32_t MandatoryCommittedSupplyValue =
        EconomicCommitmentLedgerDescriptorValue.GetCommittedSupply(ECommandCommitmentClass::MandatoryOpening) +
        EconomicCommitmentLedgerDescriptorValue.GetCommittedSupply(ECommandCommitmentClass::MandatoryRecovery);
    const uint32_t FlexibleReservedMineralsValue =
        EconomicCommitmentLedgerDescriptorValue.GetReservedMinerals(ECommandCommitmentClass::FlexibleMacro) +
        EconomicCommitmentLedgerDescriptorValue.GetReservedMinerals(ECommandCommitmentClass::Opportunistic);
    const uint32_t FlexibleReservedVespeneValue =
        EconomicCommitmentLedgerDescriptorValue.GetReservedVespene(ECommandCommitmentClass::FlexibleMacro) +
        EconomicCommitmentLedgerDescriptorValue.GetReservedVespene(ECommandCommitmentClass::Opportunistic);
    const uint32_t FlexibleReservedSupplyValue =
        EconomicCommitmentLedgerDescriptorValue.GetReservedSupply(ECommandCommitmentClass::FlexibleMacro) +
        EconomicCommitmentLedgerDescriptorValue.GetReservedSupply(ECommandCommitmentClass::Opportunistic);
    const uint32_t FlexibleCommittedMineralsValue =
        EconomicCommitmentLedgerDescriptorValue.GetCommittedMinerals(ECommandCommitmentClass::FlexibleMacro) +
        EconomicCommitmentLedgerDescriptorValue.GetCommittedMinerals(ECommandCommitmentClass::Opportunistic);
    const uint32_t FlexibleCommittedVespeneValue =
        EconomicCommitmentLedgerDescriptorValue.GetCommittedVespene(ECommandCommitmentClass::FlexibleMacro) +
        EconomicCommitmentLedgerDescriptorValue.GetCommittedVespene(ECommandCommitmentClass::Opportunistic);
    const uint32_t FlexibleCommittedSupplyValue =
        EconomicCommitmentLedgerDescriptorValue.GetCommittedSupply(ECommandCommitmentClass::FlexibleMacro) +
        EconomicCommitmentLedgerDescriptorValue.GetCommittedSupply(ECommandCommitmentClass::Opportunistic);

    EconomyStateDescriptorValue.ReservedMinerals = MandatoryReservedMineralsValue + FlexibleReservedMineralsValue;
    EconomyStateDescriptorValue.ReservedVespene = MandatoryReservedVespeneValue + FlexibleReservedVespeneValue;
    EconomyStateDescriptorValue.ReservedSupply = MandatoryReservedSupplyValue + FlexibleReservedSupplyValue;
    EconomyStateDescriptorValue.CommittedMinerals = MandatoryCommittedMineralsValue + FlexibleCommittedMineralsValue;
    EconomyStateDescriptorValue.CommittedVespene = MandatoryCommittedVespeneValue + FlexibleCommittedVespeneValue;
    EconomyStateDescriptorValue.CommittedSupply = MandatoryCommittedSupplyValue + FlexibleCommittedSupplyValue;
    EconomyStateDescriptorValue.BudgetedMinerals =
        SubtractBudgetFloor(AgentStateValue.Economy.Minerals, MandatoryReservedMineralsValue + MandatoryCommittedMineralsValue);
    EconomyStateDescriptorValue.BudgetedVespene =
        SubtractBudgetFloor(AgentStateValue.Economy.Vespene, MandatoryReservedVespeneValue + MandatoryCommittedVespeneValue);
    EconomyStateDescriptorValue.BudgetedSupplyAvailable = SubtractBudgetFloor(
        AgentStateValue.Economy.SupplyAvailable, MandatoryReservedSupplyValue + MandatoryCommittedSupplyValue);

    for (size_t HorizonIndexValue = 0U; HorizonIndexValue < ForecastHorizonCountValue; ++HorizonIndexValue)
    {
        const uint32_t ProjectedAvailableMineralsValue =
            EconomyStateDescriptorValue.ProjectedMineralsByHorizon[HorizonIndexValue];
        const uint32_t ProjectedAvailableVespeneValue =
            EconomyStateDescriptorValue.ProjectedVespeneByHorizon[HorizonIndexValue];
        const uint32_t ProjectedAvailableSupplyValue =
            EconomyStateDescriptorValue.ProjectedSupplyAvailableByHorizon[HorizonIndexValue];

        EconomicCommitmentLedgerDescriptorValue.ProjectedAvailableAfterMandatoryMineralsByHorizon[HorizonIndexValue] =
            SubtractBudgetFloor(ProjectedAvailableMineralsValue,
                                MandatoryReservedMineralsValue + MandatoryCommittedMineralsValue);
        EconomicCommitmentLedgerDescriptorValue.ProjectedAvailableAfterMandatoryVespeneByHorizon[HorizonIndexValue] =
            SubtractBudgetFloor(ProjectedAvailableVespeneValue,
                                MandatoryReservedVespeneValue + MandatoryCommittedVespeneValue);
        EconomicCommitmentLedgerDescriptorValue.ProjectedAvailableAfterMandatorySupplyByHorizon[HorizonIndexValue] =
            SubtractBudgetFloor(ProjectedAvailableSupplyValue,
                                MandatoryReservedSupplyValue + MandatoryCommittedSupplyValue);
        EconomicCommitmentLedgerDescriptorValue.ProjectedDiscretionaryMineralsByHorizon[HorizonIndexValue] =
            SubtractBudgetFloor(ProjectedAvailableMineralsValue,
                                MandatoryReservedMineralsValue + MandatoryCommittedMineralsValue +
                                    FlexibleReservedMineralsValue + FlexibleCommittedMineralsValue);
        EconomicCommitmentLedgerDescriptorValue.ProjectedDiscretionaryVespeneByHorizon[HorizonIndexValue] =
            SubtractBudgetFloor(ProjectedAvailableVespeneValue,
                                MandatoryReservedVespeneValue + MandatoryCommittedVespeneValue +
                                    FlexibleReservedVespeneValue + FlexibleCommittedVespeneValue);
        EconomicCommitmentLedgerDescriptorValue.ProjectedDiscretionarySupplyByHorizon[HorizonIndexValue] =
            SubtractBudgetFloor(ProjectedAvailableSupplyValue,
                                MandatoryReservedSupplyValue + MandatoryCommittedSupplyValue +
                                    FlexibleReservedSupplyValue + FlexibleCommittedSupplyValue);

        EconomyStateDescriptorValue.ProjectedAvailableMineralsByHorizon[HorizonIndexValue] =
            EconomicCommitmentLedgerDescriptorValue.ProjectedAvailableAfterMandatoryMineralsByHorizon[HorizonIndexValue];
        EconomyStateDescriptorValue.ProjectedAvailableVespeneByHorizon[HorizonIndexValue] =
            EconomicCommitmentLedgerDescriptorValue.ProjectedAvailableAfterMandatoryVespeneByHorizon[HorizonIndexValue];
        EconomyStateDescriptorValue.ProjectedAvailableSupplyByHorizon[HorizonIndexValue] =
            EconomicCommitmentLedgerDescriptorValue.ProjectedAvailableAfterMandatorySupplyByHorizon[HorizonIndexValue];
    }
}

bool FTerranEconomicService::CanReserveMandatory(const FGameStateDescriptor& GameStateDescriptorValue,
                                                 const uint32_t MineralsCostValue, const uint32_t VespeneCostValue,
                                                 const uint32_t SupplyCostValue) const
{
    return GameStateDescriptorValue.EconomyState.BudgetedMinerals >= MineralsCostValue &&
           GameStateDescriptorValue.EconomyState.BudgetedVespene >= VespeneCostValue &&
           GameStateDescriptorValue.EconomyState.BudgetedSupplyAvailable >= SupplyCostValue;
}

bool FTerranEconomicService::CanReserveFlexible(const FGameStateDescriptor& GameStateDescriptorValue,
                                                const uint32_t MineralsCostValue, const uint32_t VespeneCostValue,
                                                const uint32_t SupplyCostValue) const
{
    return GameStateDescriptorValue.CommitmentLedger.GetProjectedDiscretionaryMinerals(ShortForecastHorizonIndexValue) >=
               MineralsCostValue &&
           GameStateDescriptorValue.CommitmentLedger.GetProjectedDiscretionaryVespene(ShortForecastHorizonIndexValue) >=
               VespeneCostValue &&
           GameStateDescriptorValue.CommitmentLedger.GetProjectedDiscretionarySupply(ShortForecastHorizonIndexValue) >=
               SupplyCostValue;
}

}  // namespace sc2
