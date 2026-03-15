#include "test_scheduler_hot_path_profiles.h"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "common/descriptors/EMacroPhase.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/goals/FGoalDescriptor.h"
#include "common/planning/FCommandAuthoritySchedulingState.h"
#include "common/planning/FCommandOrderRecord.h"
#include "common/planning/FCommandTaskDescriptor.h"
#include "common/planning/FDefaultStrategicDirector.h"
#include "common/planning/FTerranCommandTaskPriorityService.h"

namespace sc2
{
namespace
{

using FSteadyClock = std::chrono::steady_clock;
using FSteadyTimePoint = std::chrono::time_point<FSteadyClock>;

bool Check(const bool ConditionValue, bool& SuccessValue, const std::string& MessageValue)
{
    if (!ConditionValue)
    {
        SuccessValue = false;
        std::cerr << "    " << MessageValue << std::endl;
    }

    return ConditionValue;
}

uint64_t GetElapsedMicroseconds(const FSteadyTimePoint& StartTimeValue, const FSteadyTimePoint& EndTimeValue)
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(EndTimeValue - StartTimeValue).count());
}

template <typename TValue>
size_t GetApproximateVectorRetainedBytes(const std::vector<TValue>& Values)
{
    return Values.capacity() * sizeof(TValue);
}

size_t GetApproximateTaskDescriptorVectorRetainedBytes(const std::vector<FCommandTaskDescriptor>& TaskDescriptorsValue)
{
    size_t ApproximateByteCountValue =
        GetApproximateVectorRetainedBytes(TaskDescriptorsValue);

    for (const FCommandTaskDescriptor& TaskDescriptorValue : TaskDescriptorsValue)
    {
        ApproximateByteCountValue += TaskDescriptorValue.TriggerReferenceClockTime.capacity() * sizeof(char);
        ApproximateByteCountValue +=
            TaskDescriptorValue.TriggerRequiredCompletedTaskIds.capacity() * sizeof(uint32_t);
    }

    return ApproximateByteCountValue;
}

size_t GetApproximateGoalSetRetainedBytes(const FAgentGoalSetDescriptor& AgentGoalSetDescriptorValue)
{
    return GetApproximateVectorRetainedBytes(AgentGoalSetDescriptorValue.ImmediateGoals) +
           GetApproximateVectorRetainedBytes(AgentGoalSetDescriptorValue.NearTermGoals) +
           GetApproximateVectorRetainedBytes(AgentGoalSetDescriptorValue.StrategicGoals);
}

size_t GetApproximateReadyIntentQueueRetainedBytes(
    const std::array<std::array<std::vector<size_t>, IntentDomainCountValue>, CommandPriorityTierCountValue>&
        QueueGroupsValue)
{
    size_t ApproximateByteCountValue = 0U;

    for (size_t PriorityTierIndexValue = 0U; PriorityTierIndexValue < CommandPriorityTierCountValue;
         ++PriorityTierIndexValue)
    {
        for (size_t IntentDomainIndexValue = 0U; IntentDomainIndexValue < IntentDomainCountValue;
             ++IntentDomainIndexValue)
        {
            ApproximateByteCountValue += GetApproximateVectorRetainedBytes(
                QueueGroupsValue[PriorityTierIndexValue][IntentDomainIndexValue]);
        }
    }

    return ApproximateByteCountValue;
}

size_t GetApproximateTierQueueRetainedBytes(
    const std::array<std::vector<size_t>, CommandPriorityTierCountValue>& QueueGroupsValue)
{
    size_t ApproximateByteCountValue = 0U;

    for (size_t PriorityTierIndexValue = 0U; PriorityTierIndexValue < CommandPriorityTierCountValue;
         ++PriorityTierIndexValue)
    {
        ApproximateByteCountValue +=
            GetApproximateVectorRetainedBytes(QueueGroupsValue[PriorityTierIndexValue]);
    }

    return ApproximateByteCountValue;
}

