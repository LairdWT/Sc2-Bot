#include "common/planning/FTerranCommandTaskAdmissionService.h"

#include <array>
#include <vector>

#include "common/descriptors/FGameStateDescriptor.h"
#include "common/economy/EconomyForecastConstants.h"
#include "common/planning/FBlockedTaskRecord.h"
#include "common/planning/FCommandAuthoritySchedulingState.h"
#include "common/planning/FCommandOrderRecord.h"
#include "common/planning/FCommandTaskSignatureKey.h"

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

uint64_t QuantizePositiveRate(const float RateValue)
{
    return RateValue > 0.0f ? static_cast<uint64_t>((RateValue * 1000.0f) + 0.5f) : 0U;
}

uint64_t BuildResourceFingerprint(const FGameStateDescriptor& GameStateDescriptorValue)
{
    const FEconomyStateDescriptor& EconomyStateDescriptorValue = GameStateDescriptorValue.EconomyState;
    uint64_t ResourceFingerprintValue = 0U;
    ResourceFingerprintValue =
        CombineFingerprint(ResourceFingerprintValue, EconomyStateDescriptorValue.BudgetedMinerals);
    ResourceFingerprintValue =
        CombineFingerprint(ResourceFingerprintValue, EconomyStateDescriptorValue.BudgetedVespene);
    ResourceFingerprintValue =
        CombineFingerprint(ResourceFingerprintValue, EconomyStateDescriptorValue.BudgetedSupplyAvailable);
    ResourceFingerprintValue =
        CombineFingerprint(ResourceFingerprintValue, GameStateDescriptorValue.BuildPlanning.AvailableMinerals);
    ResourceFingerprintValue =
        CombineFingerprint(ResourceFingerprintValue, GameStateDescriptorValue.BuildPlanning.AvailableVespene);
    ResourceFingerprintValue =
        CombineFingerprint(ResourceFingerprintValue, GameStateDescriptorValue.BuildPlanning.AvailableSupply);
    ResourceFingerprintValue = CombineFingerprint(
        ResourceFingerprintValue,
        EconomyStateDescriptorValue.ProjectedAvailableMineralsByHorizon[ShortForecastHorizonIndexValue]);
    ResourceFingerprintValue = CombineFingerprint(
        ResourceFingerprintValue,
        EconomyStateDescriptorValue.ProjectedAvailableVespeneByHorizon[ShortForecastHorizonIndexValue]);
    ResourceFingerprintValue = CombineFingerprint(
        ResourceFingerprintValue,
        EconomyStateDescriptorValue.ProjectedAvailableSupplyByHorizon[ShortForecastHorizonIndexValue]);
    ResourceFingerprintValue =
        CombineFingerprint(ResourceFingerprintValue,
                           EconomyStateDescriptorValue.GrossMineralIncomeByHorizon[ShortForecastHorizonIndexValue]);
    ResourceFingerprintValue =
        CombineFingerprint(ResourceFingerprintValue,
                           EconomyStateDescriptorValue.GrossVespeneIncomeByHorizon[ShortForecastHorizonIndexValue]);
    ResourceFingerprintValue = CombineFingerprint(
        ResourceFingerprintValue,
        QuantizePositiveRate(
            EconomyStateDescriptorValue.GrossMineralIncomeAverageByHorizon[ShortForecastHorizonIndexValue]));
    ResourceFingerprintValue = CombineFingerprint(
        ResourceFingerprintValue,
        QuantizePositiveRate(
            EconomyStateDescriptorValue.GrossVespeneIncomeAverageByHorizon[ShortForecastHorizonIndexValue]));
    ResourceFingerprintValue =
        CombineFingerprint(ResourceFingerprintValue,
                           static_cast<uint64_t>(GameStateDescriptorValue.SchedulerOutlook.ExpectedSupplyCapDelta));
    ResourceFingerprintValue =
        CombineFingerprint(ResourceFingerprintValue,
                           static_cast<uint64_t>(GameStateDescriptorValue.SchedulerOutlook.ExpectedSupplyUsedDelta));
    return ResourceFingerprintValue;
}

