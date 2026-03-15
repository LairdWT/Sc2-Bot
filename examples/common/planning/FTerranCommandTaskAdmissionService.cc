#include "common/planning/FTerranCommandTaskAdmissionService.h"

#include <array>
#include <vector>

#include "common/descriptors/FGameStateDescriptor.h"
#include "common/planning/FBlockedTaskRecord.h"
#include "common/planning/FCommandAuthoritySchedulingState.h"
#include "common/planning/FCommandOrderRecord.h"

namespace sc2
{
namespace
{

constexpr uint32_t MaxActiveArmyOrdersValue = 8U;
constexpr uint32_t MaxActiveSquadOrdersValue = 16U;
constexpr uint64_t NoProducerRetryDelayGameLoopsValue = 224U;
constexpr uint64_t InsufficientResourcesRetryDelayGameLoopsValue = 16U;
constexpr uint64_t PlacementRetryDelayGameLoopsValue = 32U;
constexpr uint64_t ProducerBusyRetryDelayGameLoopsValue = 24U;

FBuildPlacementSlotId CreateBuildPlacementSlotId(const EBuildPlacementSlotType SlotTypeValue, const uint8_t OrdinalValue)
{
    FBuildPlacementSlotId BuildPlacementSlotIdValue;
    BuildPlacementSlotIdValue.SlotType = SlotTypeValue;
    BuildPlacementSlotIdValue.Ordinal = OrdinalValue;
    return BuildPlacementSlotIdValue;
}

uint64_t CombineFingerprint(const uint64_t CurrentFingerprintValue, const uint64_t NextValue)
{
    constexpr uint64_t OffsetBasisValue = 1469598103934665603ULL;
    constexpr uint64_t PrimeValue = 1099511628211ULL;

    uint64_t CombinedFingerprintValue = CurrentFingerprintValue == 0U ? OffsetBasisValue : CurrentFingerprintValue;
    CombinedFingerprintValue ^= NextValue;
    CombinedFingerprintValue *= PrimeValue;
    return CombinedFingerprintValue;
}

uint64_t BuildGoalFingerprint(const FGameStateDescriptor& GameStateDescriptorValue)
{
    uint64_t GoalFingerprintValue = 0U;
    const std::array<const std::vector<FGoalDescriptor>*, 3U> GoalListsValue = {
        &GameStateDescriptorValue.GoalSet.ImmediateGoals,
        &GameStateDescriptorValue.GoalSet.NearTermGoals,
        &GameStateDescriptorValue.GoalSet.StrategicGoals};

    for (const std::vector<FGoalDescriptor>* GoalListValue : GoalListsValue)
    {
        if (GoalListValue == nullptr)
        {
            continue;
        }

        for (const FGoalDescriptor& GoalDescriptorValue : *GoalListValue)
        {
            GoalFingerprintValue = CombineFingerprint(GoalFingerprintValue, GoalDescriptorValue.GoalId);
            GoalFingerprintValue =
                CombineFingerprint(GoalFingerprintValue, static_cast<uint64_t>(GoalDescriptorValue.GoalStatus));
            GoalFingerprintValue = CombineFingerprint(GoalFingerprintValue, GoalDescriptorValue.TargetCount);
            GoalFingerprintValue = CombineFingerprint(GoalFingerprintValue,
                                                      static_cast<uint64_t>(GoalDescriptorValue.TargetUnitTypeId));
            GoalFingerprintValue =
                CombineFingerprint(GoalFingerprintValue, static_cast<uint64_t>(GoalDescriptorValue.TargetUpgradeId.ToType()));
        }
    }

    return GoalFingerprintValue;
}

uint64_t BuildPlacementFingerprint(const FGameStateDescriptor& GameStateDescriptorValue)
{
    uint64_t PlacementFingerprintValue = 0U;
    PlacementFingerprintValue =
        CombineFingerprint(PlacementFingerprintValue, GameStateDescriptorValue.BuildPlanning.ObservedTownHallCount);
    PlacementFingerprintValue = CombineFingerprint(
        PlacementFingerprintValue,
        static_cast<uint64_t>(GameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
            UNIT_TYPEID::TERRAN_SUPPLYDEPOT)]));
    PlacementFingerprintValue = CombineFingerprint(
        PlacementFingerprintValue,
        static_cast<uint64_t>(GameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
            UNIT_TYPEID::TERRAN_BARRACKS)]));
    PlacementFingerprintValue = CombineFingerprint(
        PlacementFingerprintValue,
        static_cast<uint64_t>(GameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
            UNIT_TYPEID::TERRAN_FACTORY)]));
    PlacementFingerprintValue = CombineFingerprint(
        PlacementFingerprintValue,
        static_cast<uint64_t>(GameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
            UNIT_TYPEID::TERRAN_STARPORT)]));
    PlacementFingerprintValue =
        CombineFingerprint(PlacementFingerprintValue, GameStateDescriptorValue.RampWallDescriptor.bIsValid ? 1U : 0U);
    PlacementFingerprintValue = CombineFingerprint(
        PlacementFingerprintValue, static_cast<uint64_t>(GameStateDescriptorValue.ObservedRampWallState.LeftDepotState));
    PlacementFingerprintValue = CombineFingerprint(
        PlacementFingerprintValue, static_cast<uint64_t>(GameStateDescriptorValue.ObservedRampWallState.BarracksState));
    PlacementFingerprintValue = CombineFingerprint(
        PlacementFingerprintValue, static_cast<uint64_t>(GameStateDescriptorValue.ObservedRampWallState.RightDepotState));
    return PlacementFingerprintValue;
}