size_t GetApproximateSchedulingStateRetainedBytes(const FCommandAuthoritySchedulingState& SchedulingStateValue)
{
    size_t ApproximateByteCountValue = sizeof(FCommandAuthoritySchedulingState);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.OrderIds);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.ParentOrderIds);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.SourceGoalIds);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.SourceLayers);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.LifecycleStates);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.TaskPackageKinds);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.TaskNeedKinds);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.TaskTypes);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.BasePriorityValues);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.EffectivePriorityValues);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.PriorityTiers);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.IntentDomains);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.CreationSteps);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.DeadlineSteps);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.OwningArmyIndices);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.OwningSquadIndices);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.ActorTags);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.AbilityIds);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.TargetKinds);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.TargetPoints);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.TargetUnitTags);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.QueuedValues);
    ApproximateByteCountValue +=
        GetApproximateVectorRetainedBytes(SchedulingStateValue.RequiresPlacementValidationValues);
    ApproximateByteCountValue +=
        GetApproximateVectorRetainedBytes(SchedulingStateValue.RequiresPathingValidationValues);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.PlanStepIds);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.TargetCounts);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.RequestedQueueCounts);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.ProducerUnitTypeIds);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.ResultUnitTypeIds);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.UpgradeIds);
    ApproximateByteCountValue +=
        GetApproximateVectorRetainedBytes(SchedulingStateValue.PreferredPlacementSlotTypes);
    ApproximateByteCountValue +=
        GetApproximateVectorRetainedBytes(SchedulingStateValue.PreferredPlacementSlotIdTypes);
    ApproximateByteCountValue +=
        GetApproximateVectorRetainedBytes(SchedulingStateValue.PreferredPlacementSlotIdOrdinals);
    ApproximateByteCountValue +=
        GetApproximateVectorRetainedBytes(SchedulingStateValue.ReservedPlacementSlotTypes);
    ApproximateByteCountValue +=
        GetApproximateVectorRetainedBytes(SchedulingStateValue.ReservedPlacementSlotOrdinals);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.LastDeferralReasons);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.LastDeferralSteps);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.LastDeferralGameLoops);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.DispatchSteps);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.DispatchGameLoops);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.ObservedCountsAtDispatch);
    ApproximateByteCountValue +=
        GetApproximateVectorRetainedBytes(SchedulingStateValue.ObservedInConstructionCountsAtDispatch);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.DispatchAttemptCounts);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.StrategicOrderIndices);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.PlanningProcessIndices);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.ArmyOrderIndices);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.SquadOrderIndices);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.ReadyIntentIndices);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.DispatchedOrderIndices);
    ApproximateByteCountValue += GetApproximateVectorRetainedBytes(SchedulingStateValue.CompletedOrderIndices);
    ApproximateByteCountValue += GetApproximateTierQueueRetainedBytes(SchedulingStateValue.StrategicQueues);
    ApproximateByteCountValue += GetApproximateTierQueueRetainedBytes(SchedulingStateValue.PlanningQueues);
    ApproximateByteCountValue += GetApproximateTierQueueRetainedBytes(SchedulingStateValue.ArmyQueues);
    ApproximateByteCountValue += GetApproximateTierQueueRetainedBytes(SchedulingStateValue.SquadQueues);
    ApproximateByteCountValue +=
        GetApproximateReadyIntentQueueRetainedBytes(SchedulingStateValue.ReadyIntentQueues);

    ApproximateByteCountValue +=
        SchedulingStateValue.OrderIdToIndex.bucket_count() *
        (sizeof(void*) + sizeof(std::pair<const uint32_t, size_t>));

    return ApproximateByteCountValue;
}

