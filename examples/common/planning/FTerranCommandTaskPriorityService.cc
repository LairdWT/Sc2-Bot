#include "common/planning/FTerranCommandTaskPriorityService.h"

#include <algorithm>

#include "common/armies/EArmyMissionType.h"
#include "common/descriptors/FMacroStateDescriptor.h"
#include "common/economy/EconomyForecastConstants.h"
#include "common/planning/FCommandAuthoritySchedulingState.h"

namespace sc2
{
namespace
{

uint32_t GetWorkerSaturationTargetCount(const FMacroStateDescriptor& MacroStateDescriptorValue)
{
    const uint32_t TargetWorkerCountValue =
        (std::max<uint32_t>(1U, MacroStateDescriptorValue.DesiredBaseCount) * 16U);
    return std::min<uint32_t>(72U, TargetWorkerCountValue);
}

bool IsExpansionEconomicallyStable(const FGameStateDescriptor& GameStateDescriptorValue)
{
    const FEconomyStateDescriptor& EconomyStateDescriptorValue = GameStateDescriptorValue.EconomyState;
    const FProductionStateDescriptor& ProductionStateDescriptorValue = GameStateDescriptorValue.ProductionState;
    const uint32_t ProjectedBaseCountValue =
        ProductionStateDescriptorValue.GetProjectedBuildingCount(UNIT_TYPEID::TERRAN_COMMANDCENTER);
    const uint32_t ProjectedWorkerCountValue =
        ProductionStateDescriptorValue.GetProjectedUnitCount(UNIT_TYPEID::TERRAN_SCV);
    return EconomyStateDescriptorValue.ProjectedAvailableMineralsByHorizon[ShortForecastHorizonIndexValue] >= 300U &&
           ProjectedWorkerCountValue >= (std::max<uint32_t>(1U, ProjectedBaseCountValue) * 14U);
}

bool IsDefenseEmergency(const FGameStateDescriptor& GameStateDescriptorValue)
{
    if (GameStateDescriptorValue.ArmyState.ArmyMissions.empty())
    {
        return false;
    }

    switch (GameStateDescriptorValue.ArmyState.ArmyMissions.front().MissionType)
    {
        case EArmyMissionType::DefendOwnedBase:
        case EArmyMissionType::Regroup:
            return true;
        default:
            return false;
    }
}

bool IsOpeningIncomplete(const FGameStateDescriptor& GameStateDescriptorValue)
{
    return GameStateDescriptorValue.OpeningPlanExecutionState.LifecycleState != EOpeningPlanLifecycleState::Completed;
}

bool IsOpeningBlendLimitedGoalTaskType(const ECommandTaskType CommandTaskTypeValue)
{
    switch (CommandTaskTypeValue)
    {
        case ECommandTaskType::AddOn:
        case ECommandTaskType::UpgradeResearch:
        case ECommandTaskType::UnitProduction:
            return true;
        default:
            return false;
    }
}

bool HasImminentSupplyRelief(const FGameStateDescriptor& GameStateDescriptorValue)
{
    return GameStateDescriptorValue.SchedulerOutlook.ExpectedSupplyAvailableDelta > 0;
}

bool IsProjectedSupplyEmergency(const FGameStateDescriptor& GameStateDescriptorValue)
{
    return GameStateDescriptorValue.EconomyState.ProjectedAvailableSupplyByHorizon[ShortForecastHorizonIndexValue] <= 1U &&
           !HasImminentSupplyRelief(GameStateDescriptorValue);
}

bool HasProjectedProductionBottleneck(const FGameStateDescriptor& GameStateDescriptorValue)
{
    const FProductionStateDescriptor& ProductionStateDescriptorValue = GameStateDescriptorValue.ProductionState;
    const uint32_t CurrentCombatProducerCapacityValue = ProductionStateDescriptorValue.CurrentBarracksCapacity +
                                                        ProductionStateDescriptorValue.CurrentFactoryCapacity +
                                                        ProductionStateDescriptorValue.CurrentStarportCapacity;
    const uint32_t CurrentCombatProducerOccupancyValue = ProductionStateDescriptorValue.CurrentBarracksOccupancy +
                                                         ProductionStateDescriptorValue.CurrentFactoryOccupancy +
                                                         ProductionStateDescriptorValue.CurrentStarportOccupancy;
    return CurrentCombatProducerCapacityValue > 0U &&
           CurrentCombatProducerOccupancyValue >= CurrentCombatProducerCapacityValue;
}

bool HasSustainedMineralFloat(const FExecutionPressureDescriptor& ExecutionPressureDescriptorValue)
{
    if (ExecutionPressureDescriptorValue.MineralBankState != EExecutionConditionState::Active)
    {
        return false;
    }

    return ExecutionPressureDescriptorValue.CurrentMineralBankAmount >= 800U ||
           (ExecutionPressureDescriptorValue.CurrentMineralBankAmount >= 500U &&
            ExecutionPressureDescriptorValue.MineralBankDurationGameLoops >=
                ForecastHorizonGameLoopsValue[ShortForecastHorizonIndexValue]);
}

bool HasIdleTownHallPressure(const FExecutionPressureDescriptor& ExecutionPressureDescriptorValue)
{
    return ExecutionPressureDescriptorValue.IdleTownHallCount > 0U;
}

bool HasIdleCombatProductionPressure(const FExecutionPressureDescriptor& ExecutionPressureDescriptorValue)
{
    return ExecutionPressureDescriptorValue.GetIdleCombatProductionStructureCount() > 0U ||
           ExecutionPressureDescriptorValue.RecentIdleProductionConflictCount > 0U;
}

ECommandPriorityTier ApplyOpeningBlendPriorityTierClamp(
    const FGameStateDescriptor& GameStateDescriptorValue, const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
    const size_t OrderIndexValue, const ECommandPriorityTier PriorityTierValue)
{
    if (!IsOpeningIncomplete(GameStateDescriptorValue))
    {
        return PriorityTierValue;
    }

    switch (CommandAuthoritySchedulingStateValue.TaskOrigins[OrderIndexValue])
    {
        case ECommandTaskOrigin::Opening:
            switch (PriorityTierValue)
            {
                case ECommandPriorityTier::Low:
                case ECommandPriorityTier::Normal:
                    return ECommandPriorityTier::High;
                case ECommandPriorityTier::Critical:
                case ECommandPriorityTier::High:
                default:
                    return PriorityTierValue;
            }
        case ECommandTaskOrigin::GoalMacro:
            if (!IsOpeningBlendLimitedGoalTaskType(CommandAuthoritySchedulingStateValue.TaskTypes[OrderIndexValue]))
            {
                return PriorityTierValue;
            }

            switch (PriorityTierValue)
            {
                case ECommandPriorityTier::Critical:
                case ECommandPriorityTier::High:
                    return ECommandPriorityTier::Normal;
                case ECommandPriorityTier::Normal:
                case ECommandPriorityTier::Low:
                default:
                    return PriorityTierValue;
            }
        case ECommandTaskOrigin::Recovery:
        default:
            return PriorityTierValue;
    }
}

}  // namespace

void FTerranCommandTaskPriorityService::UpdateTaskPriorities(FGameStateDescriptor& GameStateDescriptorValue) const
{
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState;
    if (!CommandAuthoritySchedulingStateValue.bPrioritiesDirty)
    {
        return;
    }

    CommandAuthoritySchedulingStateValue.BeginMutationBatch();
    const size_t OrderCountValue = CommandAuthoritySchedulingStateValue.OrderIds.size();
    for (size_t OrderIndexValue = 0U; OrderIndexValue < OrderCountValue; ++OrderIndexValue)
    {
        const int FocusWeightValue =
            GetFocusWeight(GameStateDescriptorValue.MacroState.PrimaryProductionFocus,
                           CommandAuthoritySchedulingStateValue.TaskTypes[OrderIndexValue]);
        const int EffectivePriorityValue = CommandAuthoritySchedulingStateValue.BasePriorityValues[OrderIndexValue] +
                                           GetTaskTypeWeight(CommandAuthoritySchedulingStateValue.TaskTypes[OrderIndexValue]) +
                                           FocusWeightValue +
                                           GetEmergencyWeight(GameStateDescriptorValue,
                                                              CommandAuthoritySchedulingStateValue,
                                                              OrderIndexValue);
        CommandAuthoritySchedulingStateValue.EffectivePriorityValues[OrderIndexValue] = EffectivePriorityValue;
        const ECommandPriorityTier RawPriorityTierValue =
            DeterminePriorityTier(GameStateDescriptorValue, CommandAuthoritySchedulingStateValue, OrderIndexValue,
                                  EffectivePriorityValue);
        CommandAuthoritySchedulingStateValue.PriorityTiers[OrderIndexValue] =
            ApplyOpeningBlendPriorityTierClamp(GameStateDescriptorValue, CommandAuthoritySchedulingStateValue,
                                               OrderIndexValue, RawPriorityTierValue);
    }

    CommandAuthoritySchedulingStateValue.bDerivedQueuesDirty = true;
    CommandAuthoritySchedulingStateValue.bPrioritiesDirty = false;
    CommandAuthoritySchedulingStateValue.EndMutationBatch();
}

int FTerranCommandTaskPriorityService::GetTaskTypeWeight(const ECommandTaskType CommandTaskTypeValue) const
{
    switch (CommandTaskTypeValue)
    {
        case ECommandTaskType::Recovery:
            return 2000;
        case ECommandTaskType::ArmyMission:
            return 1400;
        case ECommandTaskType::Supply:
            return 1200;
        case ECommandTaskType::WorkerProduction:
            return 1000;
        case ECommandTaskType::Expansion:
            return 900;
        case ECommandTaskType::StaticDefense:
            return 850;
        case ECommandTaskType::Refinery:
            return 750;
        case ECommandTaskType::ProductionStructure:
            return 700;
        case ECommandTaskType::TechStructure:
            return 650;
        case ECommandTaskType::AddOn:
            return 600;
        case ECommandTaskType::UnitProduction:
            return 600;
        case ECommandTaskType::UpgradeResearch:
            return 500;
        case ECommandTaskType::Unknown:
        default:
            return 0;
    }
}

int FTerranCommandTaskPriorityService::GetFocusWeight(const EProductionFocus ProductionFocusValue,
                                                      const ECommandTaskType CommandTaskTypeValue) const
{
    switch (ProductionFocusValue)
    {
        case EProductionFocus::Recovery:
            switch (CommandTaskTypeValue)
            {
                case ECommandTaskType::Recovery:
                case ECommandTaskType::WorkerProduction:
                case ECommandTaskType::Supply:
                    return 400;
                case ECommandTaskType::Expansion:
                case ECommandTaskType::UpgradeResearch:
                    return -200;
                default:
                    return 0;
            }
        case EProductionFocus::Supply:
            return CommandTaskTypeValue == ECommandTaskType::Supply ? 400 : -100;
        case EProductionFocus::Workers:
            switch (CommandTaskTypeValue)
            {
                case ECommandTaskType::WorkerProduction:
                    return 400;
                case ECommandTaskType::Expansion:
                case ECommandTaskType::Refinery:
                    return 200;
                case ECommandTaskType::ArmyMission:
                    return -200;
                default:
                    return 0;
            }
        case EProductionFocus::Expansion:
            switch (CommandTaskTypeValue)
            {
                case ECommandTaskType::Expansion:
                    return 400;
                case ECommandTaskType::WorkerProduction:
                case ECommandTaskType::Refinery:
                    return 200;
                case ECommandTaskType::UpgradeResearch:
                    return -100;
                default:
                    return 0;
            }
        case EProductionFocus::Production:
            switch (CommandTaskTypeValue)
            {
                case ECommandTaskType::ProductionStructure:
                case ECommandTaskType::AddOn:
                case ECommandTaskType::Refinery:
                    return 400;
                case ECommandTaskType::TechStructure:
                case ECommandTaskType::UnitProduction:
                    return 200;
                default:
                    return 0;
            }
        case EProductionFocus::Army:
            switch (CommandTaskTypeValue)
            {
                case ECommandTaskType::ArmyMission:
                case ECommandTaskType::UnitProduction:
                    return 400;
                case ECommandTaskType::ProductionStructure:
                case ECommandTaskType::AddOn:
                    return 200;
                case ECommandTaskType::Expansion:
                    return -200;
                default:
                    return 0;
            }
        case EProductionFocus::Upgrades:
            switch (CommandTaskTypeValue)
            {
                case ECommandTaskType::UpgradeResearch:
                    return 400;
                case ECommandTaskType::TechStructure:
                case ECommandTaskType::AddOn:
                    return 200;
                default:
                    return 0;
            }
        case EProductionFocus::Defense:
            switch (CommandTaskTypeValue)
            {
                case ECommandTaskType::ArmyMission:
                case ECommandTaskType::StaticDefense:
                    return 400;
                case ECommandTaskType::UnitProduction:
                    return 200;
                case ECommandTaskType::Expansion:
                    return -200;
                default:
                    return 0;
            }
        default:
            return 0;
    }
}

int FTerranCommandTaskPriorityService::GetEmergencyWeight(
    const FGameStateDescriptor& GameStateDescriptorValue,
    const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue, const size_t OrderIndexValue) const
{
    int EmergencyWeightValue = 0;
    const FMacroStateDescriptor& MacroStateDescriptorValue = GameStateDescriptorValue.MacroState;
    const FEconomyStateDescriptor& EconomyStateDescriptorValue = GameStateDescriptorValue.EconomyState;
    const FProductionStateDescriptor& ProductionStateDescriptorValue = GameStateDescriptorValue.ProductionState;
    const FExecutionPressureDescriptor& ExecutionPressureDescriptorValue =
        GameStateDescriptorValue.ExecutionPressure;
    const ECommandTaskType CommandTaskTypeValue = CommandAuthoritySchedulingStateValue.TaskTypes[OrderIndexValue];
    const bool HasSustainedMineralFloatValue = HasSustainedMineralFloat(ExecutionPressureDescriptorValue);
    const bool HasIdleTownHallPressureValue = HasIdleTownHallPressure(ExecutionPressureDescriptorValue);
    const bool HasIdleCombatProductionPressureValue =
        HasIdleCombatProductionPressure(ExecutionPressureDescriptorValue);

    if (IsProjectedSupplyEmergency(GameStateDescriptorValue))
    {
        switch (CommandTaskTypeValue)
        {
            case ECommandTaskType::Supply:
                EmergencyWeightValue += 1200;
                break;
            case ECommandTaskType::WorkerProduction:
                EmergencyWeightValue -= 600;
                break;
            case ECommandTaskType::Recovery:
            case ECommandTaskType::ArmyMission:
                break;
            default:
                EmergencyWeightValue -= 300;
                break;
        }
    }

    if (MacroStateDescriptorValue.ActiveBaseCount < MacroStateDescriptorValue.DesiredBaseCount &&
        IsExpansionEconomicallyStable(GameStateDescriptorValue) &&
        CommandTaskTypeValue == ECommandTaskType::Expansion)
    {
        EmergencyWeightValue += 350;
    }

    if (ProductionStateDescriptorValue.GetProjectedUnitCount(UNIT_TYPEID::TERRAN_SCV) <
            GetWorkerSaturationTargetCount(MacroStateDescriptorValue) &&
        CommandTaskTypeValue == ECommandTaskType::WorkerProduction)
    {
        EmergencyWeightValue += 300;
    }

    if (IsDefenseEmergency(GameStateDescriptorValue))
    {
        switch (CommandTaskTypeValue)
        {
            case ECommandTaskType::ArmyMission:
            case ECommandTaskType::UnitProduction:
            case ECommandTaskType::StaticDefense:
                EmergencyWeightValue += 400;
                break;
            case ECommandTaskType::Expansion:
                EmergencyWeightValue -= 300;
                break;
            default:
                break;
        }
    }

    if (HasIdleTownHallPressureValue && CommandTaskTypeValue == ECommandTaskType::WorkerProduction)
    {
        EmergencyWeightValue += 350;
    }

    if (HasIdleCombatProductionPressureValue)
    {
        switch (CommandTaskTypeValue)
        {
            case ECommandTaskType::UnitProduction:
                EmergencyWeightValue += 425;
                break;
            case ECommandTaskType::AddOn:
                EmergencyWeightValue += 275;
                break;
            case ECommandTaskType::UpgradeResearch:
                EmergencyWeightValue += 175;
                break;
            default:
                break;
        }
    }

    if (HasSustainedMineralFloatValue)
    {
        switch (CommandTaskTypeValue)
        {
            case ECommandTaskType::WorkerProduction:
                if (ProductionStateDescriptorValue.GetProjectedUnitCount(UNIT_TYPEID::TERRAN_SCV) <
                    GetWorkerSaturationTargetCount(MacroStateDescriptorValue))
                {
                    EmergencyWeightValue += 250;
                }
                break;
            case ECommandTaskType::Supply:
                if (ExecutionPressureDescriptorValue.SupplyBlockState == EExecutionConditionState::Active ||
                    EconomyStateDescriptorValue.ProjectedAvailableSupplyByHorizon[ShortForecastHorizonIndexValue] <= 4U)
                {
                    EmergencyWeightValue += 300;
                }
                break;
            case ECommandTaskType::Expansion:
                if (MacroStateDescriptorValue.ActiveBaseCount < MacroStateDescriptorValue.DesiredBaseCount)
                {
                    EmergencyWeightValue += 275;
                }
                break;
            case ECommandTaskType::ProductionStructure:
                EmergencyWeightValue += HasProjectedProductionBottleneck(GameStateDescriptorValue) &&
                                                !HasIdleCombatProductionPressureValue
                                            ? 350
                                            : 125;
                break;
            case ECommandTaskType::AddOn:
                EmergencyWeightValue += 275;
                break;
            case ECommandTaskType::UnitProduction:
                EmergencyWeightValue += 350;
                break;
            case ECommandTaskType::TechStructure:
            case ECommandTaskType::UpgradeResearch:
                EmergencyWeightValue += 225;
                break;
            default:
                break;
        }
    }

    if (EconomyStateDescriptorValue.ProjectedAvailableMineralsByHorizon[ShortForecastHorizonIndexValue] >= 500U)
    {
        switch (CommandTaskTypeValue)
        {
            case ECommandTaskType::ProductionStructure:
                EmergencyWeightValue += HasProjectedProductionBottleneck(GameStateDescriptorValue) ? 350 : 250;
                break;
            case ECommandTaskType::UnitProduction:
                EmergencyWeightValue += ProductionStateDescriptorValue.CurrentBarracksCapacity > 0U ||
                                                ProductionStateDescriptorValue.CurrentFactoryCapacity > 0U ||
                                                ProductionStateDescriptorValue.CurrentStarportCapacity > 0U
                                            ? 250
                                            : 100;
                break;
            default:
                break;
        }
    }

    return EmergencyWeightValue;
}

ECommandPriorityTier FTerranCommandTaskPriorityService::DeterminePriorityTier(
    const FGameStateDescriptor& GameStateDescriptorValue,
    const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue, const size_t OrderIndexValue,
    const int EffectivePriorityValue) const
{
    const bool IsDefenseEmergencyValue = IsDefenseEmergency(GameStateDescriptorValue);
    const bool IsSupplyEmergencyValue = IsProjectedSupplyEmergency(GameStateDescriptorValue);
    const ECommandTaskType CommandTaskTypeValue = CommandAuthoritySchedulingStateValue.TaskTypes[OrderIndexValue];

    if (CommandTaskTypeValue == ECommandTaskType::Recovery ||
        (IsSupplyEmergencyValue && CommandTaskTypeValue == ECommandTaskType::Supply) ||
        (IsDefenseEmergencyValue &&
         (CommandTaskTypeValue == ECommandTaskType::ArmyMission ||
          CommandTaskTypeValue == ECommandTaskType::StaticDefense)))
    {
        return ECommandPriorityTier::Critical;
    }

    if (CommandTaskTypeValue == ECommandTaskType::Expansion ||
        CommandTaskTypeValue == ECommandTaskType::WorkerProduction ||
        CommandTaskTypeValue == ECommandTaskType::ProductionStructure ||
        CommandTaskTypeValue == ECommandTaskType::ArmyMission)
    {
        return ECommandPriorityTier::High;
    }

    if (CommandTaskTypeValue == ECommandTaskType::UpgradeResearch && EffectivePriorityValue < 900)
    {
        return ECommandPriorityTier::Low;
    }

    if (EffectivePriorityValue <= 400)
    {
        return ECommandPriorityTier::Low;
    }

    return ECommandPriorityTier::Normal;
}

}  // namespace sc2