uint64_t BuildArmyMissionFingerprint(const FGameStateDescriptor& GameStateDescriptorValue)
{
    if (GameStateDescriptorValue.ArmyState.ArmyMissions.empty())
    {
        return 0U;
    }

    const FArmyMissionDescriptor& ArmyMissionDescriptorValue = GameStateDescriptorValue.ArmyState.ArmyMissions.front();
    uint64_t ArmyMissionFingerprintValue = 0U;
    ArmyMissionFingerprintValue =
        CombineFingerprint(ArmyMissionFingerprintValue, static_cast<uint64_t>(ArmyMissionDescriptorValue.MissionType));
    ArmyMissionFingerprintValue =
        CombineFingerprint(ArmyMissionFingerprintValue, ArmyMissionDescriptorValue.SourceGoalId);
    ArmyMissionFingerprintValue = CombineFingerprint(
        ArmyMissionFingerprintValue, static_cast<uint64_t>(ArmyMissionDescriptorValue.ObjectivePoint.x * 100.0f));
    ArmyMissionFingerprintValue = CombineFingerprint(
        ArmyMissionFingerprintValue, static_cast<uint64_t>(ArmyMissionDescriptorValue.ObjectivePoint.y * 100.0f));
    ArmyMissionFingerprintValue =
        CombineFingerprint(ArmyMissionFingerprintValue, ArmyMissionDescriptorValue.SearchExpansionOrdinal);
    return ArmyMissionFingerprintValue;
}

EIntentDomain DetermineIntentDomainFromAbility(const FCommandOrderRecord& CommandOrderRecordValue)
{
    if (CommandOrderRecordValue.TaskType == ECommandTaskType::ArmyMission)
    {
        return EIntentDomain::ArmyCombat;
    }

    switch (CommandOrderRecordValue.AbilityId.ToType())
    {
        case ABILITY_ID::TRAIN_SCV:
        case ABILITY_ID::TRAIN_MARINE:
        case ABILITY_ID::TRAIN_MARAUDER:
        case ABILITY_ID::TRAIN_HELLION:
        case ABILITY_ID::TRAIN_CYCLONE:
        case ABILITY_ID::TRAIN_MEDIVAC:
        case ABILITY_ID::TRAIN_LIBERATOR:
        case ABILITY_ID::TRAIN_SIEGETANK:
        case ABILITY_ID::TRAIN_WIDOWMINE:
        case ABILITY_ID::TRAIN_VIKINGFIGHTER:
        case ABILITY_ID::RESEARCH_STIMPACK:
        case ABILITY_ID::RESEARCH_COMBATSHIELD:
        case ABILITY_ID::RESEARCH_CONCUSSIVESHELLS:
        case ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONSLEVEL1:
            return EIntentDomain::UnitProduction;
        case ABILITY_ID::INVALID:
            return EIntentDomain::Recovery;
        default:
            return EIntentDomain::StructureBuild;
    }
}

