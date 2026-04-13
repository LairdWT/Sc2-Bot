#include "terran.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <unordered_map>

#include "sc2lib/sc2_search.h"

namespace sc2
{
namespace
{

constexpr float WallStructureMatchRadiusSquaredValue = 6.25f;
constexpr int GasHarvestIntentPriorityValue = 320;
constexpr int GasReliefIntentPriorityValue = 321;
constexpr int MineralRebalanceIntentPriorityValue = 322;
constexpr uint64_t TerminalOrderCompactionIntervalStepCountValue = 120U;
constexpr size_t TerminalOrderCompactionTriggerCountValue = 512U;
constexpr uint64_t RecentProductionRallyCounterWindowStepCountValue = 120U;
using FSteadyClock = std::chrono::steady_clock;
using FSteadyTimePoint = std::chrono::time_point<FSteadyClock>;

uint64_t GetElapsedMicroseconds(const FSteadyTimePoint& StartTimeValue, const FSteadyTimePoint& EndTimeValue)
{
    return static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(EndTimeValue - StartTimeValue).count());
}

EExecutionConditionState GetExecutionConditionState(const bool ConditionValue)
{
    return ConditionValue ? EExecutionConditionState::Active : EExecutionConditionState::Inactive;
}

bool IsProductionRallyStructureType(const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_BARRACKS:
        case UNIT_TYPEID::TERRAN_FACTORY:
        case UNIT_TYPEID::TERRAN_STARPORT:
            return true;
        default:
            return false;
    }
}

AbilityID GetProductionStructureRallyAbility(const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_BARRACKS:
        case UNIT_TYPEID::TERRAN_FACTORY:
        case UNIT_TYPEID::TERRAN_STARPORT:
            return ABILITY_ID::RALLY_UNITS;
        default:
            return ABILITY_ID::INVALID;
    }
}

bool IsTerranAddonBuildAbility(const ABILITY_ID AbilityIdValue)
{
    switch (AbilityIdValue)
    {
        case ABILITY_ID::BUILD_REACTOR_BARRACKS:
        case ABILITY_ID::BUILD_TECHLAB_BARRACKS:
        case ABILITY_ID::BUILD_REACTOR_FACTORY:
        case ABILITY_ID::BUILD_TECHLAB_FACTORY:
        case ABILITY_ID::BUILD_REACTOR_STARPORT:
        case ABILITY_ID::BUILD_TECHLAB_STARPORT:
            return true;
        default:
            return false;
    }
}

bool IsWallDepotUnitType(const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOT:
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED:
            return true;
        default:
            return false;
    }
}

bool HasActiveSchedulerOrderForActorTag(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                        const Tag ActorTagValue)
{
    const size_t OrderCountValue = CommandAuthoritySchedulingStateValue.OrderIds.size();
    for (size_t OrderIndexValue = 0U; OrderIndexValue < OrderCountValue; ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.ActorTags[OrderIndexValue] != ActorTagValue ||
            IsTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        return true;
    }

    return false;
}

bool IsTownHallStructureType(const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
        case UNIT_TYPEID::TERRAN_PLANETARYFORTRESS:
            return true;
        default:
            return false;
    }
}

bool IsIdleStructureWithoutScheduledWork(const Unit& ControlledUnitValue,
                                         const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue)
{
    return ControlledUnitValue.build_progress >= 1.0f && ControlledUnitValue.orders.empty() &&
           !HasActiveSchedulerOrderForActorTag(CommandAuthoritySchedulingStateValue, ControlledUnitValue.tag);
}

void PrintMainLayoutSlotFamily(const char* LabelPtrValue,
                               const std::vector<FBuildPlacementSlot>& BuildPlacementSlotsValue)
{
    std::cout << " | " << LabelPtrValue << " ";
    if (BuildPlacementSlotsValue.empty())
    {
        std::cout << "None";
        return;
    }

    for (size_t SlotIndexValue = 0U; SlotIndexValue < BuildPlacementSlotsValue.size(); ++SlotIndexValue)
    {
        if (SlotIndexValue > 0U)
        {
            std::cout << ",";
        }

        const FBuildPlacementSlot& BuildPlacementSlotValue = BuildPlacementSlotsValue[SlotIndexValue];
        std::cout << SlotIndexValue << ":(" << BuildPlacementSlotValue.BuildPoint.x << ", "
                  << BuildPlacementSlotValue.BuildPoint.y << ")";
    }
}

void PrintGoalList(const char* LabelPtrValue, const std::vector<FGoalDescriptor>& GoalDescriptorsValue)
{
    std::cout << LabelPtrValue << ": ";
    if (GoalDescriptorsValue.empty())
    {
        std::cout << "None";
        return;
    }

    for (size_t GoalIndexValue = 0U; GoalIndexValue < GoalDescriptorsValue.size(); ++GoalIndexValue)
    {
        if (GoalIndexValue > 0U)
        {
            std::cout << " | ";
        }

        const FGoalDescriptor& GoalDescriptorValue = GoalDescriptorsValue[GoalIndexValue];
        std::cout << GoalDescriptorValue.GoalId << ":" << ToString(GoalDescriptorValue.GoalType)
                  << "=" << GoalDescriptorValue.TargetCount;
    }
}

void PrintPriorityTierQueueSummary(const char* LabelPtrValue,
                                   const std::array<std::vector<size_t>, CommandPriorityTierCountValue>& QueueGroupValue)
{
    std::cout << LabelPtrValue << ": ";
    for (size_t PriorityTierIndexValue = 0U; PriorityTierIndexValue < CommandPriorityTierCountValue;
         ++PriorityTierIndexValue)
    {
        if (PriorityTierIndexValue > 0U)
        {
            std::cout << " | ";
        }

        const ECommandPriorityTier CommandPriorityTierValue = static_cast<ECommandPriorityTier>(PriorityTierIndexValue);
        std::cout << ToString(CommandPriorityTierValue) << " " << QueueGroupValue[PriorityTierIndexValue].size();
    }
}

void PrintReadyIntentQueueSummary(
    const std::array<std::array<std::vector<size_t>, IntentDomainCountValue>, CommandPriorityTierCountValue>&
        QueueGroupValue)
{
    std::cout << "ReadyIntents: ";
    for (size_t PriorityTierIndexValue = 0U; PriorityTierIndexValue < CommandPriorityTierCountValue;
         ++PriorityTierIndexValue)
    {
        if (PriorityTierIndexValue > 0U)
        {
            std::cout << " | ";
        }

        uint32_t TierIntentCountValue = 0U;
        for (size_t IntentDomainIndexValue = 0U; IntentDomainIndexValue < IntentDomainCountValue; ++IntentDomainIndexValue)
        {
            TierIntentCountValue +=
                static_cast<uint32_t>(QueueGroupValue[PriorityTierIndexValue][IntentDomainIndexValue].size());
        }

        const ECommandPriorityTier CommandPriorityTierValue = static_cast<ECommandPriorityTier>(PriorityTierIndexValue);
        std::cout << ToString(CommandPriorityTierValue) << " " << TierIntentCountValue;
    }
}

bool IsMandatoryCommitmentClass(const ECommandCommitmentClass CommandCommitmentClassValue)
{
    switch (CommandCommitmentClassValue)
    {
        case ECommandCommitmentClass::MandatoryOpening:
        case ECommandCommitmentClass::MandatoryRecovery:
            return true;
        case ECommandCommitmentClass::FlexibleMacro:
        case ECommandCommitmentClass::Opportunistic:
        default:
            return false;
    }
}

uint32_t CountActiveMandatoryOpeningTasks(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue)
{
    uint32_t ActiveTaskCountValue = 0U;
    const size_t OrderCountValue = CommandAuthoritySchedulingStateValue.OrderIds.size();
    for (size_t OrderIndexValue = 0U; OrderIndexValue < OrderCountValue; ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.CommitmentClasses[OrderIndexValue] !=
                ECommandCommitmentClass::MandatoryOpening ||
            IsTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        ++ActiveTaskCountValue;
    }

    return ActiveTaskCountValue;
}

uint32_t CountBlockedMandatoryTasks(const FBlockedTaskRingBuffer& BlockedTaskRingBufferValue)
{
    uint32_t BlockedTaskCountValue = 0U;
    const size_t BlockedRecordCountValue = BlockedTaskRingBufferValue.GetCount();
    for (size_t BlockedRecordIndexValue = 0U; BlockedRecordIndexValue < BlockedRecordCountValue;
         ++BlockedRecordIndexValue)
    {
        const FBlockedTaskRecord* BlockedTaskRecordPtrValue =
            BlockedTaskRingBufferValue.GetRecordAtOrderedIndex(BlockedRecordIndexValue);
        if (BlockedTaskRecordPtrValue == nullptr ||
            !IsMandatoryCommitmentClass(BlockedTaskRecordPtrValue->CommitmentClass))
        {
            continue;
        }

        ++BlockedTaskCountValue;
    }

    return BlockedTaskCountValue;
}

void PrintMandatorySlotOwners(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue)
{
    std::cout << "Mandatory Slot Owners: ";
    bool HasPrintedOwnerValue = false;
    const size_t OrderCountValue = CommandAuthoritySchedulingStateValue.OrderIds.size();
    for (size_t OrderIndexValue = 0U; OrderIndexValue < OrderCountValue; ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.CommitmentClasses[OrderIndexValue] !=
                ECommandCommitmentClass::MandatoryOpening ||
            IsTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]) ||
            CommandAuthoritySchedulingStateValue.PreferredPlacementSlotIdTypes[OrderIndexValue] ==
                EBuildPlacementSlotType::Unknown)
        {
            continue;
        }

        if (HasPrintedOwnerValue)
        {
            std::cout << " | ";
        }

        FBuildPlacementSlotId BuildPlacementSlotIdValue;
        BuildPlacementSlotIdValue.SlotType =
            CommandAuthoritySchedulingStateValue.PreferredPlacementSlotIdTypes[OrderIndexValue];
        BuildPlacementSlotIdValue.Ordinal =
            CommandAuthoritySchedulingStateValue.PreferredPlacementSlotIdOrdinals[OrderIndexValue];
        std::cout << ToString(BuildPlacementSlotIdValue.SlotType)
                  << ":" << static_cast<uint32_t>(BuildPlacementSlotIdValue.Ordinal)
                  << "=Order" << CommandAuthoritySchedulingStateValue.OrderIds[OrderIndexValue];
        HasPrintedOwnerValue = true;
    }

    if (!HasPrintedOwnerValue)
    {
        std::cout << "None";
    }
}

bool IsProductionRailStructureType(const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_BARRACKS:
        case UNIT_TYPEID::TERRAN_FACTORY:
        case UNIT_TYPEID::TERRAN_STARPORT:
            return true;
        default:
            return false;
    }
}

const Unit* FindProductionRailStructureForSlot(const Units& SelfUnitsValue,
                                               const FBuildPlacementSlot& BuildPlacementSlotValue)
{
    const Unit* BestUnitValue = nullptr;
    float BestDistanceSquaredValue = std::numeric_limits<float>::max();

    for (const Unit* SelfUnitValue : SelfUnitsValue)
    {
        if (SelfUnitValue == nullptr || !SelfUnitValue->is_building || SelfUnitValue->is_flying ||
            !IsProductionRailStructureType(SelfUnitValue->unit_type.ToType()))
        {
            continue;
        }

        const float DistanceSquaredValue =
            DistanceSquared2D(Point2D(SelfUnitValue->pos), BuildPlacementSlotValue.BuildPoint);
        if (DistanceSquaredValue > WallStructureMatchRadiusSquaredValue || DistanceSquaredValue >= BestDistanceSquaredValue)
        {
            continue;
        }

        BestDistanceSquaredValue = DistanceSquaredValue;
        BestUnitValue = SelfUnitValue;
    }

    return BestUnitValue;
}

const char* GetProductionRailOccupancyLabel(const Unit* OccupyingUnitValue)
{
    if (OccupyingUnitValue == nullptr)
    {
        return "Empty";
    }

    switch (OccupyingUnitValue->unit_type.ToType())
    {
        case UNIT_TYPEID::TERRAN_BARRACKS:
            return OccupyingUnitValue->build_progress >= 1.0f ? "Barracks" : "BarracksInProgress";
        case UNIT_TYPEID::TERRAN_FACTORY:
            return OccupyingUnitValue->build_progress >= 1.0f ? "Factory" : "FactoryInProgress";
        case UNIT_TYPEID::TERRAN_STARPORT:
            return OccupyingUnitValue->build_progress >= 1.0f ? "Starport" : "StarportInProgress";
        default:
            return OccupyingUnitValue->build_progress >= 1.0f ? "Occupied" : "OccupiedInProgress";
    }
}

void PrintProductionRailSlots(const std::vector<FBuildPlacementSlot>& BuildPlacementSlotsValue,
                              const Units& SelfUnitsValue)
{
    std::cout << " | ProductionRail ";
    if (BuildPlacementSlotsValue.empty())
    {
        std::cout << "None";
        return;
    }

    for (size_t SlotIndexValue = 0U; SlotIndexValue < BuildPlacementSlotsValue.size(); ++SlotIndexValue)
    {
        if (SlotIndexValue > 0U)
        {
            std::cout << ",";
        }

        const FBuildPlacementSlot& BuildPlacementSlotValue = BuildPlacementSlotsValue[SlotIndexValue];
        const Unit* OccupyingUnitValue = FindProductionRailStructureForSlot(SelfUnitsValue, BuildPlacementSlotValue);
        std::cout << static_cast<uint32_t>(BuildPlacementSlotValue.SlotId.Ordinal)
                  << ":" << GetProductionRailOccupancyLabel(OccupyingUnitValue)
                  << "@(" << BuildPlacementSlotValue.BuildPoint.x << ", "
                  << BuildPlacementSlotValue.BuildPoint.y << ")";
    }
}

bool IsRefineryUnitType(const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_REFINERY:
        case UNIT_TYPEID::TERRAN_REFINERYRICH:
            return true;
        default:
            return false;
    }
}

bool IsHarvestGatherAbility(const ABILITY_ID AbilityIdValue)
{
    switch (AbilityIdValue)
    {
        case ABILITY_ID::HARVEST_GATHER:
        case ABILITY_ID::HARVEST_GATHER_SCV:
            return true;
        default:
            return false;
    }
}

bool IsHarvestReturnAbility(const ABILITY_ID AbilityIdValue)
{
    switch (AbilityIdValue)
    {
        case ABILITY_ID::HARVEST_RETURN:
        case ABILITY_ID::HARVEST_RETURN_SCV:
            return true;
        default:
            return false;
    }
}

uint32_t CountRecoveryMoveIntents(const FIntentBuffer& IntentBufferValue)
{
    uint32_t IntentCountValue = 0U;
    for (const FUnitIntent& IntentValue : IntentBufferValue.Intents)
    {
        if (IntentValue.Domain == EIntentDomain::Recovery &&
            IntentValue.Ability == ABILITY_ID::GENERAL_MOVE)
        {
            ++IntentCountValue;
        }
    }

    return IntentCountValue;
}

bool IsWorkerCommittedToConstruction(const Unit& WorkerUnitValue)
{
    if (WorkerUnitValue.orders.empty())
    {
        return false;
    }

    switch (WorkerUnitValue.orders.front().ability_id.ToType())
    {
        case ABILITY_ID::BUILD_SUPPLYDEPOT:
        case ABILITY_ID::BUILD_BARRACKS:
        case ABILITY_ID::BUILD_FACTORY:
        case ABILITY_ID::BUILD_STARPORT:
        case ABILITY_ID::BUILD_REFINERY:
        case ABILITY_ID::BUILD_COMMANDCENTER:
            return true;
        default:
            return false;
    }
}

