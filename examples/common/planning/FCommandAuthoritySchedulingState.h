#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "common/planning/ECommandAuthorityLayer.h"
#include "common/planning/ECommandPriorityTier.h"
#include "common/planning/ECommandOrderDeferralReason.h"
#include "common/planning/EIntentDomain.h"
#include "common/planning/EIntentPlaybackState.h"
#include "common/planning/EOrderLifecycleState.h"
#include "common/planning/EPlanningProcessorState.h"
#include "common/planning/FCommandOrderRecord.h"
#include "common/services/FBuildPlacementSlotId.h"

namespace sc2
{

enum class EIntentTargetKind : uint8_t;

struct FCommandAuthoritySchedulingState
{
public:
    FCommandAuthoritySchedulingState();

    void Reset();
    void Reserve(size_t OrderCapacityValue);
    void BeginMutationBatch();
    void EndMutationBatch();
    uint32_t EnqueueOrder(const FCommandOrderRecord& CommandOrderRecordValue);
    bool IsOrderIndexValid(size_t OrderIndexValue) const;
    bool TryGetOrderIndex(uint32_t OrderIdValue, size_t& OutOrderIndexValue) const;
    bool TryGetChildOrderIndex(uint32_t ParentOrderIdValue, ECommandAuthorityLayer SourceLayerValue,
                               size_t& OutOrderIndexValue) const;
    bool TryGetActiveChildOrderIndex(uint32_t ParentOrderIdValue, ECommandAuthorityLayer SourceLayerValue,
                                     size_t& OutOrderIndexValue) const;
    size_t GetOrderCount() const;
    FCommandOrderRecord GetOrderRecord(size_t OrderIndexValue) const;
    bool SetOrderLifecycleState(uint32_t OrderIdValue, EOrderLifecycleState LifecycleStateValue);
    bool SetOrderDeferralState(uint32_t OrderIdValue, ECommandOrderDeferralReason DeferralReasonValue,
                               uint64_t CurrentStepValue, uint64_t CurrentGameLoopValue);
    bool ClearOrderDeferralState(uint32_t OrderIdValue);
    bool SetOrderDispatchState(uint32_t OrderIdValue, uint64_t DispatchStepValue, uint64_t DispatchGameLoopValue,
                               uint32_t ObservedCountValue, uint32_t ObservedInConstructionCountValue);
    bool SetOrderReservedPlacementSlot(uint32_t OrderIdValue, const FBuildPlacementSlotId& BuildPlacementSlotIdValue);
    bool ClearOrderReservedPlacementSlot(uint32_t OrderIdValue);
    bool CompactTerminalOrders();
    void RebuildDerivedQueues();

public:
    uint32_t NextOrderId;
    EPlanningProcessorState ProcessorState;
    EIntentPlaybackState PlaybackState;
    uint32_t MaxStrategicOrdersPerStep;
    uint32_t MaxArmyOrdersPerStep;
    uint32_t MaxSquadOrdersPerStep;
    uint32_t MaxUnitIntentsPerStep;
    uint32_t MutationBatchDepth;
    bool bDerivedQueuesDirty;

    std::vector<uint32_t> OrderIds;
    std::vector<uint32_t> ParentOrderIds;
    std::vector<uint32_t> SourceGoalIds;
    std::vector<ECommandAuthorityLayer> SourceLayers;
    std::vector<EOrderLifecycleState> LifecycleStates;
    std::vector<ECommandTaskPackageKind> TaskPackageKinds;
    std::vector<ECommandTaskNeedKind> TaskNeedKinds;
    std::vector<ECommandTaskType> TaskTypes;
    std::vector<int> BasePriorityValues;
    std::vector<int> EffectivePriorityValues;
    std::vector<ECommandPriorityTier> PriorityTiers;
    std::vector<EIntentDomain> IntentDomains;
    std::vector<uint64_t> CreationSteps;
    std::vector<uint64_t> DeadlineSteps;
    std::vector<int32_t> OwningArmyIndices;
    std::vector<int32_t> OwningSquadIndices;
    std::vector<Tag> ActorTags;
    std::vector<AbilityID> AbilityIds;
    std::vector<EIntentTargetKind> TargetKinds;
    std::vector<Point2D> TargetPoints;
    std::vector<Tag> TargetUnitTags;
    std::vector<bool> QueuedValues;
    std::vector<bool> RequiresPlacementValidationValues;
    std::vector<bool> RequiresPathingValidationValues;
    std::vector<uint32_t> PlanStepIds;
    std::vector<uint32_t> TargetCounts;
    std::vector<uint32_t> RequestedQueueCounts;
    std::vector<UNIT_TYPEID> ProducerUnitTypeIds;
    std::vector<UNIT_TYPEID> ResultUnitTypeIds;
    std::vector<UpgradeID> UpgradeIds;
    std::vector<EBuildPlacementSlotType> PreferredPlacementSlotTypes;
    std::vector<EBuildPlacementSlotType> PreferredPlacementSlotIdTypes;
    std::vector<uint8_t> PreferredPlacementSlotIdOrdinals;
    std::vector<EBuildPlacementSlotType> ReservedPlacementSlotTypes;
    std::vector<uint8_t> ReservedPlacementSlotOrdinals;
    std::vector<ECommandOrderDeferralReason> LastDeferralReasons;
    std::vector<uint64_t> LastDeferralSteps;
    std::vector<uint64_t> LastDeferralGameLoops;
    std::vector<uint64_t> DispatchSteps;
    std::vector<uint64_t> DispatchGameLoops;
    std::vector<uint32_t> ObservedCountsAtDispatch;
    std::vector<uint32_t> ObservedInConstructionCountsAtDispatch;
    std::vector<uint32_t> DispatchAttemptCounts;

    std::unordered_map<uint32_t, size_t> OrderIdToIndex;
    std::vector<size_t> StrategicOrderIndices;
    std::vector<size_t> PlanningProcessIndices;
    std::vector<size_t> ArmyOrderIndices;
    std::vector<size_t> SquadOrderIndices;
    std::vector<size_t> ReadyIntentIndices;
    std::vector<size_t> CompletedOrderIndices;
    std::array<std::vector<size_t>, CommandPriorityTierCountValue> StrategicQueues;
    std::array<std::vector<size_t>, CommandPriorityTierCountValue> PlanningQueues;
    std::array<std::vector<size_t>, CommandPriorityTierCountValue> ArmyQueues;
    std::array<std::vector<size_t>, CommandPriorityTierCountValue> SquadQueues;
    std::array<std::array<std::vector<size_t>, IntentDomainCountValue>, CommandPriorityTierCountValue>
        ReadyIntentQueues;

private:
    void MarkDerivedQueuesDirty();
    void RebuildProcessorState();
    void RebuildPlaybackState();
    void AppendQueuedOrderIndex(size_t OrderIndexValue);
    void SortDerivedQueues();
};

}  // namespace sc2