size_t GetHotOrderLayerCap(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                           const ECommandAuthorityLayer SourceLayerValue)
{
    switch (SourceLayerValue)
    {
        case ECommandAuthorityLayer::Agent:
        case ECommandAuthorityLayer::StrategicDirector:
            return CommandAuthoritySchedulingStateValue.MaxActiveStrategicOrders;
        case ECommandAuthorityLayer::EconomyAndProduction:
            return CommandAuthoritySchedulingStateValue.MaxActivePlanningOrders;
        case ECommandAuthorityLayer::Army:
            return MaxActiveArmyOrdersValue;
        case ECommandAuthorityLayer::Squad:
            return MaxActiveSquadOrdersValue;
        case ECommandAuthorityLayer::UnitExecution:
            return CommandAuthoritySchedulingStateValue.MaxActiveUnitExecutionOrders;
        default:
            return CommandAuthoritySchedulingStateValue.MaxActivePlanningOrders;
    }
}

bool IsStrategicBufferLayer(const ECommandAuthorityLayer SourceLayerValue)
{
    switch (SourceLayerValue)
    {
        case ECommandAuthorityLayer::Agent:
        case ECommandAuthorityLayer::StrategicDirector:
            return true;
        default:
            return false;
    }
}

bool DoesTaskSignatureMatch(const uint32_t LeftTaskIdValue, const uint32_t LeftSourceGoalIdValue,
                            const AbilityID LeftAbilityIdValue, const UNIT_TYPEID LeftResultUnitTypeIdValue,
                            const UpgradeID LeftUpgradeIdValue,
                            const FBuildPlacementSlotId& LeftPreferredPlacementSlotIdValue,
                            const uint32_t RightTaskIdValue, const uint32_t RightSourceGoalIdValue,
                            const AbilityID RightAbilityIdValue, const UNIT_TYPEID RightResultUnitTypeIdValue,
                            const UpgradeID RightUpgradeIdValue,
                            const FBuildPlacementSlotId& RightPreferredPlacementSlotIdValue)
{
    if (LeftTaskIdValue != 0U || RightTaskIdValue != 0U)
    {
        return LeftTaskIdValue == RightTaskIdValue;
    }

    return LeftSourceGoalIdValue == RightSourceGoalIdValue && LeftAbilityIdValue == RightAbilityIdValue &&
           LeftResultUnitTypeIdValue == RightResultUnitTypeIdValue && LeftUpgradeIdValue == RightUpgradeIdValue &&
           LeftPreferredPlacementSlotIdValue == RightPreferredPlacementSlotIdValue;
}

bool DoesHotOrderMatchOpeningTask(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                  const FCommandTaskDescriptor& CommandTaskDescriptorValue)
{
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (IsTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        const FBuildPlacementSlotId PreferredPlacementSlotIdValue = CreateBuildPlacementSlotId(
            CommandAuthoritySchedulingStateValue.PreferredPlacementSlotIdTypes[OrderIndexValue],
            CommandAuthoritySchedulingStateValue.PreferredPlacementSlotIdOrdinals[OrderIndexValue]);
        if (DoesTaskSignatureMatch(
                CommandAuthoritySchedulingStateValue.PlanStepIds[OrderIndexValue],
                CommandAuthoritySchedulingStateValue.SourceGoalIds[OrderIndexValue],
                CommandAuthoritySchedulingStateValue.AbilityIds[OrderIndexValue],
                CommandAuthoritySchedulingStateValue.ResultUnitTypeIds[OrderIndexValue],
                CommandAuthoritySchedulingStateValue.UpgradeIds[OrderIndexValue], PreferredPlacementSlotIdValue,
                CommandTaskDescriptorValue.TaskId, CommandTaskDescriptorValue.SourceGoalId,
                CommandTaskDescriptorValue.ActionAbilityId, CommandTaskDescriptorValue.ActionResultUnitTypeId,
                CommandTaskDescriptorValue.ActionUpgradeId, CommandTaskDescriptorValue.ActionPreferredPlacementSlotId))
        {
            return true;
        }
    }

    return false;
}