bool DoesAbilityRequireObservedConstructionConfirmation(const ABILITY_ID AbilityIdValue)
{
    switch (AbilityIdValue)
    {
        case ABILITY_ID::BUILD_SUPPLYDEPOT:
        case ABILITY_ID::BUILD_BARRACKS:
        case ABILITY_ID::BUILD_FACTORY:
        case ABILITY_ID::BUILD_STARPORT:
        case ABILITY_ID::BUILD_REFINERY:
        case ABILITY_ID::BUILD_COMMANDCENTER:
        case ABILITY_ID::BUILD_BUNKER:
        case ABILITY_ID::BUILD_ENGINEERINGBAY:
            return true;
        default:
            return false;
    }
}

Tag GetCommittedRefineryTag(const ObservationInterface& ObservationValue, const Unit& WorkerUnitValue)
{
    if (WorkerUnitValue.orders.empty())
    {
        return NullTag;
    }

    const UnitOrder& WorkerOrderValue = WorkerUnitValue.orders.front();
    if (!IsHarvestGatherAbility(WorkerOrderValue.ability_id) && !IsHarvestReturnAbility(WorkerOrderValue.ability_id))
    {
        return NullTag;
    }

    if (WorkerOrderValue.target_unit_tag == NullTag)
    {
        return NullTag;
    }

    const Unit* TargetUnitValue = ObservationValue.GetUnit(WorkerOrderValue.target_unit_tag);
    if (TargetUnitValue == nullptr || !IsRefineryUnitType(TargetUnitValue->unit_type.ToType()))
    {
        return NullTag;
    }

    return TargetUnitValue->tag;
}

bool IsWorkerCommittedToRefinery(const ObservationInterface& ObservationValue, const Unit& WorkerUnitValue)
{
    if (IsCarryingVespene(WorkerUnitValue))
    {
        return true;
    }

    return GetCommittedRefineryTag(ObservationValue, WorkerUnitValue) != NullTag;
}

bool IsWorkerCommittedToMinerals(const ObservationInterface& ObservationValue, const Unit& WorkerUnitValue)
{
    if (IsCarryingMinerals(WorkerUnitValue))
    {
        return true;
    }

    if (WorkerUnitValue.orders.empty())
    {
        return false;
    }

    const UnitOrder& WorkerOrderValue = WorkerUnitValue.orders.front();
    if (!IsHarvestGatherAbility(WorkerOrderValue.ability_id) && !IsHarvestReturnAbility(WorkerOrderValue.ability_id))
    {
        return false;
    }

    if (WorkerOrderValue.target_unit_tag == NullTag)
    {
        return false;
    }

    const Unit* TargetUnitValue = ObservationValue.GetUnit(WorkerOrderValue.target_unit_tag);
    if (TargetUnitValue == nullptr)
    {
        return false;
    }

    const IsMineralPatch MineralPatchFilterValue;
    return MineralPatchFilterValue(*TargetUnitValue);
}

bool DoesUnitMatchWallSlot(const Unit& SelfUnitValue, const FBuildPlacementSlot& BuildPlacementSlotValue)
{
    if (!SelfUnitValue.is_building || SelfUnitValue.is_flying)
    {
        return false;
    }

    switch (BuildPlacementSlotValue.SlotId.SlotType)
    {
        case EBuildPlacementSlotType::MainRampDepotLeft:
        case EBuildPlacementSlotType::MainRampDepotRight:
            return IsWallDepotUnitType(SelfUnitValue.unit_type.ToType());
        case EBuildPlacementSlotType::MainRampBarracksWithAddon:
            return SelfUnitValue.unit_type.ToType() == UNIT_TYPEID::TERRAN_BARRACKS;
        case EBuildPlacementSlotType::Unknown:
        case EBuildPlacementSlotType::NaturalApproachDepot:
        case EBuildPlacementSlotType::MainSupportDepot:
        case EBuildPlacementSlotType::MainBarracksWithAddon:
        case EBuildPlacementSlotType::MainFactoryWithAddon:
        case EBuildPlacementSlotType::MainStarportWithAddon:
        case EBuildPlacementSlotType::MainProductionWithAddon:
        case EBuildPlacementSlotType::MainSupportStructure:
        default:
            return false;
    }
}

const Unit* FindWallStructureForSlot(const Units& SelfUnitsValue, const FBuildPlacementSlot& BuildPlacementSlotValue)
{
    const Unit* BestUnitValue = nullptr;
    float BestDistanceSquaredValue = std::numeric_limits<float>::max();

    for (const Unit* SelfUnitValue : SelfUnitsValue)
    {
        if (SelfUnitValue == nullptr || !DoesUnitMatchWallSlot(*SelfUnitValue, BuildPlacementSlotValue))
        {
            continue;
        }

        const float DistanceSquaredValue =
            DistanceSquared2D(Point2D(SelfUnitValue->pos), BuildPlacementSlotValue.BuildPoint);
        if (DistanceSquaredValue > WallStructureMatchRadiusSquaredValue || DistanceSquaredValue >= BestDistanceSquaredValue)
        {
            continue;
        }

        BestDistanceSquaredValue = DistanceSquaredValue;
        BestUnitValue = SelfUnitValue;
    }

    return BestUnitValue;
}

const char* GetWallSlotOccupancyLabel(const Unit* OccupyingUnitValue)
{
    if (OccupyingUnitValue == nullptr)
    {
        return "Empty";
    }

    return OccupyingUnitValue->build_progress >= 1.0f ? "Filled" : "InProgress";
}

const Unit* SelectWorkerForRefinery(const ObservationInterface& ObservationValue, const FAgentState& AgentStateValue,
                                    const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                    const FIntentBuffer& IntentBufferValue, const Unit& RefineryUnitValue,
                                    const std::unordered_set<Tag>& ReservedWorkerTagsValue)
{
    const Unit* BestIdleWorkerValue = nullptr;
    float BestIdleWorkerDistanceSquaredValue = std::numeric_limits<float>::max();
    const Unit* BestMineralWorkerValue = nullptr;
    float BestMineralWorkerDistanceSquaredValue = std::numeric_limits<float>::max();

    for (const Unit* WorkerUnitValue : AgentStateValue.UnitContainer.ControlledUnits)
    {
        if (WorkerUnitValue == nullptr || WorkerUnitValue->unit_type.ToType() != UNIT_TYPEID::TERRAN_SCV ||
            WorkerUnitValue->build_progress < 1.0f ||
            ReservedWorkerTagsValue.find(WorkerUnitValue->tag) != ReservedWorkerTagsValue.end() ||
            IntentBufferValue.HasIntentForActor(WorkerUnitValue->tag) ||
            HasActiveSchedulerOrderForActorTag(CommandAuthoritySchedulingStateValue, WorkerUnitValue->tag) ||
            IsWorkerCommittedToConstruction(*WorkerUnitValue) ||
            IsWorkerCommittedToRefinery(ObservationValue, *WorkerUnitValue))
        {
            continue;
        }

        const float WorkerDistanceSquaredValue =
            DistanceSquared2D(Point2D(WorkerUnitValue->pos), Point2D(RefineryUnitValue.pos));
        if (WorkerUnitValue->orders.empty())
        {
            if (BestIdleWorkerValue == nullptr ||
                WorkerDistanceSquaredValue < BestIdleWorkerDistanceSquaredValue)
            {
                BestIdleWorkerValue = WorkerUnitValue;
                BestIdleWorkerDistanceSquaredValue = WorkerDistanceSquaredValue;
            }
            continue;
        }

        if (!IsWorkerCommittedToMinerals(ObservationValue, *WorkerUnitValue))
        {
            continue;
        }

        if (BestMineralWorkerValue == nullptr ||
            WorkerDistanceSquaredValue < BestMineralWorkerDistanceSquaredValue)
        {
            BestMineralWorkerValue = WorkerUnitValue;
            BestMineralWorkerDistanceSquaredValue = WorkerDistanceSquaredValue;
        }
    }

    return BestIdleWorkerValue != nullptr ? BestIdleWorkerValue : BestMineralWorkerValue;
}

const Unit* SelectWorkerForGasRelief(const ObservationInterface& ObservationValue, const FAgentState& AgentStateValue,
                                     const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                     const FIntentBuffer& IntentBufferValue, const Unit& RefineryUnitValue,
                                     const std::unordered_set<Tag>& ReservedWorkerTagsValue)
{
    const Unit* BestRefineryWorkerValue = nullptr;
    float BestRefineryWorkerDistanceSquaredValue = std::numeric_limits<float>::max();

    for (const Unit* WorkerUnitValue : AgentStateValue.UnitContainer.ControlledUnits)
    {
        if (WorkerUnitValue == nullptr || WorkerUnitValue->unit_type.ToType() != UNIT_TYPEID::TERRAN_SCV ||
            WorkerUnitValue->build_progress < 1.0f ||
            ReservedWorkerTagsValue.find(WorkerUnitValue->tag) != ReservedWorkerTagsValue.end() ||
            IntentBufferValue.HasIntentForActor(WorkerUnitValue->tag) ||
            HasActiveSchedulerOrderForActorTag(CommandAuthoritySchedulingStateValue, WorkerUnitValue->tag) ||
            IsWorkerCommittedToConstruction(*WorkerUnitValue) ||
            GetCommittedRefineryTag(ObservationValue, *WorkerUnitValue) != RefineryUnitValue.tag)
        {
            continue;
        }

        const float WorkerDistanceSquaredValue =
            DistanceSquared2D(Point2D(WorkerUnitValue->pos), Point2D(RefineryUnitValue.pos));
        if (BestRefineryWorkerValue == nullptr ||
            WorkerDistanceSquaredValue < BestRefineryWorkerDistanceSquaredValue)
        {
            BestRefineryWorkerValue = WorkerUnitValue;
            BestRefineryWorkerDistanceSquaredValue = WorkerDistanceSquaredValue;
        }
    }

    return BestRefineryWorkerValue;
}

int GetCommittedHarvesterCountForRefinery(const ObservationInterface& ObservationValue,
                                          const FAgentState& AgentStateValue,
                                          const Tag RefineryTagValue)
{
    int CommittedHarvesterCountValue = 0;

    for (const Unit* WorkerUnitValue : AgentStateValue.UnitContainer.ControlledUnits)
    {
        if (WorkerUnitValue == nullptr || WorkerUnitValue->unit_type.ToType() != UNIT_TYPEID::TERRAN_SCV ||
            WorkerUnitValue->build_progress < 1.0f)
        {
            continue;
        }

        if (GetCommittedRefineryTag(ObservationValue, *WorkerUnitValue) != RefineryTagValue)
        {
            continue;
        }

        ++CommittedHarvesterCountValue;
    }

    return CommittedHarvesterCountValue;
}

int GetPlannedHarvesterDeltaForRefinery(const std::unordered_map<Tag, int>& PlannedFillCountsByRefineryTagValue,
                                        const std::unordered_map<Tag, int>& PlannedReliefCountsByRefineryTagValue,
                                        const Tag RefineryTagValue)
{
    int PlannedHarvesterDeltaValue = 0;

    const std::unordered_map<Tag, int>::const_iterator PlannedFillIteratorValue =
        PlannedFillCountsByRefineryTagValue.find(RefineryTagValue);
    if (PlannedFillIteratorValue != PlannedFillCountsByRefineryTagValue.end())
    {
        PlannedHarvesterDeltaValue += PlannedFillIteratorValue->second;
    }

    const std::unordered_map<Tag, int>::const_iterator PlannedReliefIteratorValue =
        PlannedReliefCountsByRefineryTagValue.find(RefineryTagValue);
    if (PlannedReliefIteratorValue != PlannedReliefCountsByRefineryTagValue.end())
    {
        PlannedHarvesterDeltaValue -= PlannedReliefIteratorValue->second;
    }

    return PlannedHarvesterDeltaValue;
}

const Unit* FindNearestReadyTownHallUnit(const Units& ReadyTownHallUnitsValue, const Point2D& OriginPointValue)
{
    const Unit* BestTownHallUnitValue = nullptr;
    float BestDistanceSquaredValue = std::numeric_limits<float>::max();

    for (const Unit* TownHallUnitValue : ReadyTownHallUnitsValue)
    {
        if (TownHallUnitValue == nullptr || TownHallUnitValue->build_progress < 1.0f)
        {
            continue;
        }

        const float DistanceSquaredValue =
            DistanceSquared2D(Point2D(TownHallUnitValue->pos), OriginPointValue);
        if (BestTownHallUnitValue != nullptr && DistanceSquaredValue >= BestDistanceSquaredValue)
        {
            continue;
        }

        BestTownHallUnitValue = TownHallUnitValue;
        BestDistanceSquaredValue = DistanceSquaredValue;
    }

    return BestTownHallUnitValue;
}

int GetPlannedHarvesterDeltaForTownHall(const std::unordered_map<Tag, int>& PlannedInboundCountsByTownHallTagValue,
                                        const std::unordered_map<Tag, int>& PlannedOutboundCountsByTownHallTagValue,
                                        const Tag TownHallTagValue)
{
    int PlannedHarvesterDeltaValue = 0;

    const std::unordered_map<Tag, int>::const_iterator PlannedInboundIteratorValue =
        PlannedInboundCountsByTownHallTagValue.find(TownHallTagValue);
    if (PlannedInboundIteratorValue != PlannedInboundCountsByTownHallTagValue.end())
    {
        PlannedHarvesterDeltaValue += PlannedInboundIteratorValue->second;
    }

    const std::unordered_map<Tag, int>::const_iterator PlannedOutboundIteratorValue =
        PlannedOutboundCountsByTownHallTagValue.find(TownHallTagValue);
    if (PlannedOutboundIteratorValue != PlannedOutboundCountsByTownHallTagValue.end())
    {
        PlannedHarvesterDeltaValue -= PlannedOutboundIteratorValue->second;
    }

    return PlannedHarvesterDeltaValue;
}

const Unit* FindMineralTownHallForWorker(const ObservationInterface& ObservationValue, const Unit& WorkerUnitValue,
                                         const Units& ReadyTownHallUnitsValue)
{
    if (!IsWorkerCommittedToMinerals(ObservationValue, WorkerUnitValue))
    {
        return nullptr;
    }

    return FindNearestReadyTownHallUnit(ReadyTownHallUnitsValue, Point2D(WorkerUnitValue.pos));
}