FCommandOrderRecord CreateProfileOrder(const size_t OrderIndexValue)
{
    const uint32_t ParentOrderIdValue = OrderIndexValue > 0U ? static_cast<uint32_t>(OrderIndexValue) : 0U;
    const uint64_t CreationStepValue = static_cast<uint64_t>(OrderIndexValue + 1U);

    switch (OrderIndexValue % 8U)
    {
        case 0U:
        {
            FCommandOrderRecord CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
                ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::TRAIN_SCV, 250,
                EIntentDomain::UnitProduction, CreationStepValue);
            CommandOrderRecordValue.TaskType = ECommandTaskType::WorkerProduction;
            CommandOrderRecordValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_COMMANDCENTER;
            CommandOrderRecordValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
            CommandOrderRecordValue.TargetCount = 32U;
            return CommandOrderRecordValue;
        }
        case 1U:
        {
            FCommandOrderRecord CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
                ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::BUILD_SUPPLYDEPOT, 240,
                EIntentDomain::StructureBuild, CreationStepValue);
            CommandOrderRecordValue.TaskType = ECommandTaskType::Supply;
            CommandOrderRecordValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
            CommandOrderRecordValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_SUPPLYDEPOT;
            CommandOrderRecordValue.TargetCount = 8U;
            return CommandOrderRecordValue;
        }
        case 2U:
        {
            FCommandOrderRecord CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
                ECommandAuthorityLayer::EconomyAndProduction, NullTag, ABILITY_ID::BUILD_BARRACKS, 210,
                EIntentDomain::StructureBuild, CreationStepValue, 0U, ParentOrderIdValue);
            CommandOrderRecordValue.TaskType = ECommandTaskType::ProductionStructure;
            CommandOrderRecordValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
            CommandOrderRecordValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKS;
            CommandOrderRecordValue.TargetCount = 3U;
            return CommandOrderRecordValue;
        }
        case 3U:
        {
            FCommandOrderRecord CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
                ECommandAuthorityLayer::Army, NullTag, ABILITY_ID::INVALID, 180,
                EIntentDomain::ArmyCombat, CreationStepValue, 0U, ParentOrderIdValue, 0);
            CommandOrderRecordValue.TaskType = ECommandTaskType::ArmyMission;
            CommandOrderRecordValue.OwningArmyIndex = 0;
            return CommandOrderRecordValue;
        }
        case 4U:
        {
            FCommandOrderRecord CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
                ECommandAuthorityLayer::Squad, NullTag, ABILITY_ID::INVALID, 175,
                EIntentDomain::ArmyCombat, CreationStepValue, 0U, ParentOrderIdValue, 0, 0);
            CommandOrderRecordValue.TaskType = ECommandTaskType::ArmyMission;
            CommandOrderRecordValue.OwningArmyIndex = 0;
            CommandOrderRecordValue.OwningSquadIndex = 0;
            return CommandOrderRecordValue;
        }
        case 5U:
        {
            FCommandOrderRecord CommandOrderRecordValue = FCommandOrderRecord::CreatePointTarget(
                ECommandAuthorityLayer::UnitExecution, static_cast<Tag>(4000U + OrderIndexValue),
                ABILITY_ID::ATTACK_ATTACK, Point2D(80.0f, 80.0f), 170,
                EIntentDomain::ArmyCombat, CreationStepValue, 0U, ParentOrderIdValue, 0, 0, true, false, false);
            CommandOrderRecordValue.TaskType = ECommandTaskType::ArmyMission;
            CommandOrderRecordValue.LifecycleState = EOrderLifecycleState::Ready;
            CommandOrderRecordValue.EffectivePriorityValue = 1700;
            CommandOrderRecordValue.PriorityTier = ECommandPriorityTier::High;
            return CommandOrderRecordValue;
        }
        case 6U:
        {
            FCommandOrderRecord CommandOrderRecordValue = FCommandOrderRecord::CreatePointTarget(
                ECommandAuthorityLayer::UnitExecution, static_cast<Tag>(5000U + OrderIndexValue),
                ABILITY_ID::MOVE_MOVE, Point2D(40.0f, 40.0f), 130,
                EIntentDomain::ArmyCombat, CreationStepValue, 0U, ParentOrderIdValue, 0, 0, true, false, false);
            CommandOrderRecordValue.TaskType = ECommandTaskType::ArmyMission;
            CommandOrderRecordValue.LifecycleState = EOrderLifecycleState::Dispatched;
            CommandOrderRecordValue.DispatchAttemptCount = 1U;
            CommandOrderRecordValue.DispatchGameLoop = CreationStepValue;
            CommandOrderRecordValue.EffectivePriorityValue = 1500;
            CommandOrderRecordValue.PriorityTier = ECommandPriorityTier::High;
            return CommandOrderRecordValue;
        }
        case 7U:
        default:
        {
            FCommandOrderRecord CommandOrderRecordValue = FCommandOrderRecord::CreatePointTarget(
                ECommandAuthorityLayer::UnitExecution, static_cast<Tag>(6000U + OrderIndexValue),
                ABILITY_ID::MOVE_MOVE, Point2D(20.0f, 20.0f), 100,
                EIntentDomain::ArmyCombat, CreationStepValue, 0U, ParentOrderIdValue, 0, 0, true, false, false);
            CommandOrderRecordValue.TaskType = ECommandTaskType::ArmyMission;
            CommandOrderRecordValue.LifecycleState = EOrderLifecycleState::Completed;
            CommandOrderRecordValue.EffectivePriorityValue = 1200;
            CommandOrderRecordValue.PriorityTier = ECommandPriorityTier::Normal;
            return CommandOrderRecordValue;
        }
    }
}