bool DoesHotOrderMatchOrderSignature(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                     const FCommandOrderRecord& CommandOrderRecordValue)
{
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (IsTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        const FBuildPlacementSlotId PreferredPlacementSlotIdValue = CreateBuildPlacementSlotId(
            CommandAuthoritySchedulingStateValue.PreferredPlacementSlotIdTypes[OrderIndexValue],
            CommandAuthoritySchedulingStateValue.PreferredPlacementSlotIdOrdinals[OrderIndexValue]);
        if (DoesTaskSignatureMatch(
                CommandAuthoritySchedulingStateValue.PlanStepIds[OrderIndexValue],
                CommandAuthoritySchedulingStateValue.SourceGoalIds[OrderIndexValue],
                CommandAuthoritySchedulingStateValue.AbilityIds[OrderIndexValue],
                CommandAuthoritySchedulingStateValue.ResultUnitTypeIds[OrderIndexValue],
                CommandAuthoritySchedulingStateValue.UpgradeIds[OrderIndexValue], PreferredPlacementSlotIdValue,
                CommandOrderRecordValue.PlanStepId, CommandOrderRecordValue.SourceGoalId,
                CommandOrderRecordValue.AbilityId, CommandOrderRecordValue.ResultUnitTypeId,
                CommandOrderRecordValue.UpgradeId, CommandOrderRecordValue.PreferredPlacementSlotId))
        {
            return true;
        }
    }

    return false;
}

bool HasEquivalentBlockedTask(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                              const FCommandOrderRecord& CommandOrderRecordValue)
{
    return CommandAuthoritySchedulingStateValue.BlockedStrategicTasks.HasEquivalentOrderSignature(CommandOrderRecordValue) ||
           CommandAuthoritySchedulingStateValue.BlockedPlanningTasks.HasEquivalentOrderSignature(CommandOrderRecordValue);
}

FBlockedTaskRecord CreateBlockedTaskRecord(const FCommandOrderRecord& CommandOrderRecordValue,
                                           const ECommandOrderDeferralReason DeferralReasonValue,
                                           const uint64_t CurrentGameLoopValue,
                                           const uint64_t LastSeenStimulusRevisionValue)
{
    FBlockedTaskRecord BlockedTaskRecordValue;
    BlockedTaskRecordValue.TaskId = CommandOrderRecordValue.PlanStepId;
    BlockedTaskRecordValue.SourceGoalId = CommandOrderRecordValue.SourceGoalId;
    BlockedTaskRecordValue.PackageKind = CommandOrderRecordValue.TaskPackageKind;
    BlockedTaskRecordValue.NeedKind = CommandOrderRecordValue.TaskNeedKind;
    BlockedTaskRecordValue.TaskType = CommandOrderRecordValue.TaskType;
    BlockedTaskRecordValue.RetentionPolicy = CommandOrderRecordValue.RetentionPolicy;
    BlockedTaskRecordValue.AbilityId = CommandOrderRecordValue.AbilityId;
    BlockedTaskRecordValue.ResultUnitTypeId = CommandOrderRecordValue.ResultUnitTypeId;
    BlockedTaskRecordValue.UpgradeId = CommandOrderRecordValue.UpgradeId;
    BlockedTaskRecordValue.PreferredPlacementSlotId = CommandOrderRecordValue.PreferredPlacementSlotId;
    BlockedTaskRecordValue.BlockingReason = DeferralReasonValue;
    BlockedTaskRecordValue.RequestedQueueCount = CommandOrderRecordValue.RequestedQueueCount;
    BlockedTaskRecordValue.RetryCount = CommandOrderRecordValue.ConsecutiveDeferralCount;
    BlockedTaskRecordValue.LastSeenStimulusRevision = LastSeenStimulusRevisionValue;

    switch (DeferralReasonValue)
    {
        case ECommandOrderDeferralReason::NoProducer:
            BlockedTaskRecordValue.WakeKind = EBlockedTaskWakeKind::ProducerAvailability;
            BlockedTaskRecordValue.NextEligibleGameLoop = CurrentGameLoopValue + NoProducerRetryDelayGameLoopsValue;
            break;
        case ECommandOrderDeferralReason::InsufficientResources:
            BlockedTaskRecordValue.WakeKind = EBlockedTaskWakeKind::Resources;
            BlockedTaskRecordValue.NextEligibleGameLoop =
                CurrentGameLoopValue + InsufficientResourcesRetryDelayGameLoopsValue;
            break;
        case ECommandOrderDeferralReason::NoValidPlacement:
        case ECommandOrderDeferralReason::ReservedSlotOccupied:
        case ECommandOrderDeferralReason::ReservedSlotInvalidated:
            BlockedTaskRecordValue.WakeKind = EBlockedTaskWakeKind::Placement;
            BlockedTaskRecordValue.NextEligibleGameLoop = CurrentGameLoopValue + PlacementRetryDelayGameLoopsValue;
            break;
        case ECommandOrderDeferralReason::ProducerBusy:
            BlockedTaskRecordValue.WakeKind = EBlockedTaskWakeKind::ProducerAvailability;
            BlockedTaskRecordValue.NextEligibleGameLoop = CurrentGameLoopValue + ProducerBusyRetryDelayGameLoopsValue;
            break;
        default:
            BlockedTaskRecordValue.WakeKind = EBlockedTaskWakeKind::CooldownOnly;
            BlockedTaskRecordValue.NextEligibleGameLoop = CurrentGameLoopValue + PlacementRetryDelayGameLoopsValue;
            break;
    }

    return BlockedTaskRecordValue;
}