const Unit* SelectWorkerForMineralRebalance(
    const ObservationInterface& ObservationValue, const FAgentState& AgentStateValue,
    const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
    const FIntentBuffer& IntentBufferValue, const Unit& ReceiverTownHallUnitValue,
    const Units& ReadyTownHallUnitsValue, const std::unordered_set<Tag>& ReservedWorkerTagsValue,
    const std::unordered_map<Tag, int>& PlannedInboundCountsByTownHallTagValue,
    const std::unordered_map<Tag, int>& PlannedOutboundCountsByTownHallTagValue)
{
    const Unit* BestWorkerUnitValue = nullptr;
    float BestDistanceSquaredValue = std::numeric_limits<float>::max();
    bool BestWorkerIsCarryingMineralsValue = true;
    int BestSourceHarvesterSurplusValue = 0;

    for (const Unit* WorkerUnitValue : AgentStateValue.UnitContainer.ControlledUnits)
    {
        if (WorkerUnitValue == nullptr || WorkerUnitValue->unit_type.ToType() != UNIT_TYPEID::TERRAN_SCV ||
            WorkerUnitValue->build_progress < 1.0f ||
            ReservedWorkerTagsValue.find(WorkerUnitValue->tag) != ReservedWorkerTagsValue.end() ||
            IntentBufferValue.HasIntentForActor(WorkerUnitValue->tag) ||
            HasActiveSchedulerOrderForActorTag(CommandAuthoritySchedulingStateValue, WorkerUnitValue->tag) ||
            IsWorkerCommittedToConstruction(*WorkerUnitValue) ||
            IsWorkerCommittedToRefinery(ObservationValue, *WorkerUnitValue) ||
            !IsWorkerCommittedToMinerals(ObservationValue, *WorkerUnitValue))
        {
            continue;
        }

        const Unit* SourceTownHallUnitValue =
            FindMineralTownHallForWorker(ObservationValue, *WorkerUnitValue, ReadyTownHallUnitsValue);
        if (SourceTownHallUnitValue == nullptr || SourceTownHallUnitValue->tag == ReceiverTownHallUnitValue.tag)
        {
            continue;
        }

        const int EffectiveAssignedHarvesterCountValue =
            std::max(SourceTownHallUnitValue->assigned_harvesters, 0) +
            GetPlannedHarvesterDeltaForTownHall(PlannedInboundCountsByTownHallTagValue,
                                                PlannedOutboundCountsByTownHallTagValue,
                                                SourceTownHallUnitValue->tag);
        const int SourceHarvesterSurplusValue =
            EffectiveAssignedHarvesterCountValue - SourceTownHallUnitValue->ideal_harvesters;
        if (SourceHarvesterSurplusValue <= 0)
        {
            continue;
        }

        const bool WorkerIsCarryingMineralsValue = IsCarryingMinerals(*WorkerUnitValue);
        const float DistanceSquaredValue =
            DistanceSquared2D(Point2D(WorkerUnitValue->pos), Point2D(ReceiverTownHallUnitValue.pos));
        const bool ShouldReplaceBestWorkerValue =
            BestWorkerUnitValue == nullptr ||
            SourceHarvesterSurplusValue > BestSourceHarvesterSurplusValue ||
            (SourceHarvesterSurplusValue == BestSourceHarvesterSurplusValue &&
             ((BestWorkerIsCarryingMineralsValue && !WorkerIsCarryingMineralsValue) ||
              (BestWorkerIsCarryingMineralsValue == WorkerIsCarryingMineralsValue &&
               DistanceSquaredValue < BestDistanceSquaredValue)));
        if (!ShouldReplaceBestWorkerValue)
        {
            continue;
        }

        BestWorkerUnitValue = WorkerUnitValue;
        BestDistanceSquaredValue = DistanceSquaredValue;
        BestWorkerIsCarryingMineralsValue = WorkerIsCarryingMineralsValue;
        BestSourceHarvesterSurplusValue = SourceHarvesterSurplusValue;
    }

    return BestWorkerUnitValue;
}

}  // namespace

void TerranAgent::OnGameStart()
{
    CurrentStep = 0;
    ObservationPtr = Observation();
    if (!ObservationPtr)
    {
        SCLOG(LoggingVerbosity::error, "ERROR in TerranAgent::OnGameStart() - Observation() is null");
        return;
    }

    ExpansionLocations.clear();
    QueryInterface* QueryValue = Query();
    if (QueryValue)
    {
        const std::vector<Point3D> ExpansionLocationValues =
            search::CalculateExpansionLocations(ObservationPtr, QueryValue);
        ExpansionLocations.reserve(ExpansionLocationValues.size());
        for (const Point3D& ExpansionLocationValue : ExpansionLocationValues)
        {
            ExpansionLocations.push_back(Point2D(ExpansionLocationValue.x, ExpansionLocationValue.y));
        }
    }

    // Initialize per-map static layout data from compiled map descriptors
    {
        const GameInfo& GameInfoValue = ObservationPtr->GetGameInfo();
        const std::string NormalizedMapNameValue =
            FMapLayoutDictionary::NormalizeMapName(GameInfoValue.map_name);
        MapDescriptorPtrValue = FMapLayoutDictionary::TryGetMapByName(NormalizedMapNameValue);
        if (MapDescriptorPtrValue != nullptr)
        {
            const Point2D StartLocationValue = Point2D(ObservationPtr->GetStartLocation());
            OwnSpawnLayoutPtrValue =
                FMapLayoutDictionary::TryGetSpawnLayout(*MapDescriptorPtrValue, StartLocationValue);
        }
    }

    GameStateDescriptor.Reset();
    EconomyDomainState.Reset();
    ExecutionTelemetry.Reset();
    ProductionRallyStates.clear();
    CurrentWallGateState = EWallGateState::Unavailable;
    LastArmyExecutionOrderCount = 0U;
    LastProductionRallyApplyCount = 0U;
    RecentProductionRallyCounterWindowStartStep = 0U;
    RecentProductionRallyApplyCount = 0U;
    LastBlockerReliefMoveCount = 0U;
    LastUnitExecutionReplanCount = 0U;
    LastActiveIndexedExecutionOrderCount = 0U;
    LastTerminalCompactionStep = 0U;
    PendingProductionRallyIntents.clear();

    const FFrameContext Frame = FFrameContext::Create(ObservationPtr, Query(), CurrentStep);
    UpdateAgentState(Frame);
    InitializeRampWallDescriptor(Frame);
    InitializeMainBaseLayoutDescriptor(Frame);
    RebuildObservedGameStateDescriptor(Frame);
    RebuildEnemyObservationDescriptor(Frame);
    RebuildForecastState();
    RebuildExecutionPressureDescriptor(Frame);
    UpdateStrategicAndPlanningState();
    PrintAgentState();
}

void TerranAgent::OnStep()
{
    ++CurrentStep;
    const FSteadyTimePoint StepStartTimeValue = FSteadyClock::now();

    ObservationPtr = Observation();
    if (!ObservationPtr)
    {
        SCLOG(LoggingVerbosity::error, "ERROR in TerranAgent::OnStep() - Observation() is null");
        return;
    }

    const FFrameContext Frame = FFrameContext::Create(ObservationPtr, Query(), CurrentStep);

    FSteadyTimePoint PhaseStartTimeValue = FSteadyClock::now();
    UpdateAgentState(Frame);
    FSteadyTimePoint PhaseEndTimeValue = FSteadyClock::now();
    LastAgentStateUpdateMicroseconds = GetElapsedMicroseconds(PhaseStartTimeValue, PhaseEndTimeValue);

    PhaseStartTimeValue = FSteadyClock::now();
    UpdateDispatchedSchedulerOrders(Frame);
    PhaseEndTimeValue = FSteadyClock::now();
    LastDispatchMaintenanceMicroseconds = GetElapsedMicroseconds(PhaseStartTimeValue, PhaseEndTimeValue);

    PhaseStartTimeValue = FSteadyClock::now();
    RebuildObservedGameStateDescriptor(Frame);
    RebuildEnemyObservationDescriptor(Frame);
    RebuildForecastState();
    RebuildExecutionPressureDescriptor(Frame);
    UpdateStrategicAndPlanningState();
    PhaseEndTimeValue = FSteadyClock::now();
    LastDescriptorRebuildMicroseconds = GetElapsedMicroseconds(PhaseStartTimeValue, PhaseEndTimeValue);

    IntentBuffer.Reset();
    PendingProductionRallyIntents.clear();
    ProduceSchedulerIntents(Frame);
    ProduceProductionRallyIntents();
    ProduceWallGateIntents(Frame);
    ProduceWorkerHarvestIntents(Frame);
    ProduceRecoveryIntents(Frame);
    UpdateExecutionTelemetry(Frame);

    PhaseStartTimeValue = FSteadyClock::now();
    ResolvedIntents = IntentArbiter.Resolve(Frame, AgentState.UnitContainer, IntentBuffer);
    PhaseEndTimeValue = FSteadyClock::now();
    LastIntentResolutionMicroseconds = GetElapsedMicroseconds(PhaseStartTimeValue, PhaseEndTimeValue);

    PhaseStartTimeValue = FSteadyClock::now();
    ExecuteResolvedIntents(Frame, ResolvedIntents);
    ExecuteProductionRallyIntents();
    ExecuteOrbitalAbilities(Frame);
    PhaseEndTimeValue = FSteadyClock::now();
    LastIntentExecutionMicroseconds = GetElapsedMicroseconds(PhaseStartTimeValue, PhaseEndTimeValue);

    PhaseStartTimeValue = FSteadyClock::now();
    CaptureNewlyDispatchedSchedulerOrders(Frame);
    PhaseEndTimeValue = FSteadyClock::now();
    LastDispatchCaptureMicroseconds = GetElapsedMicroseconds(PhaseStartTimeValue, PhaseEndTimeValue);
    LastStepMicroseconds = GetElapsedMicroseconds(StepStartTimeValue, PhaseEndTimeValue);

    if (CurrentStep % 120 == 0)
    {
        PrintAgentState();
    }
}

void TerranAgent::OnGameEnd()
{
    sc2::renderer::Shutdown();
}

void TerranAgent::OnUnitIdle(const Unit* UnitPtr)
{
    if (!UnitPtr)
    {
        return;
    }

    if (UnitPtr->unit_type.ToType() == UNIT_TYPEID::TERRAN_SCV)
    {
        PendingRecoveryWorkers.insert(UnitPtr->tag);
    }
}

void TerranAgent::OnUnitCreated(const Unit* UnitPtr)
{
    (void)UnitPtr;
}

void TerranAgent::OnBuildingConstructionComplete(const Unit* UnitPtr)
{
    if (UnitPtr == nullptr || !IsProductionRallyStructureType(UnitPtr->unit_type.ToType()))
    {
        return;
    }

    FProductionRallyState& ProductionRallyStateValue = ProductionRallyStates[UnitPtr->tag];
    ProductionRallyStateValue.Reset();
}

void TerranAgent::UpdateAgentState(const FFrameContext& Frame)
{
    if (!Frame.Observation)
    {
        SCLOG(LoggingVerbosity::error, "ERROR in TerranAgent::UpdateAgentState() - observation is null");
        return;
    }

    NeutralUnits = Frame.Observation->GetUnits(Unit::Alliance::Neutral);
    AgentState.Update(Frame);
}

void TerranAgent::InitializeRampWallDescriptor(const FFrameContext& Frame)
{
    GameStateDescriptor.RampWallDescriptor.Reset();
    GameStateDescriptor.MainBaseLayoutDescriptor.Reset();
    if (BuildPlacementService == nullptr || ObservationPtr == nullptr)
    {
        return;
    }

    FBuildPlacementContext BuildPlacementContextValue = CreateBuildPlacementContext();
    BuildPlacementContextValue.RampWallDescriptor.Reset();
    GameStateDescriptor.RampWallDescriptor =
        BuildPlacementService->GetRampWallDescriptor(Frame, BuildPlacementContextValue);

    if (!GameStateDescriptor.RampWallDescriptor.bIsValid)
    {
        ExecutionTelemetry.RecordWallDescriptorInvalid(CurrentStep, Frame.GameLoop);
    }
}

void TerranAgent::InitializeMainBaseLayoutDescriptor(const FFrameContext& Frame)
{
    GameStateDescriptor.MainBaseLayoutDescriptor.Reset();
    if (BuildPlacementService == nullptr || ObservationPtr == nullptr)
    {
        return;
    }

    FBuildPlacementContext BuildPlacementContextValue = CreateBuildPlacementContext();
    BuildPlacementContextValue.MainBaseLayoutDescriptor.Reset();
    GameStateDescriptor.MainBaseLayoutDescriptor =
        BuildPlacementService->GetMainBaseLayoutDescriptor(Frame, BuildPlacementContextValue);
}

void TerranAgent::RebuildObservedGameStateDescriptor(const FFrameContext& Frame)
{
    (void)Frame;

    if (GameStateDescriptorBuilder)
    {
        GameStateDescriptorBuilder->RebuildGameStateDescriptor(CurrentStep, Frame.GameLoop, AgentState,
                                                               GameStateDescriptor);
    }
    else
    {
        GameStateDescriptor.CurrentStep = CurrentStep;
        GameStateDescriptor.CurrentGameLoop = Frame.GameLoop;
    }
}

void TerranAgent::RebuildEnemyObservationDescriptor(const FFrameContext& Frame)
{
    if (EnemyObservationBuilder == nullptr || Frame.Observation == nullptr)
    {
        return;
    }

    EnemyObservationBuilder->RebuildEnemyObservation(*Frame.Observation, Frame.GameLoop,
                                                      GameStateDescriptor.EnemyObservation);
}

void TerranAgent::RebuildForecastState()
{
    DefaultForecastStateBuilder.RebuildForecastState(AgentState, EconomyDomainState, GameStateDescriptor);
}

void TerranAgent::RebuildExecutionPressureDescriptor(const FFrameContext& Frame)
{
    FExecutionPressureDescriptor& ExecutionPressureDescriptorValue = GameStateDescriptor.ExecutionPressure;
    ExecutionPressureDescriptorValue.Reset();

    const bool IsSupplyBlockedValue = ObservationPtr != nullptr && ObservationPtr->GetFoodCap() < 200U &&
                                      GameStateDescriptor.BuildPlanning.AvailableSupply == 0U;
    ExecutionPressureDescriptorValue.SupplyBlockState = GetExecutionConditionState(IsSupplyBlockedValue);
    if (IsSupplyBlockedValue)
    {
        ExecutionPressureDescriptorValue.SupplyBlockDurationGameLoops =
            ExecutionTelemetry.SupplyBlockState == EExecutionConditionState::Active
                ? ExecutionTelemetry.GetCurrentSupplyBlockDurationGameLoops(Frame.GameLoop)
                : 0U;
    }

    const bool IsBankingMineralsValue = GameStateDescriptor.BuildPlanning.AvailableMinerals >= 400U;
    ExecutionPressureDescriptorValue.MineralBankState = GetExecutionConditionState(IsBankingMineralsValue);
    ExecutionPressureDescriptorValue.CurrentMineralBankAmount = GameStateDescriptor.BuildPlanning.AvailableMinerals;
    if (IsBankingMineralsValue)
    {
        ExecutionPressureDescriptorValue.MineralBankDurationGameLoops =
            ExecutionTelemetry.MineralBankState == EExecutionConditionState::Active
                ? ExecutionTelemetry.GetCurrentMineralBankDurationGameLoops(Frame.GameLoop)
                : 0U;
    }

    ExecutionPressureDescriptorValue.RecentIdleProductionConflictCount =
        ExecutionTelemetry.RecentIdleProductionConflictCount;
    ExecutionPressureDescriptorValue.RecentSchedulerOrderDeferralCount =
        ExecutionTelemetry.RecentSchedulerOrderDeferralCount;

    for (const FExecutionEventRecord& ExecutionEventRecordValue : ExecutionTelemetry.RecentEvents)
    {
        if (ExecutionEventRecordValue.EventType != EAgentExecutionEventType::SchedulerOrderDeferred)
        {
            continue;
        }

        switch (ExecutionEventRecordValue.DeferralReason)
        {
            case ECommandOrderDeferralReason::NoProducer:
                ++ExecutionPressureDescriptorValue.RecentNoProducerDeferralCount;
                break;
            case ECommandOrderDeferralReason::ProducerBusy:
                ++ExecutionPressureDescriptorValue.RecentProducerBusyDeferralCount;
                break;
            case ECommandOrderDeferralReason::InsufficientResources:
                ++ExecutionPressureDescriptorValue.RecentInsufficientResourcesDeferralCount;
                break;
            case ECommandOrderDeferralReason::NoValidPlacement:
            case ECommandOrderDeferralReason::ReservedSlotOccupied:
            case ECommandOrderDeferralReason::ReservedSlotInvalidated:
                ++ExecutionPressureDescriptorValue.RecentPlacementDeferralCount;
                break;
            default:
                break;
        }
    }

    if (ObservationPtr == nullptr)
    {
        return;
    }

    const Units SelfUnitsValue = ObservationPtr->GetUnits(Unit::Alliance::Self);
    for (const Unit* SelfUnitValue : SelfUnitsValue)
    {
        if (SelfUnitValue == nullptr || SelfUnitValue->is_flying)
        {
            continue;
        }

        const UNIT_TYPEID UnitTypeIdValue = SelfUnitValue->unit_type.ToType();
        if (IsTownHallStructureType(UnitTypeIdValue))
        {
            if (IsIdleStructureWithoutScheduledWork(*SelfUnitValue, GameStateDescriptor.CommandAuthoritySchedulingState))
            {
                ++ExecutionPressureDescriptorValue.IdleTownHallCount;
            }
            continue;
        }

        switch (UnitTypeIdValue)
        {
            case UNIT_TYPEID::TERRAN_BARRACKS:
                if (IsIdleStructureWithoutScheduledWork(*SelfUnitValue,
                                                        GameStateDescriptor.CommandAuthoritySchedulingState))
                {
                    ++ExecutionPressureDescriptorValue.IdleBarracksCount;
                }
                break;
            case UNIT_TYPEID::TERRAN_FACTORY:
                if (IsIdleStructureWithoutScheduledWork(*SelfUnitValue,
                                                        GameStateDescriptor.CommandAuthoritySchedulingState))
                {
                    ++ExecutionPressureDescriptorValue.IdleFactoryCount;
                }
                break;
            case UNIT_TYPEID::TERRAN_STARPORT:
                if (IsIdleStructureWithoutScheduledWork(*SelfUnitValue,
                                                        GameStateDescriptor.CommandAuthoritySchedulingState))
                {
                    ++ExecutionPressureDescriptorValue.IdleStarportCount;
                }
                break;
            default:
                break;
        }
    }
}