uint64_t BuildProducerFingerprint(const FGameStateDescriptor& GameStateDescriptorValue)
{
    const FProductionStateDescriptor& ProductionStateDescriptorValue = GameStateDescriptorValue.ProductionState;
    uint64_t ProducerFingerprintValue = 0U;
    ProducerFingerprintValue =
        CombineFingerprint(ProducerFingerprintValue, GameStateDescriptorValue.MacroState.WorkerCount);
    ProducerFingerprintValue =
        CombineFingerprint(ProducerFingerprintValue, GameStateDescriptorValue.BuildPlanning.ObservedTownHallCount);
    ProducerFingerprintValue =
        CombineFingerprint(ProducerFingerprintValue, GameStateDescriptorValue.MacroState.BarracksCount);
    ProducerFingerprintValue =
        CombineFingerprint(ProducerFingerprintValue, GameStateDescriptorValue.MacroState.FactoryCount);
    ProducerFingerprintValue =
        CombineFingerprint(ProducerFingerprintValue, GameStateDescriptorValue.MacroState.StarportCount);
    ProducerFingerprintValue = CombineFingerprint(
        ProducerFingerprintValue,
        ProductionStateDescriptorValue.GetProjectedUnitCount(UNIT_TYPEID::TERRAN_SCV));
    ProducerFingerprintValue = CombineFingerprint(
        ProducerFingerprintValue,
        ProductionStateDescriptorValue.GetProjectedBuildingCount(UNIT_TYPEID::TERRAN_COMMANDCENTER));
    ProducerFingerprintValue = CombineFingerprint(
        ProducerFingerprintValue,
        ProductionStateDescriptorValue.GetProjectedBuildingCount(UNIT_TYPEID::TERRAN_BARRACKS));
    ProducerFingerprintValue = CombineFingerprint(
        ProducerFingerprintValue,
        ProductionStateDescriptorValue.GetProjectedBuildingCount(UNIT_TYPEID::TERRAN_FACTORY));
    ProducerFingerprintValue = CombineFingerprint(
        ProducerFingerprintValue,
        ProductionStateDescriptorValue.GetProjectedBuildingCount(UNIT_TYPEID::TERRAN_STARPORT));
    ProducerFingerprintValue =
        CombineFingerprint(ProducerFingerprintValue, ProductionStateDescriptorValue.CurrentTownHallCapacity);
    ProducerFingerprintValue =
        CombineFingerprint(ProducerFingerprintValue, ProductionStateDescriptorValue.CurrentTownHallOccupancy);
    ProducerFingerprintValue =
        CombineFingerprint(ProducerFingerprintValue, ProductionStateDescriptorValue.NearTermTownHallCapacity);
    ProducerFingerprintValue =
        CombineFingerprint(ProducerFingerprintValue, ProductionStateDescriptorValue.CurrentBarracksCapacity);
    ProducerFingerprintValue =
        CombineFingerprint(ProducerFingerprintValue, ProductionStateDescriptorValue.CurrentBarracksOccupancy);
    ProducerFingerprintValue =
        CombineFingerprint(ProducerFingerprintValue, ProductionStateDescriptorValue.NearTermBarracksCapacity);
    ProducerFingerprintValue =
        CombineFingerprint(ProducerFingerprintValue, ProductionStateDescriptorValue.CurrentFactoryCapacity);
    ProducerFingerprintValue =
        CombineFingerprint(ProducerFingerprintValue, ProductionStateDescriptorValue.CurrentFactoryOccupancy);
    ProducerFingerprintValue =
        CombineFingerprint(ProducerFingerprintValue, ProductionStateDescriptorValue.NearTermFactoryCapacity);
    ProducerFingerprintValue =
        CombineFingerprint(ProducerFingerprintValue, ProductionStateDescriptorValue.CurrentStarportCapacity);
    ProducerFingerprintValue =
        CombineFingerprint(ProducerFingerprintValue, ProductionStateDescriptorValue.CurrentStarportOccupancy);
    ProducerFingerprintValue =
        CombineFingerprint(ProducerFingerprintValue, ProductionStateDescriptorValue.NearTermStarportCapacity);
    ProducerFingerprintValue = CombineFingerprint(
        ProducerFingerprintValue,
        QuantizePositiveRate(
            ProductionStateDescriptorValue.TownHallThroughputAveragesByHorizon[ShortForecastHorizonIndexValue]));
    ProducerFingerprintValue = CombineFingerprint(
        ProducerFingerprintValue,
        QuantizePositiveRate(
            ProductionStateDescriptorValue.BarracksThroughputAveragesByHorizon[ShortForecastHorizonIndexValue]));
    ProducerFingerprintValue = CombineFingerprint(
        ProducerFingerprintValue,
        QuantizePositiveRate(
            ProductionStateDescriptorValue.FactoryThroughputAveragesByHorizon[ShortForecastHorizonIndexValue]));
    ProducerFingerprintValue = CombineFingerprint(
        ProducerFingerprintValue,
        QuantizePositiveRate(
            ProductionStateDescriptorValue.StarportThroughputAveragesByHorizon[ShortForecastHorizonIndexValue]));
    return ProducerFingerprintValue;
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

bool DoesHotOrderMatchOpeningTask(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                  const FCommandTaskDescriptor& CommandTaskDescriptorValue)
{
    return CommandAuthoritySchedulingStateValue.HasEquivalentActiveTaskSignature(
        FCommandTaskSignatureKey::FromTaskDescriptor(CommandTaskDescriptorValue));
}

bool DoesHotOrderMatchOrderSignature(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                     const FCommandOrderRecord& CommandOrderRecordValue)
{
    return CommandAuthoritySchedulingStateValue.HasEquivalentActiveTaskSignature(
        FCommandTaskSignatureKey::FromOrderRecord(CommandOrderRecordValue));
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
    BlockedTaskRecordValue.SourceLayer = CommandOrderRecordValue.SourceLayer;
    BlockedTaskRecordValue.PackageKind = CommandOrderRecordValue.TaskPackageKind;
    BlockedTaskRecordValue.NeedKind = CommandOrderRecordValue.TaskNeedKind;
    BlockedTaskRecordValue.TaskType = CommandOrderRecordValue.TaskType;
    BlockedTaskRecordValue.Origin = CommandOrderRecordValue.Origin;
    BlockedTaskRecordValue.RetentionPolicy = CommandOrderRecordValue.RetentionPolicy;
    BlockedTaskRecordValue.BasePriorityValue = CommandOrderRecordValue.BasePriorityValue;
    BlockedTaskRecordValue.AbilityId = CommandOrderRecordValue.AbilityId;
    BlockedTaskRecordValue.ProducerUnitTypeId = CommandOrderRecordValue.ProducerUnitTypeId;
    BlockedTaskRecordValue.ResultUnitTypeId = CommandOrderRecordValue.ResultUnitTypeId;
    BlockedTaskRecordValue.UpgradeId = CommandOrderRecordValue.UpgradeId;
    BlockedTaskRecordValue.PreferredPlacementSlotId = CommandOrderRecordValue.PreferredPlacementSlotId;
    BlockedTaskRecordValue.BlockingReason = DeferralReasonValue;
    BlockedTaskRecordValue.RequestedQueueCount = CommandOrderRecordValue.RequestedQueueCount;
    BlockedTaskRecordValue.TargetCount = CommandOrderRecordValue.TargetCount;
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
    StrategicOrderValue.Origin = CommandTaskDescriptorValue.Origin;
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

FCommandOrderRecord CreateOrderFromBlockedTaskRecord(const FBlockedTaskRecord& BlockedTaskRecordValue,
                                                     const uint64_t CurrentGameLoopValue)
{
    FCommandOrderRecord CommandOrderRecordValue = FCommandOrderRecord::CreateNoTarget(
        BlockedTaskRecordValue.SourceLayer, NullTag, BlockedTaskRecordValue.AbilityId,
        BlockedTaskRecordValue.BasePriorityValue, EIntentDomain::StructureBuild, CurrentGameLoopValue);
    CommandOrderRecordValue.SourceGoalId = BlockedTaskRecordValue.SourceGoalId;
    CommandOrderRecordValue.TaskPackageKind = BlockedTaskRecordValue.PackageKind;
    CommandOrderRecordValue.TaskNeedKind = BlockedTaskRecordValue.NeedKind;
    CommandOrderRecordValue.TaskType = BlockedTaskRecordValue.TaskType;
    CommandOrderRecordValue.Origin = BlockedTaskRecordValue.Origin;
    CommandOrderRecordValue.RetentionPolicy = BlockedTaskRecordValue.RetentionPolicy;
    CommandOrderRecordValue.BlockedTaskWakeKind = BlockedTaskRecordValue.WakeKind;
    CommandOrderRecordValue.PlanStepId = BlockedTaskRecordValue.TaskId;
    CommandOrderRecordValue.TargetCount = BlockedTaskRecordValue.TargetCount;
    CommandOrderRecordValue.RequestedQueueCount = BlockedTaskRecordValue.RequestedQueueCount;
    CommandOrderRecordValue.ProducerUnitTypeId = BlockedTaskRecordValue.ProducerUnitTypeId;
    CommandOrderRecordValue.ResultUnitTypeId = BlockedTaskRecordValue.ResultUnitTypeId;
    CommandOrderRecordValue.UpgradeId = BlockedTaskRecordValue.UpgradeId;
    CommandOrderRecordValue.PreferredPlacementSlotId = BlockedTaskRecordValue.PreferredPlacementSlotId;
    CommandOrderRecordValue.PreferredPlacementSlotType = BlockedTaskRecordValue.PreferredPlacementSlotId.SlotType;
    CommandOrderRecordValue.IntentDomain = DetermineIntentDomainFromAbility(CommandOrderRecordValue);
    return CommandOrderRecordValue;
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
    CommandAuthoritySchedulingStateValue.AdvanceRecentBlockedTaskCounterWindow(
        GameStateDescriptorValue.CurrentStep);

    const uint64_t GoalFingerprintValue = BuildGoalFingerprint(GameStateDescriptorValue);
    if (GoalFingerprintValue != SchedulerStimulusStateValue.LastGoalFingerprint)
    {
        SchedulerStimulusStateValue.LastGoalFingerprint = GoalFingerprintValue;
        ++SchedulerStimulusStateValue.GoalRevision;
        CommandAuthoritySchedulingStateValue.bPrioritiesDirty = true;
    }

    const uint64_t ResourceFingerprintValue = BuildResourceFingerprint(GameStateDescriptorValue);
    if (ResourceFingerprintValue != SchedulerStimulusStateValue.LastResourceFingerprint)
    {
        SchedulerStimulusStateValue.LastResourceFingerprint = ResourceFingerprintValue;
        ++SchedulerStimulusStateValue.ResourceRevision;
        CommandAuthoritySchedulingStateValue.bPrioritiesDirty = true;
    }

    const uint64_t ProducerFingerprintValue = BuildProducerFingerprint(GameStateDescriptorValue);
    if (ProducerFingerprintValue != SchedulerStimulusStateValue.LastProducerFingerprint)
    {
        SchedulerStimulusStateValue.LastProducerFingerprint = ProducerFingerprintValue;
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

    for (const FBlockedTaskRecord& BlockedTaskRecordValue : ReactivatedTaskRecordsValue)
    {
        const FCommandOrderRecord ReactivatedOrderValue =
            CreateOrderFromBlockedTaskRecord(BlockedTaskRecordValue, GameStateDescriptorValue.CurrentGameLoop);
        if (DoesHotOrderMatchOrderSignature(CommandAuthoritySchedulingStateValue, ReactivatedOrderValue))
        {
            continue;
        }

        if (!CanAdmitOrder(CommandAuthoritySchedulingStateValue, ReactivatedOrderValue))
        {
            FBlockedTaskRecord DeferredBlockedTaskRecordValue = BlockedTaskRecordValue;
            DeferredBlockedTaskRecordValue.LastSeenStimulusRevision = GetStimulusRevisionForWakeKind(
                CommandAuthoritySchedulingStateValue.SchedulerStimulusState, BlockedTaskRecordValue.WakeKind);
            DeferredBlockedTaskRecordValue.NextEligibleGameLoop =
                std::max<uint64_t>(DeferredBlockedTaskRecordValue.NextEligibleGameLoop,
                                   GameStateDescriptorValue.CurrentGameLoop + 8U);
            FBlockedTaskRingBuffer& BlockedTaskRingBufferValue =
                IsStrategicBufferLayer(BlockedTaskRecordValue.SourceLayer)
                    ? CommandAuthoritySchedulingStateValue.BlockedStrategicTasks
                    : CommandAuthoritySchedulingStateValue.BlockedPlanningTasks;
            bool bCoalescedValue = false;
            bool bDroppedValue = false;
            bool bRejectedMustRunValue = false;
            BlockedTaskRingBufferValue.TryPushOrCoalesce(DeferredBlockedTaskRecordValue, bCoalescedValue,
                                                         bDroppedValue, bRejectedMustRunValue);
            continue;
        }

        const uint32_t ReactivatedOrderIdValue =
            CommandAuthoritySchedulingStateValue.EnqueueOrder(ReactivatedOrderValue);
        if (BlockedTaskRecordValue.TaskId != 0U)
        {
            GameStateDescriptorValue.OpeningPlanExecutionState.RecordSeededStep(BlockedTaskRecordValue.TaskId,
                                                                                ReactivatedOrderIdValue);
        }

        ++CommandAuthoritySchedulingStateValue.TotalReactivatedBlockedTaskCount;
        ++CommandAuthoritySchedulingStateValue.RecentReactivatedBlockedTaskCount;
    }

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
            ++CommandAuthoritySchedulingStateValue.TotalCoalescedBlockedTaskCount;
            ++CommandAuthoritySchedulingStateValue.RecentCoalescedBlockedTaskCount;
        }
        else if (BufferedValue)
        {
            ++CommandAuthoritySchedulingStateValue.TotalBufferedBlockedTaskCount;
            ++CommandAuthoritySchedulingStateValue.RecentBufferedBlockedTaskCount;
        }
        else if (bRejectedMustRunValue)
        {
            ++CommandAuthoritySchedulingStateValue.TotalRejectedMustRunBlockedTaskCount;
            ++CommandAuthoritySchedulingStateValue.RecentRejectedMustRunBlockedTaskCount;
        }
        else if (bDroppedValue)
        {
            ++CommandAuthoritySchedulingStateValue.TotalDroppedBlockedTaskCount;
            ++CommandAuthoritySchedulingStateValue.RecentDroppedBlockedTaskCount;
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