bool ShouldParkDeferredOrder(const FCommandOrderRecord& CommandOrderRecordValue)
{
    switch (CommandOrderRecordValue.LastDeferralReason)
    {
        case ECommandOrderDeferralReason::NoProducer:
        case ECommandOrderDeferralReason::InsufficientResources:
        case ECommandOrderDeferralReason::NoValidPlacement:
        case ECommandOrderDeferralReason::ReservedSlotOccupied:
        case ECommandOrderDeferralReason::ReservedSlotInvalidated:
            return true;
        case ECommandOrderDeferralReason::ProducerBusy:
            return CommandOrderRecordValue.ConsecutiveDeferralCount >= 3U;
        default:
            return false;
    }
}

uint64_t GetStimulusRevisionForWakeKind(const FSchedulerStimulusState& SchedulerStimulusStateValue,
                                        const EBlockedTaskWakeKind BlockedTaskWakeKindValue)
{
    switch (BlockedTaskWakeKindValue)
    {
        case EBlockedTaskWakeKind::ProducerAvailability:
            return SchedulerStimulusStateValue.ProducerRevision;
        case EBlockedTaskWakeKind::Resources:
            return SchedulerStimulusStateValue.ResourceRevision;
        case EBlockedTaskWakeKind::Placement:
            return SchedulerStimulusStateValue.PlacementRevision;
        case EBlockedTaskWakeKind::GoalRevision:
            return SchedulerStimulusStateValue.GoalRevision;
        case EBlockedTaskWakeKind::CooldownOnly:
        default:
            return SchedulerStimulusStateValue.GoalRevision;
    }
}

FCommandOrderRecord CreateStrategicOrderFromTaskDescriptor(const FCommandTaskDescriptor& CommandTaskDescriptorValue)
{
    FCommandOrderRecord StrategicOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::StrategicDirector, NullTag, CommandTaskDescriptorValue.ActionAbilityId,
        CommandTaskDescriptorValue.BasePriorityValue, EIntentDomain::StructureBuild,
        CommandTaskDescriptorValue.TriggerMinGameLoop);
    StrategicOrderValue.TaskPackageKind = CommandTaskDescriptorValue.PackageKind;
    StrategicOrderValue.TaskNeedKind = CommandTaskDescriptorValue.NeedKind;
    StrategicOrderValue.TaskType = CommandTaskDescriptorValue.TaskType;
    StrategicOrderValue.RetentionPolicy = CommandTaskDescriptorValue.RetentionPolicy;
    StrategicOrderValue.BlockedTaskWakeKind = CommandTaskDescriptorValue.BlockedTaskWakeKind;
    StrategicOrderValue.SourceGoalId = CommandTaskDescriptorValue.SourceGoalId;
    StrategicOrderValue.PlanStepId = CommandTaskDescriptorValue.TaskId;
    StrategicOrderValue.TargetCount = CommandTaskDescriptorValue.ActionTargetCount;
    StrategicOrderValue.RequestedQueueCount = CommandTaskDescriptorValue.ActionRequestedQueueCount;
    StrategicOrderValue.ProducerUnitTypeId = CommandTaskDescriptorValue.ActionProducerUnitTypeId;
    StrategicOrderValue.ResultUnitTypeId = CommandTaskDescriptorValue.ActionResultUnitTypeId;
    StrategicOrderValue.UpgradeId = CommandTaskDescriptorValue.ActionUpgradeId;
    StrategicOrderValue.PreferredPlacementSlotType = CommandTaskDescriptorValue.ActionPreferredPlacementSlotType;
    StrategicOrderValue.PreferredPlacementSlotId = CommandTaskDescriptorValue.ActionPreferredPlacementSlotId;
    StrategicOrderValue.IntentDomain = DetermineIntentDomainFromAbility(StrategicOrderValue);
    return StrategicOrderValue;
}