void TerranAgent::UpdateStrategicAndPlanningState()
{
    if (StrategicDirector)
    {
        StrategicDirector->UpdateGameStateDescriptor(GameStateDescriptor);
    }
    if (BuildPlanner)
    {
        BuildPlanner->ProduceBuildPlan(GameStateDescriptor, GameStateDescriptor.BuildPlanning);
    }
    if (ArmyPlanner)
    {
        ArmyPlanner->ProduceArmyPlan(GameStateDescriptor, GameStateDescriptor.ArmyState);
    }

    UpdateRallyAnchor();
}

void TerranAgent::UpdateRallyAnchor()
{
    if (!ObservationPtr)
    {
        return;
    }

    const FBuildPlacementContext BuildPlacementContextValue = CreateBuildPlacementContext();
    const Point2D BaseLocationValue = BuildPlacementContextValue.BaseLocation;
    ArmyAssemblyPoint = BuildPlacementService
                            ? BuildPlacementService->GetArmyAssemblyPoint(GameStateDescriptor, BuildPlacementContextValue)
                            : BaseLocationValue;
    ProductionRallyPoint = BuildPlacementService
                               ? BuildPlacementService->GetProductionRallyPoint(GameStateDescriptor,
                                                                               BuildPlacementContextValue)
                               : ArmyAssemblyPoint;
}

FBuildPlacementContext TerranAgent::CreateBuildPlacementContext() const
{
    FBuildPlacementContext BuildPlacementContextValue;
    if (!ObservationPtr)
    {
        return BuildPlacementContextValue;
    }

    const GameInfo& GameInfoValue = ObservationPtr->GetGameInfo();
    BuildPlacementContextValue.MapName = GameInfoValue.map_name;
    BuildPlacementContextValue.BaseLocation = Point2D(ObservationPtr->GetStartLocation());
    BuildPlacementContextValue.PlayableMin = GameInfoValue.playable_min;
    BuildPlacementContextValue.PlayableMax = GameInfoValue.playable_max;

    float BestDistanceSquaredValue = std::numeric_limits<float>::max();
    for (const Point2D& ExpansionLocationValue : ExpansionLocations)
    {
        const float DistanceSquaredValue =
            DistanceSquared2D(BuildPlacementContextValue.BaseLocation, ExpansionLocationValue);
        if (DistanceSquaredValue < 16.0f || DistanceSquaredValue >= BestDistanceSquaredValue)
        {
            continue;
        }

        BestDistanceSquaredValue = DistanceSquaredValue;
        BuildPlacementContextValue.NaturalLocation = ExpansionLocationValue;
    }

    BuildPlacementContextValue.RampWallDescriptor = GameStateDescriptor.RampWallDescriptor;
    BuildPlacementContextValue.MainBaseLayoutDescriptor = GameStateDescriptor.MainBaseLayoutDescriptor;

    return BuildPlacementContextValue;
}

void TerranAgent::PrintAgentState()
{
    AgentState.PrintStatus();

    std::cout << "Game Descriptor:\n";
    std::cout << "Step: " << GameStateDescriptor.CurrentStep
              << " | GameLoop: " << GameStateDescriptor.CurrentGameLoop << "\n";
    std::cout << "Plan: " << ToString(GameStateDescriptor.MacroState.ActiveGamePlan)
              << " | Phase: " << ToString(GameStateDescriptor.MacroState.ActiveMacroPhase)
              << " | Bases: " << GameStateDescriptor.MacroState.ActiveBaseCount << "/"
              << GameStateDescriptor.MacroState.DesiredBaseCount
              << " | Desired Armies: " << GameStateDescriptor.MacroState.DesiredArmyCount
              << " | Focus: " << ToString(GameStateDescriptor.MacroState.PrimaryProductionFocus) << "\n";
    std::cout << "Build Targets: "
              << "Workers " << GameStateDescriptor.BuildPlanning.DesiredWorkerCount
              << " | Orbitals " << GameStateDescriptor.BuildPlanning.DesiredOrbitalCommandCount
              << " | Refineries " << GameStateDescriptor.BuildPlanning.DesiredRefineryCount
              << " | Depots " << GameStateDescriptor.BuildPlanning.DesiredSupplyDepotCount
              << " | Barracks " << GameStateDescriptor.BuildPlanning.DesiredBarracksCount
              << " | Factory " << GameStateDescriptor.BuildPlanning.DesiredFactoryCount
              << " | Starport " << GameStateDescriptor.BuildPlanning.DesiredStarportCount
              << " | Marines " << GameStateDescriptor.BuildPlanning.DesiredMarineCount
              << " | Needs " << GameStateDescriptor.BuildPlanning.ActiveNeedCount << "\n";
    std::cout << "Economic Ledger: "
              << "MandatoryReserved M" << (GameStateDescriptor.CommitmentLedger.GetReservedMinerals(
                                               ECommandCommitmentClass::MandatoryOpening) +
                                           GameStateDescriptor.CommitmentLedger.GetReservedMinerals(
                                               ECommandCommitmentClass::MandatoryRecovery))
              << "/G" << (GameStateDescriptor.CommitmentLedger.GetReservedVespene(
                              ECommandCommitmentClass::MandatoryOpening) +
                          GameStateDescriptor.CommitmentLedger.GetReservedVespene(
                              ECommandCommitmentClass::MandatoryRecovery))
              << "/S" << (GameStateDescriptor.CommitmentLedger.GetReservedSupply(
                              ECommandCommitmentClass::MandatoryOpening) +
                          GameStateDescriptor.CommitmentLedger.GetReservedSupply(
                              ECommandCommitmentClass::MandatoryRecovery))
              << " | MandatoryCommitted M" << (GameStateDescriptor.CommitmentLedger.GetCommittedMinerals(
                                                   ECommandCommitmentClass::MandatoryOpening) +
                                               GameStateDescriptor.CommitmentLedger.GetCommittedMinerals(
                                                   ECommandCommitmentClass::MandatoryRecovery))
              << "/G" << (GameStateDescriptor.CommitmentLedger.GetCommittedVespene(
                              ECommandCommitmentClass::MandatoryOpening) +
                          GameStateDescriptor.CommitmentLedger.GetCommittedVespene(
                              ECommandCommitmentClass::MandatoryRecovery))
              << "/S" << (GameStateDescriptor.CommitmentLedger.GetCommittedSupply(
                              ECommandCommitmentClass::MandatoryOpening) +
                          GameStateDescriptor.CommitmentLedger.GetCommittedSupply(
                              ECommandCommitmentClass::MandatoryRecovery))
              << " | FlexibleReserved M" << (GameStateDescriptor.CommitmentLedger.GetReservedMinerals(
                                                  ECommandCommitmentClass::FlexibleMacro) +
                                              GameStateDescriptor.CommitmentLedger.GetReservedMinerals(
                                                  ECommandCommitmentClass::Opportunistic))
              << "/G" << (GameStateDescriptor.CommitmentLedger.GetReservedVespene(
                              ECommandCommitmentClass::FlexibleMacro) +
                          GameStateDescriptor.CommitmentLedger.GetReservedVespene(
                              ECommandCommitmentClass::Opportunistic))
              << "/S" << (GameStateDescriptor.CommitmentLedger.GetReservedSupply(
                              ECommandCommitmentClass::FlexibleMacro) +
                          GameStateDescriptor.CommitmentLedger.GetReservedSupply(
                              ECommandCommitmentClass::Opportunistic))
              << " | FlexibleCommitted M" << (GameStateDescriptor.CommitmentLedger.GetCommittedMinerals(
                                                   ECommandCommitmentClass::FlexibleMacro) +
                                               GameStateDescriptor.CommitmentLedger.GetCommittedMinerals(
                                                   ECommandCommitmentClass::Opportunistic))
              << "/G" << (GameStateDescriptor.CommitmentLedger.GetCommittedVespene(
                              ECommandCommitmentClass::FlexibleMacro) +
                          GameStateDescriptor.CommitmentLedger.GetCommittedVespene(
                              ECommandCommitmentClass::Opportunistic))
              << "/S" << (GameStateDescriptor.CommitmentLedger.GetCommittedSupply(
                              ECommandCommitmentClass::FlexibleMacro) +
                          GameStateDescriptor.CommitmentLedger.GetCommittedSupply(
                              ECommandCommitmentClass::Opportunistic))
              << "\n";
    std::cout << "Projected Discretionary: "
              << "Short M" << GameStateDescriptor.CommitmentLedger.GetProjectedDiscretionaryMinerals(
                                  ShortForecastHorizonIndexValue)
              << "/G" << GameStateDescriptor.CommitmentLedger.GetProjectedDiscretionaryVespene(
                                  ShortForecastHorizonIndexValue)
              << "/S" << GameStateDescriptor.CommitmentLedger.GetProjectedDiscretionarySupply(
                                  ShortForecastHorizonIndexValue)
              << " | Medium M" << GameStateDescriptor.CommitmentLedger.GetProjectedDiscretionaryMinerals(
                                       MediumForecastHorizonIndexValue)
              << "/G" << GameStateDescriptor.CommitmentLedger.GetProjectedDiscretionaryVespene(
                                  MediumForecastHorizonIndexValue)
              << "/S" << GameStateDescriptor.CommitmentLedger.GetProjectedDiscretionarySupply(
                                  MediumForecastHorizonIndexValue)
              << " | Long M" << GameStateDescriptor.CommitmentLedger.GetProjectedDiscretionaryMinerals(
                                     LongForecastHorizonIndexValue)
              << "/G" << GameStateDescriptor.CommitmentLedger.GetProjectedDiscretionaryVespene(
                                  LongForecastHorizonIndexValue)
              << "/S" << GameStateDescriptor.CommitmentLedger.GetProjectedDiscretionarySupply(
                                  LongForecastHorizonIndexValue)
              << "\n";
    std::cout << "Step Timing (us): "
              << "Total " << LastStepMicroseconds
              << " | State " << LastAgentStateUpdateMicroseconds
              << " | Descriptor " << LastDescriptorRebuildMicroseconds
              << " | DispatchUpdate " << LastDispatchMaintenanceMicroseconds
              << " | Strategic " << LastSchedulerStrategicProcessingMicroseconds
              << " | Economy " << LastSchedulerEconomyProcessingMicroseconds
              << " | Army " << LastSchedulerArmyProcessingMicroseconds
              << " | Squad " << LastSchedulerSquadProcessingMicroseconds
              << " | UnitExec " << LastSchedulerUnitExecutionProcessingMicroseconds
              << " | Drain " << LastSchedulerDrainMicroseconds
              << " | Resolve " << LastIntentResolutionMicroseconds
              << " | Execute " << LastIntentExecutionMicroseconds
              << " | Capture " << LastDispatchCaptureMicroseconds << "\n";
    PrintGoalList("Immediate Goals", GameStateDescriptor.GoalSet.ImmediateGoals);
    std::cout << std::endl;
    PrintGoalList("Near Goals", GameStateDescriptor.GoalSet.NearTermGoals);
    std::cout << std::endl;
    PrintGoalList("Strategic Goals", GameStateDescriptor.GoalSet.StrategicGoals);
    std::cout << std::endl;
    std::cout << "Army Goals: ";
    if (GameStateDescriptor.ArmyState.ArmyGoals.empty())
    {
        std::cout << "None";
    }
    else
    {
        for (size_t ArmyIndexValue = 0U; ArmyIndexValue < GameStateDescriptor.ArmyState.ArmyGoals.size();
             ++ArmyIndexValue)
        {
            if (ArmyIndexValue > 0U)
            {
                std::cout << ", ";
            }

            std::cout << ToString(GameStateDescriptor.ArmyState.ArmyGoals[ArmyIndexValue]);
        }
    }
    std::cout << std::endl;
    std::cout << "Army Postures: ";
    if (GameStateDescriptor.ArmyState.ArmyPostures.empty())
    {
        std::cout << "None";
    }
    else
    {
        for (size_t ArmyIndexValue = 0U; ArmyIndexValue < GameStateDescriptor.ArmyState.ArmyPostures.size();
             ++ArmyIndexValue)
        {
            if (ArmyIndexValue > 0U)
            {
                std::cout << ", ";
            }

            std::cout << ToString(GameStateDescriptor.ArmyState.ArmyPostures[ArmyIndexValue]);
        }
    }
    std::cout << std::endl;
    std::cout << "Army Mission: ";
    if (GameStateDescriptor.ArmyState.ArmyMissions.empty())
    {
        std::cout << "None";
    }
    else
    {
        const FArmyMissionDescriptor& ArmyMissionDescriptorValue = GameStateDescriptor.ArmyState.ArmyMissions.front();
        std::cout << ToString(ArmyMissionDescriptorValue.MissionType)
                  << " | Goal " << ArmyMissionDescriptorValue.SourceGoalId
                  << " | Objective (" << ArmyMissionDescriptorValue.ObjectivePoint.x
                  << ", " << ArmyMissionDescriptorValue.ObjectivePoint.y << ")"
                  << " | Search " << ArmyMissionDescriptorValue.SearchExpansionOrdinal
                  << " | OrdersThisStep " << LastArmyExecutionOrderCount;
    }
    std::cout << std::endl;
    PrintPriorityTierQueueSummary("StrategicQueues", GameStateDescriptor.CommandAuthoritySchedulingState.StrategicQueues);
    std::cout << std::endl;
    PrintPriorityTierQueueSummary("PlanningQueues", GameStateDescriptor.CommandAuthoritySchedulingState.PlanningQueues);
    std::cout << std::endl;
    PrintPriorityTierQueueSummary("ArmyQueues", GameStateDescriptor.CommandAuthoritySchedulingState.ArmyQueues);
    std::cout << std::endl;
    PrintPriorityTierQueueSummary("SquadQueues", GameStateDescriptor.CommandAuthoritySchedulingState.SquadQueues);
    std::cout << std::endl;
    PrintReadyIntentQueueSummary(GameStateDescriptor.CommandAuthoritySchedulingState.ReadyIntentQueues);
    std::cout << std::endl;
    std::cout << "Hot Active: "
              << "Strategic " << GameStateDescriptor.CommandAuthoritySchedulingState.GetActiveOrderCountForLayer(
                                       ECommandAuthorityLayer::StrategicDirector)
              << " | Planning " << GameStateDescriptor.CommandAuthoritySchedulingState.GetActiveOrderCountForLayer(
                                       ECommandAuthorityLayer::EconomyAndProduction)
              << " | Army " << GameStateDescriptor.CommandAuthoritySchedulingState.GetActiveOrderCountForLayer(
                                  ECommandAuthorityLayer::Army)
              << " | Squad " << GameStateDescriptor.CommandAuthoritySchedulingState.GetActiveOrderCountForLayer(
                                   ECommandAuthorityLayer::Squad)
              << " | UnitExec " << GameStateDescriptor.CommandAuthoritySchedulingState.GetActiveOrderCountForLayer(
                                      ECommandAuthorityLayer::UnitExecution)
              << "\n";
    std::cout << "Mandatory Opening: "
              << "Active " << CountActiveMandatoryOpeningTasks(GameStateDescriptor.CommandAuthoritySchedulingState)
              << " | Blocked " << (CountBlockedMandatoryTasks(
                                       GameStateDescriptor.CommandAuthoritySchedulingState.BlockedStrategicTasks) +
                                   CountBlockedMandatoryTasks(
                                       GameStateDescriptor.CommandAuthoritySchedulingState.BlockedPlanningTasks))
              << "\n";
    PrintMandatorySlotOwners(GameStateDescriptor.CommandAuthoritySchedulingState);
    std::cout << std::endl;
    std::cout << "Blocked Tasks: "
              << "Strategic " << GameStateDescriptor.CommandAuthoritySchedulingState.BlockedStrategicTasks.GetCount()
              << " | Planning " << GameStateDescriptor.CommandAuthoritySchedulingState.BlockedPlanningTasks.GetCount()
              << " | BufferedRecent "
              << GameStateDescriptor.CommandAuthoritySchedulingState.RecentBufferedBlockedTaskCount
              << " | CoalescedRecent "
              << GameStateDescriptor.CommandAuthoritySchedulingState.RecentCoalescedBlockedTaskCount
              << " | DroppedRecent "
              << GameStateDescriptor.CommandAuthoritySchedulingState.RecentDroppedBlockedTaskCount
              << " | ReactivatedRecent "
              << GameStateDescriptor.CommandAuthoritySchedulingState.RecentReactivatedBlockedTaskCount
              << " | MustRunRejectedRecent "
              << GameStateDescriptor.CommandAuthoritySchedulingState.RecentRejectedMustRunBlockedTaskCount
              << " | NoProducer "
              << (GameStateDescriptor.CommandAuthoritySchedulingState.BlockedStrategicTasks.CountRecordsByDeferralReason(
                      ECommandOrderDeferralReason::NoProducer) +
                  GameStateDescriptor.CommandAuthoritySchedulingState.BlockedPlanningTasks.CountRecordsByDeferralReason(
                      ECommandOrderDeferralReason::NoProducer))
              << " | Resources "
              << (GameStateDescriptor.CommandAuthoritySchedulingState.BlockedStrategicTasks.CountRecordsByDeferralReason(
                      ECommandOrderDeferralReason::InsufficientResources) +
                  GameStateDescriptor.CommandAuthoritySchedulingState.BlockedPlanningTasks.CountRecordsByDeferralReason(
                      ECommandOrderDeferralReason::InsufficientResources))
              << " | Placement "
              << (GameStateDescriptor.CommandAuthoritySchedulingState.BlockedStrategicTasks.CountRecordsByDeferralReason(
                      ECommandOrderDeferralReason::NoValidPlacement) +
                  GameStateDescriptor.CommandAuthoritySchedulingState.BlockedPlanningTasks.CountRecordsByDeferralReason(
                      ECommandOrderDeferralReason::NoValidPlacement) +
                  GameStateDescriptor.CommandAuthoritySchedulingState.BlockedStrategicTasks.CountRecordsByDeferralReason(
                      ECommandOrderDeferralReason::ReservedSlotOccupied) +
                  GameStateDescriptor.CommandAuthoritySchedulingState.BlockedPlanningTasks.CountRecordsByDeferralReason(
                      ECommandOrderDeferralReason::ReservedSlotOccupied) +
                  GameStateDescriptor.CommandAuthoritySchedulingState.BlockedStrategicTasks.CountRecordsByDeferralReason(
                      ECommandOrderDeferralReason::ReservedSlotInvalidated) +
                  GameStateDescriptor.CommandAuthoritySchedulingState.BlockedPlanningTasks.CountRecordsByDeferralReason(
                      ECommandOrderDeferralReason::ReservedSlotInvalidated))
              << "\n";
    std::cout << "Unit Execution Admission: "
              << "Dispatched " << GameStateDescriptor.CommandAuthoritySchedulingState.DispatchedOrderIndices.size()
              << " | Cap " << GameStateDescriptor.CommandAuthoritySchedulingState.MaxActiveUnitExecutionOrders
              << " | Rejected " << GameStateDescriptor.CommandAuthoritySchedulingState.RejectedUnitExecutionAdmissionCount
              << " | Superseded "
              << GameStateDescriptor.CommandAuthoritySchedulingState.SupersededUnitExecutionOrderCount << "\n";
    std::cout << "Execution Control: "
              << "Assembly (" << ArmyAssemblyPoint.x << ", " << ArmyAssemblyPoint.y << ")"
              << " | ProductionRally (" << ProductionRallyPoint.x << ", " << ProductionRallyPoint.y << ")"
              << " | RallyAppliesRecent " << RecentProductionRallyApplyCount
              << " | BlockerReliefMoves " << LastBlockerReliefMoveCount
              << " | UnitReplans " << LastUnitExecutionReplanCount
              << " | IndexedExecution " << LastActiveIndexedExecutionOrderCount << "\n";
    PrintWallState();
    const FMainBaseLayoutDescriptor& MainBaseLayoutDescriptorValue = GameStateDescriptor.MainBaseLayoutDescriptor;
    std::cout << "Main Layout: " << (MainBaseLayoutDescriptorValue.bIsValid ? "Valid" : "Invalid");
    if (MainBaseLayoutDescriptorValue.bIsValid)
    {
        std::cout << " | Anchor (" << MainBaseLayoutDescriptorValue.LayoutAnchorPoint.x
                  << ", " << MainBaseLayoutDescriptorValue.LayoutAnchorPoint.y << ")";
        PrintProductionRailSlots(MainBaseLayoutDescriptorValue.ProductionRailWithAddonSlots,
                                 AgentState.UnitContainer.ControlledUnits);
        PrintMainLayoutSlotFamily("Barracks", MainBaseLayoutDescriptorValue.BarracksWithAddonSlots);
        PrintMainLayoutSlotFamily("Factory", MainBaseLayoutDescriptorValue.FactoryWithAddonSlots);
        PrintMainLayoutSlotFamily("Starport", MainBaseLayoutDescriptorValue.StarportWithAddonSlots);
    }
    std::cout << std::endl;
    std::cout << "Execution Telemetry: "
              << "SupplyBlock " << ToString(ExecutionTelemetry.SupplyBlockState)
              << " (" << ExecutionTelemetry.GetCurrentSupplyBlockDurationGameLoops(GameStateDescriptor.CurrentGameLoop)
              << " loops)"
              << " | MineralBank " << ToString(ExecutionTelemetry.MineralBankState)
              << " (" << ExecutionTelemetry.GetCurrentMineralBankDurationGameLoops(GameStateDescriptor.CurrentGameLoop)
              << " loops)"
              << " | ConflictsRecent " << ExecutionTelemetry.RecentActorIntentConflictCount
              << " | IdleProductionRecent " << ExecutionTelemetry.RecentIdleProductionConflictCount
              << " | DeferralsRecent " << ExecutionTelemetry.RecentSchedulerOrderDeferralCount << "\n";
    std::cout << "Recent Execution Events: ";
    if (ExecutionTelemetry.RecentEvents.empty())
    {
        std::cout << "None";
    }
    else
    {
        for (size_t EventIndexValue = 0U; EventIndexValue < ExecutionTelemetry.RecentEvents.size(); ++EventIndexValue)
        {
            const FExecutionEventRecord& ExecutionEventRecordValue =
                ExecutionTelemetry.RecentEvents[EventIndexValue];
            if (EventIndexValue > 0U)
            {
                std::cout << " | ";
            }

            std::cout << ToString(ExecutionEventRecordValue.EventType)
                      << "@GL" << ExecutionEventRecordValue.GameLoop;
            if (ExecutionEventRecordValue.ActorTag != NullTag)
            {
                std::cout << " Actor " << ExecutionEventRecordValue.ActorTag;
            }
            if (ExecutionEventRecordValue.OrderId != 0U)
            {
                std::cout << " Order " << ExecutionEventRecordValue.OrderId;
            }
            if (ExecutionEventRecordValue.PlanStepId != 0U)
            {
                std::cout << " PlanStep " << ExecutionEventRecordValue.PlanStepId;
            }
            if (ExecutionEventRecordValue.AbilityId != ABILITY_ID::INVALID)
            {
                std::cout << " Ability " << static_cast<uint32_t>(ExecutionEventRecordValue.AbilityId);
            }
            if (ExecutionEventRecordValue.UnitTypeId != UNIT_TYPEID::INVALID)
            {
                std::cout << " Unit " << static_cast<uint32_t>(ExecutionEventRecordValue.UnitTypeId);
            }
            if (ExecutionEventRecordValue.DeferralReason != ECommandOrderDeferralReason::None)
            {
                std::cout << " Reason " << ToString(ExecutionEventRecordValue.DeferralReason);
            }
            if (ExecutionEventRecordValue.MetricValue > 0U)
            {
                std::cout << " Metric " << ExecutionEventRecordValue.MetricValue;
            }
        }
    }
    std::cout << std::endl;
}

