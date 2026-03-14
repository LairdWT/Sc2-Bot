#include "common/planning/FTerranCommandTaskPriorityService.h"

#include <algorithm>

#include "common/armies/EArmyMissionType.h"
#include "common/descriptors/FMacroStateDescriptor.h"
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
    const FMacroStateDescriptor& MacroStateDescriptorValue = GameStateDescriptorValue.MacroState;
    const FBuildPlanningState& BuildPlanningStateValue = GameStateDescriptorValue.BuildPlanning;
    return BuildPlanningStateValue.AvailableMinerals >= 300U &&
           MacroStateDescriptorValue.WorkerCount >= (MacroStateDescriptorValue.ActiveBaseCount * 14U);
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

}  // namespace

void FTerranCommandTaskPriorityService::UpdateTaskPriorities(FGameStateDescriptor& GameStateDescriptorValue) const
{
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState;

    const size_t OrderCountValue = CommandAuthoritySchedulingStateValue.OrderIds.size();
    for (size_t OrderIndexValue = 0U; OrderIndexValue < OrderCountValue; ++OrderIndexValue)
    {
        FCommandOrderRecord CommandOrderRecordValue =
            CommandAuthoritySchedulingStateValue.GetOrderRecord(OrderIndexValue);
        const int FocusWeightValue =
            GetFocusWeight(GameStateDescriptorValue.MacroState.PrimaryProductionFocus,
                           CommandOrderRecordValue.TaskType);
        const int EffectivePriorityValue =
            CommandOrderRecordValue.BasePriorityValue + GetTaskTypeWeight(CommandOrderRecordValue.TaskType) +
            FocusWeightValue + GetEmergencyWeight(GameStateDescriptorValue, CommandOrderRecordValue);
        CommandAuthoritySchedulingStateValue.EffectivePriorityValues[OrderIndexValue] = EffectivePriorityValue;
        CommandAuthoritySchedulingStateValue.PriorityTiers[OrderIndexValue] =
            DeterminePriorityTier(GameStateDescriptorValue, CommandOrderRecordValue, EffectivePriorityValue);
    }

    CommandAuthoritySchedulingStateValue.RebuildDerivedQueues();
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
    const FGameStateDescriptor& GameStateDescriptorValue, const FCommandOrderRecord& CommandOrderRecordValue) const
{
    int EmergencyWeightValue = 0;
    const FMacroStateDescriptor& MacroStateDescriptorValue = GameStateDescriptorValue.MacroState;
    const FBuildPlanningState& BuildPlanningStateValue = GameStateDescriptorValue.BuildPlanning;

    if (BuildPlanningStateValue.AvailableSupply <= 1U)
    {
        switch (CommandOrderRecordValue.TaskType)
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
        CommandOrderRecordValue.TaskType == ECommandTaskType::Expansion)
    {
        EmergencyWeightValue += 350;
    }

    if (MacroStateDescriptorValue.WorkerCount < GetWorkerSaturationTargetCount(MacroStateDescriptorValue) &&
        CommandOrderRecordValue.TaskType == ECommandTaskType::WorkerProduction)
    {
        EmergencyWeightValue += 300;
    }

    if (IsDefenseEmergency(GameStateDescriptorValue))
    {
        switch (CommandOrderRecordValue.TaskType)
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

    if (BuildPlanningStateValue.AvailableMinerals >= 500U)
    {
        switch (CommandOrderRecordValue.TaskType)
        {
            case ECommandTaskType::ProductionStructure:
            case ECommandTaskType::UnitProduction:
                EmergencyWeightValue += 250;
                break;
            default:
                break;
        }
    }

    return EmergencyWeightValue;
}

ECommandPriorityTier FTerranCommandTaskPriorityService::DeterminePriorityTier(
    const FGameStateDescriptor& GameStateDescriptorValue, const FCommandOrderRecord& CommandOrderRecordValue,
    const int EffectivePriorityValue) const
{
    const bool IsDefenseEmergencyValue = IsDefenseEmergency(GameStateDescriptorValue);
    const bool IsSupplyEmergencyValue = GameStateDescriptorValue.BuildPlanning.AvailableSupply <= 1U;

    if (CommandOrderRecordValue.TaskType == ECommandTaskType::Recovery ||
        (IsSupplyEmergencyValue && CommandOrderRecordValue.TaskType == ECommandTaskType::Supply) ||
        (IsDefenseEmergencyValue &&
         (CommandOrderRecordValue.TaskType == ECommandTaskType::ArmyMission ||
          CommandOrderRecordValue.TaskType == ECommandTaskType::StaticDefense)))
    {
        return ECommandPriorityTier::Critical;
    }

    if (CommandOrderRecordValue.TaskType == ECommandTaskType::Expansion ||
        CommandOrderRecordValue.TaskType == ECommandTaskType::WorkerProduction ||
        CommandOrderRecordValue.TaskType == ECommandTaskType::ProductionStructure ||
        CommandOrderRecordValue.TaskType == ECommandTaskType::ArmyMission)
    {
        return ECommandPriorityTier::High;
    }

    if (CommandOrderRecordValue.TaskType == ECommandTaskType::UpgradeResearch &&
        EffectivePriorityValue < 900)
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