bool CanAdmitOrder(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                   const FCommandOrderRecord& CommandOrderRecordValue)
{
    if (CommandOrderRecordValue.RetentionPolicy == ECommandTaskRetentionPolicy::HotMustRun)
    {
        return true;
    }

    const size_t ActiveOrderCountValue =
        CommandAuthoritySchedulingStateValue.GetActiveOrderCountForLayer(CommandOrderRecordValue.SourceLayer);
    const size_t HotOrderCapValue =
        GetHotOrderLayerCap(CommandAuthoritySchedulingStateValue, CommandOrderRecordValue.SourceLayer);
    return ActiveOrderCountValue < HotOrderCapValue;
}

void RetireParkedOrder(FGameStateDescriptor& GameStateDescriptorValue, const uint32_t OrderIdValue)
{
    if (OrderIdValue == 0U)
    {
        return;
    }

    GameStateDescriptorValue.CommandAuthoritySchedulingState.SetOrderLifecycleState(OrderIdValue,
                                                                                    EOrderLifecycleState::Expired);
    GameStateDescriptorValue.CommandAuthoritySchedulingState.ClearOrderDeferralState(OrderIdValue);
}

}  // namespace

void FTerranCommandTaskAdmissionService::RefreshStimulusState(FGameStateDescriptor& GameStateDescriptorValue) const
{
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState;
    FSchedulerStimulusState& SchedulerStimulusStateValue = CommandAuthoritySchedulingStateValue.SchedulerStimulusState;

    const uint64_t GoalFingerprintValue = BuildGoalFingerprint(GameStateDescriptorValue);
    if (GoalFingerprintValue != SchedulerStimulusStateValue.LastGoalFingerprint)
    {
        SchedulerStimulusStateValue.LastGoalFingerprint = GoalFingerprintValue;
        ++SchedulerStimulusStateValue.GoalRevision;
        CommandAuthoritySchedulingStateValue.bPrioritiesDirty = true;
    }

    if (GameStateDescriptorValue.BuildPlanning.AvailableMinerals != SchedulerStimulusStateValue.LastAvailableMinerals ||
        GameStateDescriptorValue.BuildPlanning.AvailableVespene != SchedulerStimulusStateValue.LastAvailableVespene ||
        GameStateDescriptorValue.BuildPlanning.AvailableSupply != SchedulerStimulusStateValue.LastAvailableSupply)
    {
        SchedulerStimulusStateValue.LastAvailableMinerals = GameStateDescriptorValue.BuildPlanning.AvailableMinerals;
        SchedulerStimulusStateValue.LastAvailableVespene = GameStateDescriptorValue.BuildPlanning.AvailableVespene;
        SchedulerStimulusStateValue.LastAvailableSupply = GameStateDescriptorValue.BuildPlanning.AvailableSupply;
        ++SchedulerStimulusStateValue.ResourceRevision;
        CommandAuthoritySchedulingStateValue.bPrioritiesDirty = true;
    }

    if (GameStateDescriptorValue.MacroState.WorkerCount != SchedulerStimulusStateValue.LastWorkerCount ||
        GameStateDescriptorValue.BuildPlanning.ObservedTownHallCount != SchedulerStimulusStateValue.LastTownHallCount ||
        GameStateDescriptorValue.MacroState.BarracksCount != SchedulerStimulusStateValue.LastBarracksCount ||
        GameStateDescriptorValue.MacroState.FactoryCount != SchedulerStimulusStateValue.LastFactoryCount ||
        GameStateDescriptorValue.MacroState.StarportCount != SchedulerStimulusStateValue.LastStarportCount)
    {
        SchedulerStimulusStateValue.LastWorkerCount = GameStateDescriptorValue.MacroState.WorkerCount;
        SchedulerStimulusStateValue.LastTownHallCount = GameStateDescriptorValue.BuildPlanning.ObservedTownHallCount;
        SchedulerStimulusStateValue.LastBarracksCount = GameStateDescriptorValue.MacroState.BarracksCount;
        SchedulerStimulusStateValue.LastFactoryCount = GameStateDescriptorValue.MacroState.FactoryCount;
        SchedulerStimulusStateValue.LastStarportCount = GameStateDescriptorValue.MacroState.StarportCount;
        ++SchedulerStimulusStateValue.ProducerRevision;
        CommandAuthoritySchedulingStateValue.bPrioritiesDirty = true;
    }

    const uint64_t PlacementFingerprintValue = BuildPlacementFingerprint(GameStateDescriptorValue);
    if (PlacementFingerprintValue != SchedulerStimulusStateValue.LastPlacementFingerprint)
    {
        SchedulerStimulusStateValue.LastPlacementFingerprint = PlacementFingerprintValue;
        ++SchedulerStimulusStateValue.PlacementRevision;
        CommandAuthoritySchedulingStateValue.bPrioritiesDirty = true;
    }

    const uint64_t ArmyMissionFingerprintValue = BuildArmyMissionFingerprint(GameStateDescriptorValue);
    if (ArmyMissionFingerprintValue != SchedulerStimulusStateValue.LastArmyMissionFingerprint)
    {
        SchedulerStimulusStateValue.LastArmyMissionFingerprint = ArmyMissionFingerprintValue;
        ++SchedulerStimulusStateValue.ArmyMissionRevision;
        CommandAuthoritySchedulingStateValue.bPrioritiesDirty = true;
    }
}