void TerranAgent::PrintWallState() const
{
    if (ObservationPtr == nullptr)
    {
        return;
    }

    const FRampWallDescriptor& RampWallDescriptorValue = GameStateDescriptor.RampWallDescriptor;
    std::cout << "Wall: "
              << (RampWallDescriptorValue.bIsValid ? "Valid" : "Invalid")
              << " | Gate " << ToString(CurrentWallGateState);
    if (!RampWallDescriptorValue.bIsValid)
    {
        std::cout << std::endl;
        return;
    }

    const Units SelfUnitsValue = ObservationPtr->GetUnits(Unit::Alliance::Self);
    const Unit* LeftWallUnitValue = FindWallStructureForSlot(SelfUnitsValue, RampWallDescriptorValue.LeftDepotSlot);
    const Unit* CenterWallUnitValue = FindWallStructureForSlot(SelfUnitsValue, RampWallDescriptorValue.BarracksSlot);
    const Unit* RightWallUnitValue = FindWallStructureForSlot(SelfUnitsValue, RampWallDescriptorValue.RightDepotSlot);

    std::cout << " | Left " << GetWallSlotOccupancyLabel(LeftWallUnitValue)
              << " | Center " << GetWallSlotOccupancyLabel(CenterWallUnitValue)
              << " | Right " << GetWallSlotOccupancyLabel(RightWallUnitValue)
              << std::endl;
}