void PopulateTaskDescriptors(std::vector<FCommandTaskDescriptor>& TaskDescriptorsValue, const size_t TaskCountValue)
{
    TaskDescriptorsValue.clear();
    TaskDescriptorsValue.reserve(TaskCountValue);

    for (size_t TaskIndexValue = 0U; TaskIndexValue < TaskCountValue; ++TaskIndexValue)
    {
        FCommandTaskDescriptor TaskDescriptorValue;
        TaskDescriptorValue.TaskId = static_cast<uint32_t>(TaskIndexValue + 1U);
        TaskDescriptorValue.PackageKind = ECommandTaskPackageKind::Opening;
        TaskDescriptorValue.NeedKind = ECommandTaskNeedKind::Unit;
        TaskDescriptorValue.ActionKind = ECommandTaskActionKind::TrainUnit;
        TaskDescriptorValue.CompletionKind = ECommandTaskCompletionKind::CountAtLeast;
        TaskDescriptorValue.TaskType = ECommandTaskType::UnitProduction;
        TaskDescriptorValue.BasePriorityValue = 150;
        TaskDescriptorValue.SourceGoalId = 300U;
        TaskDescriptorValue.TriggerMinGameLoop = static_cast<uint64_t>(TaskIndexValue * 32U);
        TaskDescriptorValue.TriggerReferenceClockTime = "08:17";
        TaskDescriptorValue.TriggerRequiredCompletedTaskIds.push_back(static_cast<uint32_t>(TaskIndexValue));
        TaskDescriptorValue.TriggerRequiredCompletedTaskIds.push_back(static_cast<uint32_t>(TaskIndexValue + 1U));
        TaskDescriptorValue.ActionAbilityId = ABILITY_ID::TRAIN_MARINE;
        TaskDescriptorValue.ActionProducerUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKS;
        TaskDescriptorValue.ActionResultUnitTypeId = UNIT_TYPEID::TERRAN_MARINE;
        TaskDescriptorValue.ActionTargetCount = 2U;
        TaskDescriptorValue.ActionRequestedQueueCount = 2U;
        TaskDescriptorValue.CompletionObservedCountAtLeast = 2U;
        TaskDescriptorsValue.push_back(TaskDescriptorValue);
    }
}