void FTerranCommandTaskAdmissionService::ReactivateBlockedTasks(FGameStateDescriptor& GameStateDescriptorValue) const
{
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState;
    std::vector<FBlockedTaskRecord> ReactivatedTaskRecordsValue;
    ReactivatedTaskRecordsValue.reserve(CommandAuthoritySchedulingStateValue.MaxBlockedStrategicTasks +
                                        CommandAuthoritySchedulingStateValue.MaxBlockedPlanningTasks);

    CommandAuthoritySchedulingStateValue.BlockedStrategicTasks.CollectReactivatableRecords(
        GameStateDescriptorValue.CurrentGameLoop, CommandAuthoritySchedulingStateValue.SchedulerStimulusState,
        ReactivatedTaskRecordsValue);
    CommandAuthoritySchedulingStateValue.BlockedPlanningTasks.CollectReactivatableRecords(
        GameStateDescriptorValue.CurrentGameLoop, CommandAuthoritySchedulingStateValue.SchedulerStimulusState,
        ReactivatedTaskRecordsValue);

    if (ReactivatedTaskRecordsValue.empty())
    {
        return;
    }

    CommandAuthoritySchedulingStateValue.ReactivatedBlockedTaskCount +=
        static_cast<uint32_t>(ReactivatedTaskRecordsValue.size());
    CommandAuthoritySchedulingStateValue.bPrioritiesDirty = true;
}

bool FTerranCommandTaskAdmissionService::TryAdmitOpeningTask(
    FGameStateDescriptor& GameStateDescriptorValue, const FCommandTaskDescriptor& CommandTaskDescriptorValue,
    uint32_t& OutOrderIdValue) const
{
    OutOrderIdValue = 0U;
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState;

    if (DoesHotOrderMatchOpeningTask(CommandAuthoritySchedulingStateValue, CommandTaskDescriptorValue))
    {
        return false;
    }

    const FCommandOrderRecord StrategicOrderValue = CreateStrategicOrderFromTaskDescriptor(CommandTaskDescriptorValue);
    if (HasEquivalentBlockedTask(CommandAuthoritySchedulingStateValue, StrategicOrderValue) ||
        !CanAdmitOrder(CommandAuthoritySchedulingStateValue, StrategicOrderValue))
    {
        return false;
    }

    OutOrderIdValue = CommandAuthoritySchedulingStateValue.EnqueueOrder(StrategicOrderValue);
    return true;
}

bool FTerranCommandTaskAdmissionService::TryAdmitGoalDrivenOrder(
    FGameStateDescriptor& GameStateDescriptorValue, const FCommandOrderRecord& CommandOrderRecordValue,
    uint32_t& OutOrderIdValue) const
{
    OutOrderIdValue = 0U;
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState;

    if (DoesHotOrderMatchOrderSignature(CommandAuthoritySchedulingStateValue, CommandOrderRecordValue) ||
        HasEquivalentBlockedTask(CommandAuthoritySchedulingStateValue, CommandOrderRecordValue) ||
        !CanAdmitOrder(CommandAuthoritySchedulingStateValue, CommandOrderRecordValue))
    {
        return false;
    }

    OutOrderIdValue = CommandAuthoritySchedulingStateValue.EnqueueOrder(CommandOrderRecordValue);
    return true;
}