void TerranAgent::UpdateExecutionTelemetry(const FFrameContext& Frame)
{
    ExecutionTelemetry.AdvanceStep(CurrentStep);
    const bool IsSupplyBlockedValue = ObservationPtr != nullptr && ObservationPtr->GetFoodCap() < 200U &&
                                      GameStateDescriptor.BuildPlanning.AvailableSupply == 0U;
    ExecutionTelemetry.UpdateSupplyBlockState(GetExecutionConditionState(IsSupplyBlockedValue), CurrentStep,
                                              Frame.GameLoop);

    const bool IsBankingMineralsValue = GameStateDescriptor.BuildPlanning.AvailableMinerals >= 400U;
    ExecutionTelemetry.UpdateMineralBankState(GetExecutionConditionState(IsBankingMineralsValue), CurrentStep,
                                              Frame.GameLoop, GameStateDescriptor.BuildPlanning.AvailableMinerals);

    std::unordered_map<Tag, FUnitIntent> FirstIntentByActor;
    for (const FUnitIntent& IntentValue : IntentBuffer.Intents)
    {
        if (IntentValue.ActorTag == NullTag)
        {
            continue;
        }

        const std::unordered_map<Tag, FUnitIntent>::const_iterator FoundIntentValue =
            FirstIntentByActor.find(IntentValue.ActorTag);
        if (FoundIntentValue == FirstIntentByActor.end())
        {
            FirstIntentByActor.emplace(IntentValue.ActorTag, IntentValue);
            continue;
        }

        if (!FoundIntentValue->second.Matches(IntentValue))
        {
            ExecutionTelemetry.RecordActorIntentConflict(CurrentStep, Frame.GameLoop, IntentValue.ActorTag,
                                                         IntentValue.Ability, IntentValue.Domain);
        }
    }

    const uint32_t PlannedMarineCountValue = AgentState.Units.GetUnitCount(UNIT_TYPEID::TERRAN_MARINE) +
                                             AgentState.Units.GetUnitsInConstruction(UNIT_TYPEID::TERRAN_MARINE) +
                                             CountOrdersAndIntentsForAbility(ABILITY_ID::TRAIN_MARINE);
    const uint32_t PlannedMarauderCountValue = AgentState.Units.GetUnitCount(UNIT_TYPEID::TERRAN_MARAUDER) +
                                               AgentState.Units.GetUnitsInConstruction(UNIT_TYPEID::TERRAN_MARAUDER) +
                                               CountOrdersAndIntentsForAbility(ABILITY_ID::TRAIN_MARAUDER);
    const bool HasBarracksDemandValue =
        PlannedMarineCountValue < GameStateDescriptor.BuildPlanning.DesiredMarineCount ||
        PlannedMarauderCountValue < GameStateDescriptor.BuildPlanning.DesiredMarauderCount;
    if (ObservationPtr != nullptr && HasBarracksDemandValue)
    {
        const Units BarracksUnitsValue =
            ObservationPtr->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_BARRACKS));
        for (const Unit* BarracksUnitValue : BarracksUnitsValue)
        {
            if (BarracksUnitValue == nullptr || BarracksUnitValue->build_progress < 1.0f ||
                !BarracksUnitValue->orders.empty() || IntentBuffer.HasIntentForActor(BarracksUnitValue->tag))
            {
                continue;
            }

            ExecutionTelemetry.RecordIdleProductionConflict(CurrentStep, Frame.GameLoop, BarracksUnitValue->tag,
                                                            UNIT_TYPEID::TERRAN_BARRACKS,
                                                            PlannedMarineCountValue <
                                                                    GameStateDescriptor.BuildPlanning.DesiredMarineCount
                                                                ? ABILITY_ID::TRAIN_MARINE
                                                                : ABILITY_ID::TRAIN_MARAUDER);
        }
    }

    const uint32_t PlannedSCVCountValue = AgentState.Units.GetWorkerCount() +
                                          AgentState.Units.GetUnitsInConstruction(UNIT_TYPEID::TERRAN_SCV) +
                                          CountOrdersAndIntentsForAbility(ABILITY_ID::TRAIN_SCV);
    const bool HasWorkerDemandValue =
        PlannedSCVCountValue < std::max<uint32_t>(GameStateDescriptor.BuildPlanning.DesiredWorkerCount,
                                                  static_cast<uint32_t>(AgentState.Buildings.GetTownHallCount() * 20U));
    if (HasWorkerDemandValue)
    {
        const Unit* TownHallUnitValue = AgentState.UnitContainer.GetFirstIdleTownHall();
        if (TownHallUnitValue != nullptr && !IntentBuffer.HasIntentForActor(TownHallUnitValue->tag))
        {
            ExecutionTelemetry.RecordIdleProductionConflict(CurrentStep, Frame.GameLoop, TownHallUnitValue->tag,
                                                            TownHallUnitValue->unit_type.ToType(),
                                                            ABILITY_ID::TRAIN_SCV);
        }
    }

    const uint32_t PlannedHellionCountValue = AgentState.Units.GetUnitCount(UNIT_TYPEID::TERRAN_HELLION) +
                                              AgentState.Units.GetUnitsInConstruction(UNIT_TYPEID::TERRAN_HELLION) +
                                              CountOrdersAndIntentsForAbility(ABILITY_ID::TRAIN_HELLION);
    const uint32_t PlannedCycloneCountValue = AgentState.Units.GetUnitCount(UNIT_TYPEID::TERRAN_CYCLONE) +
                                              AgentState.Units.GetUnitsInConstruction(UNIT_TYPEID::TERRAN_CYCLONE) +
                                              CountOrdersAndIntentsForAbility(ABILITY_ID::TRAIN_CYCLONE);
    const uint32_t PlannedSiegeTankCountValue = AgentState.Units.GetUnitCount(UNIT_TYPEID::TERRAN_SIEGETANK) +
                                                AgentState.Units.GetUnitsInConstruction(UNIT_TYPEID::TERRAN_SIEGETANK) +
                                                CountOrdersAndIntentsForAbility(ABILITY_ID::TRAIN_SIEGETANK);
    const bool HasFactoryDemandValue =
        PlannedHellionCountValue < GameStateDescriptor.BuildPlanning.DesiredHellionCount ||
        PlannedCycloneCountValue < GameStateDescriptor.BuildPlanning.DesiredCycloneCount ||
        PlannedSiegeTankCountValue < GameStateDescriptor.BuildPlanning.DesiredSiegeTankCount;
    if (ObservationPtr != nullptr && HasFactoryDemandValue)
    {
        const Units FactoryUnitsValue =
            ObservationPtr->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_FACTORY));
        for (const Unit* FactoryUnitValue : FactoryUnitsValue)
        {
            if (FactoryUnitValue == nullptr || FactoryUnitValue->build_progress < 1.0f || !FactoryUnitValue->orders.empty() ||
                IntentBuffer.HasIntentForActor(FactoryUnitValue->tag))
            {
                continue;
            }

            const ABILITY_ID FactoryDemandAbilityValue =
                PlannedSiegeTankCountValue < GameStateDescriptor.BuildPlanning.DesiredSiegeTankCount
                    ? ABILITY_ID::TRAIN_SIEGETANK
                    : (PlannedCycloneCountValue < GameStateDescriptor.BuildPlanning.DesiredCycloneCount
                           ? ABILITY_ID::TRAIN_CYCLONE
                           : ABILITY_ID::TRAIN_HELLION);
            ExecutionTelemetry.RecordIdleProductionConflict(CurrentStep, Frame.GameLoop, FactoryUnitValue->tag,
                                                            UNIT_TYPEID::TERRAN_FACTORY,
                                                            FactoryDemandAbilityValue);
        }
    }

    const uint32_t PlannedMedivacCountValue = AgentState.Units.GetUnitCount(UNIT_TYPEID::TERRAN_MEDIVAC) +
                                              AgentState.Units.GetUnitsInConstruction(UNIT_TYPEID::TERRAN_MEDIVAC) +
                                              CountOrdersAndIntentsForAbility(ABILITY_ID::TRAIN_MEDIVAC);
    const uint32_t PlannedLiberatorCountValue = AgentState.Units.GetUnitCount(UNIT_TYPEID::TERRAN_LIBERATOR) +
                                                AgentState.Units.GetUnitsInConstruction(UNIT_TYPEID::TERRAN_LIBERATOR) +
                                                CountOrdersAndIntentsForAbility(ABILITY_ID::TRAIN_LIBERATOR);
    const bool HasStarportDemandValue =
        PlannedMedivacCountValue < GameStateDescriptor.BuildPlanning.DesiredMedivacCount ||
        PlannedLiberatorCountValue < GameStateDescriptor.BuildPlanning.DesiredLiberatorCount;
    if (ObservationPtr != nullptr && HasStarportDemandValue)
    {
        const Units StarportUnitsValue =
            ObservationPtr->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_STARPORT));
        for (const Unit* StarportUnitValue : StarportUnitsValue)
        {
            if (StarportUnitValue == nullptr || StarportUnitValue->build_progress < 1.0f ||
                !StarportUnitValue->orders.empty() || IntentBuffer.HasIntentForActor(StarportUnitValue->tag))
            {
                continue;
            }

            ExecutionTelemetry.RecordIdleProductionConflict(CurrentStep, Frame.GameLoop, StarportUnitValue->tag,
                                                            UNIT_TYPEID::TERRAN_STARPORT,
                                                            PlannedMedivacCountValue <
                                                                    GameStateDescriptor.BuildPlanning.DesiredMedivacCount
                                                                ? ABILITY_ID::TRAIN_MEDIVAC
                                                                : ABILITY_ID::TRAIN_LIBERATOR);
        }
    }

    const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptor.CommandAuthoritySchedulingState;
    const size_t OrderCountValue = CommandAuthoritySchedulingStateValue.OrderIds.size();
    for (size_t OrderIndexValue = 0U; OrderIndexValue < OrderCountValue; ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.LastDeferralSteps[OrderIndexValue] != CurrentStep ||
            CommandAuthoritySchedulingStateValue.LastDeferralReasons[OrderIndexValue] ==
                ECommandOrderDeferralReason::None)
        {
            continue;
        }

        ExecutionTelemetry.RecordSchedulerOrderDeferred(
            CurrentStep, Frame.GameLoop, CommandAuthoritySchedulingStateValue.OrderIds[OrderIndexValue],
            CommandAuthoritySchedulingStateValue.PlanStepIds[OrderIndexValue],
            CommandAuthoritySchedulingStateValue.ActorTags[OrderIndexValue],
            CommandAuthoritySchedulingStateValue.AbilityIds[OrderIndexValue],
            CommandAuthoritySchedulingStateValue.IntentDomains[OrderIndexValue],
            CommandAuthoritySchedulingStateValue.LastDeferralReasons[OrderIndexValue]);
    }
}

void TerranAgent::ProduceRecoveryIntents(const FFrameContext& Frame)
{
    (void)Frame;

    std::unordered_set<Tag> RecoveryCandidates = std::move(PendingRecoveryWorkers);
    PendingRecoveryWorkers.clear();

    for (const Unit* Worker : AgentState.UnitContainer.GetWorkers())
    {
        if (Worker && Worker->orders.empty())
        {
            RecoveryCandidates.insert(Worker->tag);
        }
    }

    for (Tag WorkerTag : RecoveryCandidates)
    {
        if (IntentBuffer.HasIntentForActor(WorkerTag))
        {
            continue;
        }

        const Unit* Worker = AgentState.UnitContainer.GetUnitByTag(WorkerTag);
        if (!Worker || !Worker->orders.empty())
        {
            continue;
        }

        const Unit* RecoveryMineralPatchValue = SelectRecoveryMineralPatchForWorker(*Worker);
        if (RecoveryMineralPatchValue != nullptr)
        {
            IntentBuffer.Add(FUnitIntent::CreateUnitTarget(Worker->tag, ABILITY_ID::SMART,
                                                           RecoveryMineralPatchValue->tag,
                                                           300, EIntentDomain::Recovery));
            continue;
        }
    }
}

void TerranAgent::ProduceSchedulerIntents(const FFrameContext& Frame)
{
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptor.CommandAuthoritySchedulingState;
    const FSteadyTimePoint StrategicPhaseStartTimeValue = FSteadyClock::now();

    if (CommandTaskAdmissionService != nullptr)
    {
        CommandAuthorityProcessor.ProcessSchedulerStep(GameStateDescriptor, *CommandTaskAdmissionService);
    }
    else
    {
        CommandAuthorityProcessor.ProcessSchedulerStep(GameStateDescriptor);
    }
    FSteadyTimePoint StrategicPhaseEndTimeValue = FSteadyClock::now();
    LastSchedulerStrategicProcessingMicroseconds =
        GetElapsedMicroseconds(StrategicPhaseStartTimeValue, StrategicPhaseEndTimeValue);

    if (EconomyProductionOrderExpander != nullptr && BuildPlacementService != nullptr)
    {
        const FSteadyTimePoint EconomyPhaseStartTimeValue = FSteadyClock::now();
        const uint32_t RecoveryMoveIntentCountBeforeValue = CountRecoveryMoveIntents(IntentBuffer);
        CommandAuthoritySchedulingStateValue.BeginMutationBatch();
        EconomyProductionOrderExpander->ExpandEconomyAndProductionOrders(
            Frame, AgentState, GameStateDescriptor, IntentBuffer, *BuildPlacementService, ExpansionLocations);
        if (CommandTaskAdmissionService != nullptr)
        {
            CommandTaskAdmissionService->ParkDeferredOrders(GameStateDescriptor);
        }
        CommandAuthoritySchedulingStateValue.EndMutationBatch();
        const uint32_t RecoveryMoveIntentCountAfterValue = CountRecoveryMoveIntents(IntentBuffer);
        LastBlockerReliefMoveCount =
            RecoveryMoveIntentCountAfterValue >= RecoveryMoveIntentCountBeforeValue
                ? (RecoveryMoveIntentCountAfterValue - RecoveryMoveIntentCountBeforeValue)
                : 0U;

        const FSteadyTimePoint EconomyPhaseEndTimeValue = FSteadyClock::now();
        LastSchedulerEconomyProcessingMicroseconds =
            GetElapsedMicroseconds(EconomyPhaseStartTimeValue, EconomyPhaseEndTimeValue);
    }
    else
    {
        LastSchedulerEconomyProcessingMicroseconds = 0U;
        LastBlockerReliefMoveCount = 0U;

    }

    if (ArmyOrderExpander != nullptr)
    {
        const FSteadyTimePoint ArmyPhaseStartTimeValue = FSteadyClock::now();
        CommandAuthoritySchedulingStateValue.BeginMutationBatch();
        ArmyOrderExpander->ExpandArmyOrders(Frame, AgentState, GameStateDescriptor, ExpansionLocations, ArmyAssemblyPoint,
                                            GameStateDescriptor.CommandAuthoritySchedulingState);
        if (ArmyPlanner != nullptr)
        {
            ArmyPlanner->ProduceArmyPlan(GameStateDescriptor, GameStateDescriptor.ArmyState);
        }
        CommandAuthoritySchedulingStateValue.EndMutationBatch();
        const FSteadyTimePoint ArmyPhaseEndTimeValue = FSteadyClock::now();
        LastSchedulerArmyProcessingMicroseconds =
            GetElapsedMicroseconds(ArmyPhaseStartTimeValue, ArmyPhaseEndTimeValue);
    }
    else if (ArmyPlanner != nullptr)
    {
        const FSteadyTimePoint ArmyPhaseStartTimeValue = FSteadyClock::now();
        ArmyPlanner->ProduceArmyPlan(GameStateDescriptor, GameStateDescriptor.ArmyState);
        const FSteadyTimePoint ArmyPhaseEndTimeValue = FSteadyClock::now();
        LastSchedulerArmyProcessingMicroseconds =
            GetElapsedMicroseconds(ArmyPhaseStartTimeValue, ArmyPhaseEndTimeValue);
    }
    else
    {
        LastSchedulerArmyProcessingMicroseconds = 0U;
    }

    if (SquadOrderExpander != nullptr)
    {
        const FSteadyTimePoint SquadPhaseStartTimeValue = FSteadyClock::now();
        CommandAuthoritySchedulingStateValue.BeginMutationBatch();
        SquadOrderExpander->ExpandSquadOrders(Frame, AgentState, GameStateDescriptor, ArmyAssemblyPoint,
                                              GameStateDescriptor.CommandAuthoritySchedulingState);
        CommandAuthoritySchedulingStateValue.EndMutationBatch();
        const FSteadyTimePoint SquadPhaseEndTimeValue = FSteadyClock::now();
        LastSchedulerSquadProcessingMicroseconds =
            GetElapsedMicroseconds(SquadPhaseStartTimeValue, SquadPhaseEndTimeValue);
    }
    else
    {
        LastSchedulerSquadProcessingMicroseconds = 0U;
    }

    if (UnitExecutionPlanner != nullptr)
    {
        const FSteadyTimePoint UnitExecutionPhaseStartTimeValue = FSteadyClock::now();
        CommandAuthoritySchedulingStateValue.BeginMutationBatch();
        LastArmyExecutionOrderCount = UnitExecutionPlanner->ExpandUnitExecutionOrders(
            Frame, AgentState, GameStateDescriptor, ArmyAssemblyPoint, GameStateDescriptor.CommandAuthoritySchedulingState);
        LastUnitExecutionReplanCount = LastArmyExecutionOrderCount;
        CommandAuthoritySchedulingStateValue.EndMutationBatch();
        LastActiveIndexedExecutionOrderCount = static_cast<uint32_t>(
            GameStateDescriptor.CommandAuthoritySchedulingState.ActiveExecutionOrderIndexByActorTag.size());
        const FSteadyTimePoint UnitExecutionPhaseEndTimeValue = FSteadyClock::now();
        LastSchedulerUnitExecutionProcessingMicroseconds =
            GetElapsedMicroseconds(UnitExecutionPhaseStartTimeValue, UnitExecutionPhaseEndTimeValue);
    }
    else
    {
        LastArmyExecutionOrderCount = 0U;
        LastUnitExecutionReplanCount = 0U;
        LastActiveIndexedExecutionOrderCount = 0U;
        LastSchedulerUnitExecutionProcessingMicroseconds = 0U;
    }

    if (CommandTaskPriorityService != nullptr)
    {
        CommandTaskPriorityService->UpdateTaskPriorities(GameStateDescriptor);
    }

    const FSteadyTimePoint DrainPhaseStartTimeValue = FSteadyClock::now();
    IntentSchedulingService.DrainReadyIntents(GameStateDescriptor.CommandAuthoritySchedulingState, IntentBuffer,
                                              GameStateDescriptor.CommandAuthoritySchedulingState.MaxUnitIntentsPerStep);
    const FSteadyTimePoint DrainPhaseEndTimeValue = FSteadyClock::now();
    LastSchedulerDrainMicroseconds = GetElapsedMicroseconds(DrainPhaseStartTimeValue, DrainPhaseEndTimeValue);
}

void TerranAgent::ProduceWallGateIntents(const FFrameContext& Frame)
{
    if (ObservationPtr == nullptr || WallGateController == nullptr)
    {
        return;
    }

    const Units SelfUnitsValue = ObservationPtr->GetUnits(Unit::Alliance::Self);
    const Units EnemyUnitsValue = ObservationPtr->GetUnits(Unit::Alliance::Enemy);
    const EWallGateState DesiredWallGateStateValue = WallGateController->EvaluateDesiredWallGateState(
        SelfUnitsValue, EnemyUnitsValue, GameStateDescriptor.RampWallDescriptor);
    if (DesiredWallGateStateValue == EWallGateState::Closed && CurrentWallGateState != EWallGateState::Closed)
    {
        ExecutionTelemetry.RecordWallThreatDetected(CurrentStep, Frame.GameLoop);
    }

    // Always produce wall gate intents so newly built depots get lowered even when state is already Open
    WallGateController->ProduceWallGateIntents(SelfUnitsValue, GameStateDescriptor.RampWallDescriptor,
                                               DesiredWallGateStateValue, IntentBuffer);
    if (DesiredWallGateStateValue != CurrentWallGateState)
    {
        switch (DesiredWallGateStateValue)
        {
            case EWallGateState::Open:
                ExecutionTelemetry.RecordWallOpened(CurrentStep, Frame.GameLoop);
                break;
            case EWallGateState::Closed:
                ExecutionTelemetry.RecordWallClosed(CurrentStep, Frame.GameLoop);
                break;
            case EWallGateState::Unavailable:
            default:
                break;
        }
    }

    CurrentWallGateState = DesiredWallGateStateValue;
}