FGameStateDescriptor CreateGoalProfileGameStateDescriptor(const EMacroPhase MacroPhaseValue)
{
    FGameStateDescriptor GameStateDescriptorValue;
    GameStateDescriptorValue.CurrentStep = 1U;
    GameStateDescriptorValue.CurrentGameLoop = 1U;
    GameStateDescriptorValue.MacroState.ActiveMacroPhase = MacroPhaseValue;
    GameStateDescriptorValue.MacroState.ActiveBaseCount = MacroPhaseValue == EMacroPhase::Recovery ? 1U : 2U;
    GameStateDescriptorValue.MacroState.WorkerCount = MacroPhaseValue == EMacroPhase::Recovery ? 8U : 34U;
    GameStateDescriptorValue.MacroState.ArmySupply = MacroPhaseValue == EMacroPhase::Recovery ? 2U : 28U;
    GameStateDescriptorValue.MacroState.ArmyUnitCount = MacroPhaseValue == EMacroPhase::Recovery ? 2U : 18U;
    GameStateDescriptorValue.MacroState.BarracksCount = MacroPhaseValue == EMacroPhase::Recovery ? 1U : 2U;
    GameStateDescriptorValue.MacroState.FactoryCount = MacroPhaseValue == EMacroPhase::Recovery ? 0U : 1U;
    GameStateDescriptorValue.MacroState.StarportCount = MacroPhaseValue == EMacroPhase::Recovery ? 0U : 1U;
    GameStateDescriptorValue.BuildPlanning.AvailableMinerals =
        MacroPhaseValue == EMacroPhase::Recovery ? 125U : 550U;
    GameStateDescriptorValue.BuildPlanning.AvailableVespene =
        MacroPhaseValue == EMacroPhase::Recovery ? 0U : 200U;
    GameStateDescriptorValue.BuildPlanning.AvailableSupply =
        MacroPhaseValue == EMacroPhase::Recovery ? 3U : 12U;
    GameStateDescriptorValue.BuildPlanning.ObservedTownHallCount =
        MacroPhaseValue == EMacroPhase::Recovery ? 1U : 2U;
    GameStateDescriptorValue.BuildPlanning.ObservedOrbitalCommandCount =
        MacroPhaseValue == EMacroPhase::Recovery ? 0U : 1U;
    GameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
        UNIT_TYPEID::TERRAN_SUPPLYDEPOT)] = MacroPhaseValue == EMacroPhase::Recovery ? 1U : 4U;
    GameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
        UNIT_TYPEID::TERRAN_BARRACKS)] = MacroPhaseValue == EMacroPhase::Recovery ? 1U : 2U;
    GameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
        UNIT_TYPEID::TERRAN_FACTORY)] = MacroPhaseValue == EMacroPhase::Recovery ? 0U : 1U;
    GameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
        UNIT_TYPEID::TERRAN_STARPORT)] = MacroPhaseValue == EMacroPhase::Recovery ? 0U : 1U;
    GameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
        UNIT_TYPEID::TERRAN_REFINERY)] = MacroPhaseValue == EMacroPhase::Recovery ? 0U : 2U;
    GameStateDescriptorValue.BuildPlanning.ObservedUnitCounts[GetTerranUnitTypeIndex(
        UNIT_TYPEID::TERRAN_MARINE)] = MacroPhaseValue == EMacroPhase::Recovery ? 2U : 16U;
    GameStateDescriptorValue.BuildPlanning.ObservedUnitCounts[GetTerranUnitTypeIndex(
        UNIT_TYPEID::TERRAN_MARAUDER)] = MacroPhaseValue == EMacroPhase::Recovery ? 0U : 2U;
    GameStateDescriptorValue.ArmyState.EnsurePrimaryArmyExists();
    return GameStateDescriptorValue;
}

void PrintProfileHeader(const char* HeaderTextPtrValue)
{
    std::cout << "[HotPathProfile] " << HeaderTextPtrValue << std::endl;
}

}  // namespace