void FTerranCommandTaskAdmissionService::ParkDeferredOrders(FGameStateDescriptor& GameStateDescriptorValue) const
{
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState;
    const size_t OrderCountValue = CommandAuthoritySchedulingStateValue.OrderIds.size();

    for (size_t OrderIndexValue = 0U; OrderIndexValue < OrderCountValue; ++OrderIndexValue)
    {
        if (IsTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        const ECommandAuthorityLayer SourceLayerValue = CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue];
        if (SourceLayerValue != ECommandAuthorityLayer::EconomyAndProduction &&
            SourceLayerValue != ECommandAuthorityLayer::StrategicDirector)
        {
            continue;
        }

        const FCommandOrderRecord DeferredOrderValue = CommandAuthoritySchedulingStateValue.GetOrderRecord(OrderIndexValue);
        if (!ShouldParkDeferredOrder(DeferredOrderValue))
        {
            continue;
        }

        FCommandOrderRecord RootOrderValue = DeferredOrderValue;
        if (DeferredOrderValue.SourceLayer == ECommandAuthorityLayer::EconomyAndProduction &&
            DeferredOrderValue.ParentOrderId != 0U)
        {
            size_t ParentOrderIndexValue = 0U;
            if (CommandAuthoritySchedulingStateValue.TryGetOrderIndex(DeferredOrderValue.ParentOrderId,
                                                                      ParentOrderIndexValue) &&
                !IsTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[ParentOrderIndexValue]) &&
                IsStrategicBufferLayer(CommandAuthoritySchedulingStateValue.SourceLayers[ParentOrderIndexValue]))
            {
                RootOrderValue = CommandAuthoritySchedulingStateValue.GetOrderRecord(ParentOrderIndexValue);
            }
        }

        const EBlockedTaskWakeKind WakeKindValue = CreateBlockedTaskRecord(
                                                       RootOrderValue, DeferredOrderValue.LastDeferralReason,
                                                       GameStateDescriptorValue.CurrentGameLoop, 0U)
                                                       .WakeKind;
        const uint64_t LastSeenStimulusRevisionValue = GetStimulusRevisionForWakeKind(
            CommandAuthoritySchedulingStateValue.SchedulerStimulusState, WakeKindValue);
        const FBlockedTaskRecord BlockedTaskRecordValue = CreateBlockedTaskRecord(
            RootOrderValue, DeferredOrderValue.LastDeferralReason, GameStateDescriptorValue.CurrentGameLoop,
            LastSeenStimulusRevisionValue);

        FBlockedTaskRingBuffer& BlockedTaskRingBufferValue =
            IsStrategicBufferLayer(RootOrderValue.SourceLayer) ? CommandAuthoritySchedulingStateValue.BlockedStrategicTasks
                                                               : CommandAuthoritySchedulingStateValue.BlockedPlanningTasks;

        bool bCoalescedValue = false;
        bool bDroppedValue = false;
        bool bRejectedMustRunValue = false;
        const bool BufferedValue = BlockedTaskRingBufferValue.TryPushOrCoalesce(BlockedTaskRecordValue, bCoalescedValue,
                                                                                bDroppedValue, bRejectedMustRunValue);

        if (bCoalescedValue)
        {
            ++CommandAuthoritySchedulingStateValue.CoalescedBlockedTaskCount;
        }
        else if (BufferedValue)
        {
            ++CommandAuthoritySchedulingStateValue.BufferedBlockedTaskCount;
        }
        else if (bRejectedMustRunValue)
        {
            ++CommandAuthoritySchedulingStateValue.RejectedMustRunBlockedTaskCount;
        }
        else if (bDroppedValue)
        {
            ++CommandAuthoritySchedulingStateValue.DroppedBlockedTaskCount;
        }

        if (bRejectedMustRunValue)
        {
            continue;
        }

        if (RootOrderValue.PlanStepId != 0U)
        {
            GameStateDescriptorValue.OpeningPlanExecutionState.UnseedStep(RootOrderValue.PlanStepId);
        }

        RetireParkedOrder(GameStateDescriptorValue, DeferredOrderValue.OrderId);
        if (RootOrderValue.OrderId != DeferredOrderValue.OrderId)
        {
            RetireParkedOrder(GameStateDescriptorValue, RootOrderValue.OrderId);
        }
    }
}

}  // namespace sc2