void TerranAgent::ProduceWorkerHarvestIntents(const FFrameContext& Frame)
{
    if (Frame.Observation == nullptr)
    {
        return;
    }

    const Units RefineryUnitsValue =
        Frame.Observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_REFINERY));
    std::unordered_set<Tag> ReservedWorkerTagsValue;
    std::unordered_map<Tag, int> PlannedFillCountsByRefineryTagValue;
    std::unordered_map<Tag, int> PlannedReliefCountsByRefineryTagValue;

    for (const Unit* RefineryUnitValue : RefineryUnitsValue)
    {
        if (RefineryUnitValue == nullptr || RefineryUnitValue->build_progress < 1.0f ||
            RefineryUnitValue->ideal_harvesters <= 0 || RefineryUnitValue->vespene_contents <= 0)
        {
            continue;
        }

        const int CommittedHarvesterCountValue =
            WorkerSelectionService->GetCommittedHarvesterCountForRefinery(*Frame.Observation, AgentState,
                                                                          RefineryUnitValue->tag);
        const int EffectiveHarvesterCountValue =
            std::max(RefineryUnitValue->assigned_harvesters, CommittedHarvesterCountValue) +
            WorkerSelectionService->GetPlannedHarvesterDeltaForRefinery(PlannedFillCountsByRefineryTagValue,
                                                                        PlannedReliefCountsByRefineryTagValue,
                                                                        RefineryUnitValue->tag);
        const int MissingHarvesterCountValue =
            std::max(0, RefineryUnitValue->ideal_harvesters - EffectiveHarvesterCountValue);
        for (int MissingHarvesterIndexValue = 0; MissingHarvesterIndexValue < MissingHarvesterCountValue;
             ++MissingHarvesterIndexValue)
        {
            const Unit* WorkerUnitValue = WorkerSelectionService->SelectWorkerForRefinery(
                *Frame.Observation, AgentState, GameStateDescriptor.CommandAuthoritySchedulingState, IntentBuffer,
                *RefineryUnitValue, ReservedWorkerTagsValue);
            if (WorkerUnitValue == nullptr)
            {
                break;
            }

            IntentBuffer.Add(FUnitIntent::CreateUnitTarget(WorkerUnitValue->tag, ABILITY_ID::HARVEST_GATHER,
                                                           RefineryUnitValue->tag, GasHarvestIntentPriorityValue,
                                                           EIntentDomain::Recovery));
            ReservedWorkerTagsValue.insert(WorkerUnitValue->tag);
            ++PlannedFillCountsByRefineryTagValue[RefineryUnitValue->tag];
        }
    }

    for (const Unit* RefineryUnitValue : RefineryUnitsValue)
    {
        if (RefineryUnitValue == nullptr || RefineryUnitValue->build_progress < 1.0f ||
            RefineryUnitValue->ideal_harvesters <= 0 || RefineryUnitValue->vespene_contents <= 0)
        {
            continue;
        }

        const int CommittedHarvesterCountValue =
            WorkerSelectionService->GetCommittedHarvesterCountForRefinery(*Frame.Observation, AgentState,
                                                                          RefineryUnitValue->tag);
        const int EffectiveHarvesterCountValue =
            std::max(RefineryUnitValue->assigned_harvesters, CommittedHarvesterCountValue) +
            WorkerSelectionService->GetPlannedHarvesterDeltaForRefinery(PlannedFillCountsByRefineryTagValue,
                                                                        PlannedReliefCountsByRefineryTagValue,
                                                                        RefineryUnitValue->tag);
        const int ExcessHarvesterCountValue =
            std::max(0, EffectiveHarvesterCountValue - RefineryUnitValue->ideal_harvesters);
        for (int ExcessHarvesterIndexValue = 0; ExcessHarvesterIndexValue < ExcessHarvesterCountValue;
             ++ExcessHarvesterIndexValue)
        {
            const Unit* WorkerUnitValue = WorkerSelectionService->SelectWorkerForGasRelief(
                *Frame.Observation, AgentState, GameStateDescriptor.CommandAuthoritySchedulingState, IntentBuffer,
                *RefineryUnitValue, ReservedWorkerTagsValue);
            if (WorkerUnitValue == nullptr)
            {
                break;
            }

            const Unit* MineralPatchValue = SelectRecoveryMineralPatchForWorker(*WorkerUnitValue);
            if (MineralPatchValue == nullptr)
            {
                break;
            }

            IntentBuffer.Add(FUnitIntent::CreateUnitTarget(WorkerUnitValue->tag, ABILITY_ID::SMART,
                                                           MineralPatchValue->tag, GasReliefIntentPriorityValue,
                                                           EIntentDomain::Recovery));
            ReservedWorkerTagsValue.insert(WorkerUnitValue->tag);
            ++PlannedReliefCountsByRefineryTagValue[RefineryUnitValue->tag];
        }
    }

    ProduceWorkerMineralRebalanceIntents(Frame, ReservedWorkerTagsValue);
}

void TerranAgent::ProduceWorkerMineralRebalanceIntents(const FFrameContext& Frame,
                                                       std::unordered_set<Tag>& ReservedWorkerTagsValue)
{
    if (Frame.Observation == nullptr)
    {
        return;
    }

    constexpr uint32_t MaxMineralRebalanceIntentCountPerStepValue = 8U;

    const Units TownHallUnitsValue = Frame.Observation->GetUnits(Unit::Alliance::Self, IsTownHall());
    Units ReadyTownHallUnitsValue;
    ReadyTownHallUnitsValue.reserve(TownHallUnitsValue.size());
    for (const Unit* TownHallUnitValue : TownHallUnitsValue)
    {
        if (TownHallUnitValue == nullptr || TownHallUnitValue->build_progress < 1.0f ||
            TownHallUnitValue->ideal_harvesters <= 0)
        {
            continue;
        }

        ReadyTownHallUnitsValue.push_back(TownHallUnitValue);
    }

    if (ReadyTownHallUnitsValue.size() < 2U)
    {
        return;
    }

    std::unordered_map<Tag, int> PlannedInboundCountsByTownHallTagValue;
    std::unordered_map<Tag, int> PlannedOutboundCountsByTownHallTagValue;
    uint32_t AddedRebalanceIntentCountValue = 0U;

    for (const Unit* ReceiverTownHallUnitValue : ReadyTownHallUnitsValue)
    {
        if (ReceiverTownHallUnitValue == nullptr)
        {
            continue;
        }

        const int EffectiveAssignedHarvesterCountValue =
            std::max(ReceiverTownHallUnitValue->assigned_harvesters, 0) +
            WorkerSelectionService->GetPlannedHarvesterDeltaForTownHall(PlannedInboundCountsByTownHallTagValue,
                                                                      PlannedOutboundCountsByTownHallTagValue,
                                                                      ReceiverTownHallUnitValue->tag);
        const int MissingHarvesterCountValue =
            std::max(0, ReceiverTownHallUnitValue->ideal_harvesters - EffectiveAssignedHarvesterCountValue);
        for (int MissingHarvesterIndexValue = 0; MissingHarvesterIndexValue < MissingHarvesterCountValue;
             ++MissingHarvesterIndexValue)
        {
            if (AddedRebalanceIntentCountValue >= MaxMineralRebalanceIntentCountPerStepValue)
            {
                return;
            }

            const Unit* WorkerUnitValue = WorkerSelectionService->SelectWorkerForMineralRebalance(
                *Frame.Observation, AgentState, GameStateDescriptor.CommandAuthoritySchedulingState, IntentBuffer,
                *ReceiverTownHallUnitValue, ReadyTownHallUnitsValue, ReservedWorkerTagsValue,
                PlannedInboundCountsByTownHallTagValue, PlannedOutboundCountsByTownHallTagValue);
            if (WorkerUnitValue == nullptr)
            {
                break;
            }

            const Unit* SourceTownHallUnitValue =
                FindMineralTownHallForWorker(*Frame.Observation, *WorkerUnitValue, ReadyTownHallUnitsValue);
            const Unit* MineralPatchValue =
                FindNearestMineralPatchForTownHall(Point2D(ReceiverTownHallUnitValue->pos));
            if (SourceTownHallUnitValue == nullptr || MineralPatchValue == nullptr)
            {
                break;
            }

            IntentBuffer.Add(FUnitIntent::CreateUnitTarget(WorkerUnitValue->tag, ABILITY_ID::SMART,
                                                           MineralPatchValue->tag,
                                                           MineralRebalanceIntentPriorityValue,
                                                           EIntentDomain::Recovery));
            ReservedWorkerTagsValue.insert(WorkerUnitValue->tag);
            ++PlannedInboundCountsByTownHallTagValue[ReceiverTownHallUnitValue->tag];
            ++PlannedOutboundCountsByTownHallTagValue[SourceTownHallUnitValue->tag];
            ++AddedRebalanceIntentCountValue;
        }
    }
}

void TerranAgent::ProduceProductionRallyIntents()
{
    LastProductionRallyApplyCount = 0U;
    if (ObservationPtr == nullptr)
    {
        return;
    }

    if (CurrentStep >=
        (RecentProductionRallyCounterWindowStartStep + RecentProductionRallyCounterWindowStepCountValue))
    {
        RecentProductionRallyCounterWindowStartStep = CurrentStep;
        RecentProductionRallyApplyCount = 0U;
    }

    std::unordered_set<Tag> ActiveProductionStructureTagsValue;
    for (const Unit* ControlledUnitValue : AgentState.UnitContainer.ControlledUnits)
    {
        if (ControlledUnitValue == nullptr || ControlledUnitValue->build_progress < 1.0f)
        {
            continue;
        }

        const UNIT_TYPEID UnitTypeIdValue = ControlledUnitValue->unit_type.ToType();
        if (!IsProductionRallyStructureType(UnitTypeIdValue))
        {
            continue;
        }

        ActiveProductionStructureTagsValue.insert(ControlledUnitValue->tag);
        FProductionRallyState& ProductionRallyStateValue = ProductionRallyStates[ControlledUnitValue->tag];
        const bool bAssemblyAnchorChangedValue =
            DistanceSquared2D(ProductionRallyStateValue.DesiredRallyPoint, ProductionRallyPoint) > 1.0f;
        if (bAssemblyAnchorChangedValue)
        {
            ProductionRallyStateValue.DesiredRallyPoint = ProductionRallyPoint;
            ProductionRallyStateValue.ApplyState = EProductionRallyApplyState::PendingApply;
        }

        const bool bShouldApplyRallyValue =
            ProductionRallyStateValue.ApplyState != EProductionRallyApplyState::Applied ||
            ProductionRallyStateValue.LastAppliedGameLoop == 0U ||
            DistanceSquared2D(ProductionRallyStateValue.LastAppliedRallyPoint, ProductionRallyPoint) > 1.0f;
        if (!bShouldApplyRallyValue)
        {
            continue;
        }

        const AbilityID RallyAbilityValue = GetProductionStructureRallyAbility(UnitTypeIdValue);
        if (RallyAbilityValue == ABILITY_ID::INVALID)
        {
            continue;
        }

        PendingProductionRallyIntents.push_back(FUnitIntent::CreatePointTarget(
            ControlledUnitValue->tag, RallyAbilityValue, ProductionRallyPoint, 5, EIntentDomain::UnitProduction));
        ProductionRallyStateValue.LastAppliedRallyPoint = ProductionRallyPoint;
        ProductionRallyStateValue.LastAppliedGameLoop = GameStateDescriptor.CurrentGameLoop;
        ProductionRallyStateValue.ApplyState = EProductionRallyApplyState::Applied;
        ++LastProductionRallyApplyCount;
        ++RecentProductionRallyApplyCount;
    }

    for (std::unordered_map<Tag, FProductionRallyState>::iterator RallyStateIteratorValue =
             ProductionRallyStates.begin();
         RallyStateIteratorValue != ProductionRallyStates.end();)
    {
        if (ActiveProductionStructureTagsValue.find(RallyStateIteratorValue->first) !=
            ActiveProductionStructureTagsValue.end())
        {
            ++RallyStateIteratorValue;
            continue;
        }

        RallyStateIteratorValue = ProductionRallyStates.erase(RallyStateIteratorValue);
    }
}

void TerranAgent::ExecuteProductionRallyIntents()
{
    for (const FUnitIntent& RallyIntentValue : PendingProductionRallyIntents)
    {
        if (RallyIntentValue.TargetKind != EIntentTargetKind::Point)
        {
            continue;
        }

        Actions()->UnitCommand(RallyIntentValue.ActorTag, RallyIntentValue.Ability, RallyIntentValue.TargetPoint,
                               RallyIntentValue.Queued);
    }

    PendingProductionRallyIntents.clear();
}

void TerranAgent::ExecuteOrbitalAbilities(const FFrameContext& Frame)
{
    if (Frame.Observation == nullptr)
    {
        return;
    }

    static constexpr float MuleEnergyCostValue = 50.0f;

    const Units OrbitalCommandUnitsValue =
        Frame.Observation->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::TERRAN_ORBITALCOMMAND));

    for (const Unit* OrbitalUnitValue : OrbitalCommandUnitsValue)
    {
        if (OrbitalUnitValue == nullptr ||
            OrbitalUnitValue->build_progress < 1.0f ||
            OrbitalUnitValue->energy < MuleEnergyCostValue)
        {
            continue;
        }

        const Unit* NearestMineralPatchValue = FindNearestMineralPatchForTownHall(Point2D(OrbitalUnitValue->pos));
        if (NearestMineralPatchValue == nullptr)
        {
            continue;
        }

        Actions()->UnitCommand(OrbitalUnitValue, ABILITY_ID::EFFECT_CALLDOWNMULE,
                               NearestMineralPatchValue);
    }
}


void TerranAgent::ExecuteResolvedIntents(const FFrameContext& Frame, const std::vector<FUnitIntent>& Intents)
{
    (void)Frame;

    for (const FUnitIntent& Intent : Intents)
    {
#if _DEBUG
        if (Intent.Ability == ABILITY_ID::BUILD_REACTOR_BARRACKS ||
            Intent.Ability == ABILITY_ID::BUILD_TECHLAB_BARRACKS)
        {
            const Unit* AddonActorValue = Observation()->GetUnit(Intent.ActorTag);
            const Point2D AddonCenterValue(
                AddonActorValue ? (AddonActorValue->pos.x + 2.5f) : 0.0f,
                AddonActorValue ? (AddonActorValue->pos.y - 0.5f) : 0.0f);
            const bool PlacementQueryResultValue = AddonActorValue
                ? Query()->Placement(Intent.Ability, AddonCenterValue, AddonActorValue)
                : false;
            const bool PlacementQueryAtBarracksValue = AddonActorValue
                ? Query()->Placement(Intent.Ability, Point2D(AddonActorValue->pos), AddonActorValue)
                : false;
            std::cout << "[INTENT_EXEC] Ability=" << static_cast<int>(Intent.Ability)
                      << " ActorTag=" << Intent.ActorTag
                      << " TargetKind=" << static_cast<int>(Intent.TargetKind)
                      << " Queued=" << Intent.Queued
                      << " PlacementAtAddon=" << PlacementQueryResultValue
                      << " PlacementAtBarracks=" << PlacementQueryAtBarracksValue;
            if (AddonActorValue)
            {
                std::cout << " ActorPos=(" << AddonActorValue->pos.x << "," << AddonActorValue->pos.y << ")"
                          << " Orders=" << AddonActorValue->orders.size()
                          << " AddOnTag=" << AddonActorValue->add_on_tag
                          << " Flying=" << static_cast<int>(AddonActorValue->is_flying);
                for (size_t OrderIndex = 0U; OrderIndex < AddonActorValue->orders.size(); ++OrderIndex)
                {
                    std::cout << " Order[" << OrderIndex << "]=(ability="
                              << static_cast<int>(AddonActorValue->orders[OrderIndex].ability_id)
                              << " progress=" << AddonActorValue->orders[OrderIndex].progress << ")";
                }
            }
            std::cout << std::endl;
        }
#endif
        switch (Intent.TargetKind)
        {
            case EIntentTargetKind::None:
                Actions()->UnitCommand(Intent.ActorTag, Intent.Ability, Intent.Queued);
                break;
            case EIntentTargetKind::Point:
                Actions()->UnitCommand(Intent.ActorTag, Intent.Ability, Intent.TargetPoint, Intent.Queued);
                break;
            case EIntentTargetKind::Unit:
                Actions()->UnitCommand(Intent.ActorTag, Intent.Ability, Intent.TargetUnitTag, Intent.Queued);
                break;
            default:
                break;
        }
    }
}