bool TestSchedulerHotPathProfiles(int ArgC, char** ArgV)
{
    (void)ArgC;
    (void)ArgV;

    bool SuccessValue = true;

    PrintProfileHeader("TypeSizes");
    std::cout << "  FGoalDescriptor=" << sizeof(FGoalDescriptor)
              << " bytes | FCommandTaskDescriptor=" << sizeof(FCommandTaskDescriptor)
              << " bytes | FCommandOrderRecord=" << sizeof(FCommandOrderRecord)
              << " bytes | FCommandAuthoritySchedulingState=" << sizeof(FCommandAuthoritySchedulingState)
              << " bytes" << std::endl;

    {
        constexpr size_t ProfileTaskCountValue = 2048U;
        std::vector<FCommandTaskDescriptor> TaskDescriptorsValue;
        const FSteadyTimePoint BuildStartTimeValue = FSteadyClock::now();
        PopulateTaskDescriptors(TaskDescriptorsValue, ProfileTaskCountValue);
        const FSteadyTimePoint BuildEndTimeValue = FSteadyClock::now();
        const size_t ApproximateTaskBytesValue =
            GetApproximateTaskDescriptorVectorRetainedBytes(TaskDescriptorsValue);

        Check(TaskDescriptorsValue.size() == ProfileTaskCountValue, SuccessValue,
              "Task descriptor profile should build the requested descriptor count.");
        PrintProfileHeader("TaskDescriptors");
        std::cout << "  Count=" << ProfileTaskCountValue
                  << " | BuildUs=" << GetElapsedMicroseconds(BuildStartTimeValue, BuildEndTimeValue)
                  << " | ApproxBytes=" << ApproximateTaskBytesValue << std::endl;
    }

    {
        constexpr uint32_t GoalIterationsValue = 10000U;
        const std::array<EMacroPhase, 2U> MacroPhasesValue =
        {
            EMacroPhase::Recovery,
            EMacroPhase::MidGame,
        };

        const FDefaultStrategicDirector StrategicDirectorValue;
        for (const EMacroPhase MacroPhaseValue : MacroPhasesValue)
        {
            FGameStateDescriptor GameStateDescriptorValue = CreateGoalProfileGameStateDescriptor(MacroPhaseValue);
            const FSteadyTimePoint UpdateStartTimeValue = FSteadyClock::now();
            for (uint32_t IterationIndexValue = 0U; IterationIndexValue < GoalIterationsValue; ++IterationIndexValue)
            {
                StrategicDirectorValue.UpdateGameStateDescriptor(GameStateDescriptorValue);
            }
            const FSteadyTimePoint UpdateEndTimeValue = FSteadyClock::now();
            const uint64_t TotalUpdateMicrosecondsValue =
                GetElapsedMicroseconds(UpdateStartTimeValue, UpdateEndTimeValue);
            const uint64_t AverageUpdateMicrosecondsValue =
                GoalIterationsValue > 0U ? (TotalUpdateMicrosecondsValue / GoalIterationsValue) : 0U;

            Check(!GameStateDescriptorValue.GoalSet.ImmediateGoals.empty(), SuccessValue,
                  "Goal profiling should produce immediate goals.");
            Check(!GameStateDescriptorValue.GoalSet.StrategicGoals.empty(), SuccessValue,
                  "Goal profiling should produce strategic goals.");

            PrintProfileHeader("Goals");
            std::cout << "  Phase=" << ToString(MacroPhaseValue)
                      << " | Iterations=" << GoalIterationsValue
                      << " | AvgUpdateUs=" << AverageUpdateMicrosecondsValue
                      << " | GoalBytes=" << GetApproximateGoalSetRetainedBytes(GameStateDescriptorValue.GoalSet)
                      << " | Immediate=" << GameStateDescriptorValue.GoalSet.ImmediateGoals.size()
                      << " | Near=" << GameStateDescriptorValue.GoalSet.NearTermGoals.size()
                      << " | Strategic=" << GameStateDescriptorValue.GoalSet.StrategicGoals.size()
                      << std::endl;
        }
    }

    {
        const std::array<size_t, 3U> ProfileOrderCountsValue =
        {
            1024U,
            4096U,
            8192U,
        };
        const FTerranCommandTaskPriorityService CommandTaskPriorityServiceValue;

        for (const size_t OrderCountValue : ProfileOrderCountsValue)
        {
            FGameStateDescriptor GameStateDescriptorValue = CreateGoalProfileGameStateDescriptor(EMacroPhase::MidGame);
            FCommandAuthoritySchedulingState& SchedulingStateValue =
                GameStateDescriptorValue.CommandAuthoritySchedulingState;
            SchedulingStateValue.Reset();
            SchedulingStateValue.Reserve(OrderCountValue);

            const FSteadyTimePoint EnqueueStartTimeValue = FSteadyClock::now();
            SchedulingStateValue.BeginMutationBatch();
            for (size_t OrderIndexValue = 0U; OrderIndexValue < OrderCountValue; ++OrderIndexValue)
            {
                SchedulingStateValue.EnqueueOrder(CreateProfileOrder(OrderIndexValue));
            }
            const FSteadyTimePoint EnqueueEndTimeValue = FSteadyClock::now();

            const FSteadyTimePoint RebuildStartTimeValue = FSteadyClock::now();
            SchedulingStateValue.EndMutationBatch();
            const FSteadyTimePoint RebuildEndTimeValue = FSteadyClock::now();

            const size_t ApproximateBytesBeforePriorityValue =
                GetApproximateSchedulingStateRetainedBytes(SchedulingStateValue);

            const FSteadyTimePoint PriorityStartTimeValue = FSteadyClock::now();
            CommandTaskPriorityServiceValue.UpdateTaskPriorities(GameStateDescriptorValue);
            const FSteadyTimePoint PriorityEndTimeValue = FSteadyClock::now();

            const FSteadyTimePoint CompactionStartTimeValue = FSteadyClock::now();
            const bool CompactedAnyOrderValue = SchedulingStateValue.CompactTerminalOrders();
            const FSteadyTimePoint CompactionEndTimeValue = FSteadyClock::now();

            const size_t ApproximateBytesAfterCompactionValue =
                GetApproximateSchedulingStateRetainedBytes(SchedulingStateValue);

            Check(SchedulingStateValue.GetOrderCount() <= OrderCountValue, SuccessValue,
                  "Scheduler profiling should not grow beyond the requested order count after compaction.");
            Check(CompactedAnyOrderValue, SuccessValue,
                  "Scheduler profiling should compact completed unit-execution orders.");
            Check(ApproximateBytesAfterCompactionValue <= ApproximateBytesBeforePriorityValue, SuccessValue,
                  "Scheduler profiling compaction should not increase retained scheduler bytes.");

            PrintProfileHeader("Scheduler");
            std::cout << "  Orders=" << OrderCountValue
                      << " | EnqueueUs=" << GetElapsedMicroseconds(EnqueueStartTimeValue, EnqueueEndTimeValue)
                      << " | RebuildUs=" << GetElapsedMicroseconds(RebuildStartTimeValue, RebuildEndTimeValue)
                      << " | PriorityUs=" << GetElapsedMicroseconds(PriorityStartTimeValue, PriorityEndTimeValue)
                      << " | CompactUs=" << GetElapsedMicroseconds(CompactionStartTimeValue, CompactionEndTimeValue)
                      << " | BytesBefore=" << ApproximateBytesBeforePriorityValue
                      << " | BytesAfter=" << ApproximateBytesAfterCompactionValue
                      << " | OrdersAfter=" << SchedulingStateValue.GetOrderCount()
                      << std::endl;
        }
    }

    return SuccessValue;
}

}  // namespace sc2