void TerranAgent::UpdateDispatchedSchedulerOrders(const FFrameContext& Frame)
{
    (void)Frame;
    constexpr uint64_t ProducerConfirmationTimeoutGameLoopsValue = 96U;
    constexpr uint64_t ObservedConstructionConfirmationTimeoutGameLoopsValue = 224U;
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptor.CommandAuthoritySchedulingState;
    CommandAuthoritySchedulingStateValue.BeginMutationBatch();
    const std::vector<size_t> DispatchedOrderIndicesValue = CommandAuthoritySchedulingStateValue.DispatchedOrderIndices;
    for (const size_t OrderIndexValue : DispatchedOrderIndicesValue)
    {
        if (!CommandAuthoritySchedulingStateValue.IsOrderIndexValid(OrderIndexValue) ||
            CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] != ECommandAuthorityLayer::UnitExecution ||
            CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue] != EOrderLifecycleState::Dispatched)
        {
            continue;
        }

        const FCommandOrderRecord CommandOrderRecordValue =
            CommandAuthoritySchedulingStateValue.GetOrderRecord(OrderIndexValue);
        const uint32_t CurrentObservedCountValue = GetObservedCountForOrder(CommandOrderRecordValue);
        const uint32_t CurrentObservedInConstructionCountValue =
            GetObservedInConstructionCountForOrder(CommandOrderRecordValue);
        if (CurrentObservedCountValue > CommandOrderRecordValue.ObservedCountAtDispatch ||
            CurrentObservedInConstructionCountValue >
                CommandOrderRecordValue.ObservedInConstructionCountAtDispatch)
        {
            CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(CommandOrderRecordValue.OrderId,
                                                                       EOrderLifecycleState::Completed);
            continue;
        }

        const Unit* ActorUnitValue = AgentState.UnitContainer.GetUnitByTag(CommandOrderRecordValue.ActorTag);
        if (ActorUnitValue == nullptr && CommandOrderRecordValue.DispatchGameLoop > 0U &&
            GameStateDescriptor.CurrentGameLoop > CommandOrderRecordValue.DispatchGameLoop)
        {
            CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(CommandOrderRecordValue.OrderId,
                                                                       EOrderLifecycleState::Aborted);
            continue;
        }

        if (HasProducerConfirmedDispatchedOrder(CommandOrderRecordValue, ActorUnitValue))
        {
            CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(CommandOrderRecordValue.OrderId,
                                                                       EOrderLifecycleState::Completed);
            continue;
        }

        const uint64_t DispatchConfirmationTimeoutGameLoopsValue =
            DoesAbilityRequireObservedConstructionConfirmation(CommandOrderRecordValue.AbilityId)
                ? ObservedConstructionConfirmationTimeoutGameLoopsValue
                : ProducerConfirmationTimeoutGameLoopsValue;
        if (CommandOrderRecordValue.DispatchGameLoop == 0U ||
            GameStateDescriptor.CurrentGameLoop <
                (CommandOrderRecordValue.DispatchGameLoop + DispatchConfirmationTimeoutGameLoopsValue))
        {
            continue;
        }

        CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(CommandOrderRecordValue.OrderId,
                                                                   EOrderLifecycleState::Aborted);
    }
    const bool bCompactionIntervalElapsedValue =
        CurrentStep >= (LastTerminalCompactionStep + TerminalOrderCompactionIntervalStepCountValue);
    const bool bCompactionThresholdReachedValue =
        CommandAuthoritySchedulingStateValue.OrderIds.size() >= TerminalOrderCompactionTriggerCountValue;
    if (bCompactionIntervalElapsedValue && bCompactionThresholdReachedValue &&
        CommandAuthoritySchedulingStateValue.CompactTerminalOrders(&GameStateDescriptor.OpeningPlanExecutionState))
    {
        LastTerminalCompactionStep = CurrentStep;
    }
    CommandAuthoritySchedulingStateValue.EndMutationBatch();
}

void TerranAgent::CaptureNewlyDispatchedSchedulerOrders(const FFrameContext& Frame)
{
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptor.CommandAuthoritySchedulingState;
    const size_t OrderCountValue = CommandAuthoritySchedulingStateValue.OrderIds.size();
    for (size_t OrderIndexValue = 0U; OrderIndexValue < OrderCountValue; ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] != ECommandAuthorityLayer::UnitExecution ||
            CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue] != EOrderLifecycleState::Dispatched ||
            CommandAuthoritySchedulingStateValue.DispatchAttemptCounts[OrderIndexValue] > 0U)
        {
            continue;
        }

        const FCommandOrderRecord CommandOrderRecordValue =
            CommandAuthoritySchedulingStateValue.GetOrderRecord(OrderIndexValue);
        CommandAuthoritySchedulingStateValue.SetOrderDispatchState(
            CommandOrderRecordValue.OrderId, CurrentStep, Frame.GameLoop, GetObservedCountForOrder(CommandOrderRecordValue),
            GetObservedInConstructionCountForOrder(CommandOrderRecordValue));
    }
}

uint32_t TerranAgent::CountOrdersAndIntentsForAbility(const ABILITY_ID AbilityIdValue) const
{
    uint32_t OrderCountValue = 0U;

    for (const Unit* UnitValue : AgentState.UnitContainer.ControlledUnits)
    {
        if (!UnitValue)
        {
            continue;
        }

        for (const UnitOrder& OrderValue : UnitValue->orders)
        {
            if (OrderValue.ability_id == AbilityIdValue)
            {
                ++OrderCountValue;
            }
        }
    }

    for (const FUnitIntent& IntentValue : IntentBuffer.Intents)
    {
        if (IntentValue.Ability == AbilityIdValue)
        {
            ++OrderCountValue;
        }
    }

    return OrderCountValue;
}

uint32_t TerranAgent::GetObservedCountForOrder(const FCommandOrderRecord& CommandOrderRecordValue) const
{
    switch (CommandOrderRecordValue.ResultUnitTypeId)
    {
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
            return GameStateDescriptor.BuildPlanning.ObservedTownHallCount;
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
            return GameStateDescriptor.BuildPlanning.ObservedOrbitalCommandCount;
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOT:
            return static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_SUPPLYDEPOT)]) +
                   static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED)]);
        case UNIT_TYPEID::TERRAN_BARRACKS:
            return static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_BARRACKS)]) +
                   static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_BARRACKSFLYING)]);
        case UNIT_TYPEID::TERRAN_FACTORY:
            return static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_FACTORY)]) +
                   static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_FACTORYFLYING)]);
        case UNIT_TYPEID::TERRAN_STARPORT:
            return static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_STARPORT)]) +
                   static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_STARPORTFLYING)]);
        case UNIT_TYPEID::TERRAN_REFINERY:
            return static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_REFINERY)]) +
                   static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
                       UNIT_TYPEID::TERRAN_REFINERYRICH)]);
        case UNIT_TYPEID::TERRAN_HELLION:
            return static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedUnitCounts[GetTerranUnitTypeIndex(
                       UNIT_TYPEID::TERRAN_HELLION)]) +
                   static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedUnitCounts[GetTerranUnitTypeIndex(
                       UNIT_TYPEID::TERRAN_HELLIONTANK)]);
        case UNIT_TYPEID::TERRAN_LIBERATOR:
            return static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedUnitCounts[GetTerranUnitTypeIndex(
                       UNIT_TYPEID::TERRAN_LIBERATOR)]) +
                   static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedUnitCounts[GetTerranUnitTypeIndex(
                       UNIT_TYPEID::TERRAN_LIBERATORAG)]);
        case UNIT_TYPEID::TERRAN_SIEGETANK:
            return static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedUnitCounts[GetTerranUnitTypeIndex(
                       UNIT_TYPEID::TERRAN_SIEGETANK)]) +
                   static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedUnitCounts[GetTerranUnitTypeIndex(
                       UNIT_TYPEID::TERRAN_SIEGETANKSIEGED)]);
        default:
            break;
    }

    if (IsTerranBuilding(CommandOrderRecordValue.ResultUnitTypeId))
    {
        const size_t BuildingTypeIndexValue = GetTerranBuildingTypeIndex(CommandOrderRecordValue.ResultUnitTypeId);
        return IsTerranBuildingTypeIndexValid(BuildingTypeIndexValue)
                   ? static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingCounts[BuildingTypeIndexValue])
                   : 0U;
    }

    const size_t UnitTypeIndexValue = GetTerranUnitTypeIndex(CommandOrderRecordValue.ResultUnitTypeId);
    return IsTerranUnitTypeIndexValid(UnitTypeIndexValue)
               ? static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedUnitCounts[UnitTypeIndexValue])
               : 0U;
}

uint32_t TerranAgent::GetObservedInConstructionCountForOrder(const FCommandOrderRecord& CommandOrderRecordValue) const
{
    if (CommandOrderRecordValue.ResultUnitTypeId == UNIT_TYPEID::TERRAN_COMMANDCENTER)
    {
        return static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedBuildingsInConstruction[
            GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_COMMANDCENTER)]);
    }

    if (IsTerranBuilding(CommandOrderRecordValue.ResultUnitTypeId))
    {
        const size_t BuildingTypeIndexValue = GetTerranBuildingTypeIndex(CommandOrderRecordValue.ResultUnitTypeId);
        return IsTerranBuildingTypeIndexValid(BuildingTypeIndexValue)
                   ? static_cast<uint32_t>(
                         GameStateDescriptor.BuildPlanning.ObservedBuildingsInConstruction[BuildingTypeIndexValue])
                   : 0U;
    }

    const size_t UnitTypeIndexValue = GetTerranUnitTypeIndex(CommandOrderRecordValue.ResultUnitTypeId);
    return IsTerranUnitTypeIndexValid(UnitTypeIndexValue)
               ? static_cast<uint32_t>(GameStateDescriptor.BuildPlanning.ObservedUnitsInConstruction[UnitTypeIndexValue])
               : 0U;
}

bool TerranAgent::HasProducerConfirmedDispatchedOrder(const FCommandOrderRecord& CommandOrderRecordValue,
                                                      const Unit* ActorUnitValue) const
{
    if (ActorUnitValue == nullptr)
    {
        return false;
    }

    if (DoesAbilityRequireObservedConstructionConfirmation(CommandOrderRecordValue.AbilityId))
    {
        return false;
    }

    if (CommandOrderRecordValue.AbilityId == ABILITY_ID::MORPH_ORBITALCOMMAND &&
        ActorUnitValue->unit_type.ToType() == UNIT_TYPEID::TERRAN_ORBITALCOMMAND)
    {
        return true;
    }

    if (IsTerranAddonBuildAbility(CommandOrderRecordValue.AbilityId) &&
        ActorUnitValue->add_on_tag != NullTag)
    {
        return true;
    }

    for (const UnitOrder& UnitOrderValue : ActorUnitValue->orders)
    {
        if (UnitOrderValue.ability_id == CommandOrderRecordValue.AbilityId)
        {
            return true;
        }
    }

    return false;
}

const Unit* TerranAgent::FindNearestMineralPatch(const Point2D& OriginPointValue) const
{
    float NearestDistanceSquaredValue = std::numeric_limits<float>::max();
    const Unit* SelectedMineralPatchValue = nullptr;
    const IsMineralPatch MineralFilterValue;

    for (const Unit* NeutralUnitValue : NeutralUnits)
    {
        if (NeutralUnitValue == nullptr || !MineralFilterValue(*NeutralUnitValue))
        {
            continue;
        }

        const Point2D DeltaPointValue = Point2D(NeutralUnitValue->pos) - OriginPointValue;
        const float DistanceSquaredValue = Dot2D(DeltaPointValue, DeltaPointValue);
        if (DistanceSquaredValue <= NearestDistanceSquaredValue)
        {
            NearestDistanceSquaredValue = DistanceSquaredValue;
            SelectedMineralPatchValue = NeutralUnitValue;
        }
    }

    return SelectedMineralPatchValue;
}

const Unit* TerranAgent::FindNearestMineralPatchForTownHall(const Point2D& TownHallPointValue) const
{
    constexpr float SupportedMineralLineRadiusSquaredValue = 225.0f;

    float NearestDistanceSquaredValue = std::numeric_limits<float>::max();
    const Unit* SelectedMineralPatchValue = nullptr;
    const IsMineralPatch MineralFilterValue;

    for (const Unit* NeutralUnitValue : NeutralUnits)
    {
        if (NeutralUnitValue == nullptr || !MineralFilterValue(*NeutralUnitValue))
        {
            continue;
        }

        const float DistanceToTownHallSquaredValue =
            DistanceSquared2D(Point2D(NeutralUnitValue->pos), TownHallPointValue);
        if (DistanceToTownHallSquaredValue > SupportedMineralLineRadiusSquaredValue)
        {
            continue;
        }

        if (DistanceToTownHallSquaredValue >= NearestDistanceSquaredValue)
        {
            continue;
        }

        NearestDistanceSquaredValue = DistanceToTownHallSquaredValue;
        SelectedMineralPatchValue = NeutralUnitValue;
    }

    return SelectedMineralPatchValue;
}

const Unit* TerranAgent::FindNearestReadyTownHall(const Point2D& OriginPointValue) const
{
    float NearestDistanceSquaredValue = std::numeric_limits<float>::max();
    const Unit* SelectedTownHallUnitValue = nullptr;
    for (const Unit* BuildingUnitValue : AgentState.UnitContainer.ControlledUnits)
    {
        if (BuildingUnitValue == nullptr || BuildingUnitValue->build_progress < 1.0f)
        {
            continue;
        }

        if (!IsTownHallStructureType(BuildingUnitValue->unit_type.ToType()))
        {
            continue;
        }

        const float DistanceSquaredValue =
            DistanceSquared2D(Point2D(BuildingUnitValue->pos), OriginPointValue);
        if (DistanceSquaredValue >= NearestDistanceSquaredValue)
        {
            continue;
        }

        NearestDistanceSquaredValue = DistanceSquaredValue;
        SelectedTownHallUnitValue = BuildingUnitValue;
    }

    return SelectedTownHallUnitValue;
}

const Unit* TerranAgent::SelectRecoveryMineralPatchForWorker(const Unit& WorkerUnitValue) const
{
    const Unit* ReadyTownHallUnitValue = FindNearestReadyTownHall(Point2D(WorkerUnitValue.pos));
    if (ReadyTownHallUnitValue == nullptr)
    {
        return nullptr;
    }

    return FindNearestMineralPatchForTownHall(Point2D(ReadyTownHallUnitValue->pos));
}

Point2D TerranAgent::GetEnemyTargetLocation() const
{
    if (ObservationPtr && !ObservationPtr->GetGameInfo().enemy_start_locations.empty())
    {
        return ObservationPtr->GetGameInfo().enemy_start_locations.front();
    }

    if (ObservationPtr)
    {
        return Point2D(ObservationPtr->GetStartLocation());
    }

    return Point2D();
}

}  // namespace sc2
