#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "common/planning/EBlockedTaskWakeKind.h"
#include "common/planning/ECommandAuthorityLayer.h"
#include "common/planning/ECommandCommitmentClass.h"
#include "common/planning/ECommandPriorityTier.h"
#include "common/planning/ECommandOrderDeferralReason.h"
#include "common/planning/ECommandTaskOrigin.h"
#include "common/planning/ECommandTaskExecutionGuarantee.h"
#include "common/planning/ECommandTaskRetentionPolicy.h"
#include "common/planning/FCommandTaskSignatureKey.h"
#include "common/planning/FBlockedTaskRingBuffer.h"
#include "common/planning/EIntentDomain.h"
#include "common/planning/EIntentPlaybackState.h"
#include "common/planning/EOrderLifecycleState.h"
#include "common/planning/EPlanningProcessorState.h"
#include "common/planning/FCommandOrderRecord.h"
#include "common/planning/FSchedulerStimulusState.h"
#include "common/services/FBuildPlacementSlotId.h"

namespace sc2
{

enum class EIntentTargetKind : uint8_t;
struct FOpeningPlanExecutionState;

struct FCommandAuthoritySchedulingState
{
public:
    FCommandAuthoritySchedulingState();

    void Reset();
    void Reserve(size_t OrderCapacityValue);
    void AdvanceRecentBlockedTaskCounterWindow(uint64_t CurrentStepValue);
    void BeginMutationBatch();
    void EndMutationBatch();
    uint32_t EnqueueOrder(const FCommandOrderRecord& CommandOrderRecordValue);
    bool IsOrderIndexValid(size_t OrderIndexValue) const;
    bool TryGetOrderIndex(uint32_t OrderIdValue, size_t& OutOrderIndexValue) const;
    bool TryGetChildOrderIndex(uint32_t ParentOrderIdValue, ECommandAuthorityLayer SourceLayerValue,
                               size_t& OutOrderIndexValue) const;
    bool TryGetActiveChildOrderIndex(uint32_t ParentOrderIdValue, ECommandAuthorityLayer SourceLayerValue,
                                     size_t& OutOrderIndexValue) const;
    bool TryGetActiveExecutionOrderIndexForActor(Tag ActorTagValue, size_t& OutOrderIndexValue) const;
    size_t GetOrderCount() const;
    FCommandOrderRecord GetOrderRecord(size_t OrderIndexValue) const;
    bool HasActiveStrategicOrderForGoalId(uint32_t SourceGoalIdValue) const;
    bool HasEquivalentActiveTaskSignature(const FCommandTaskSignatureKey& CommandTaskSignatureKeyValue) const;
    bool SetOrderLifecycleState(uint32_t OrderIdValue, EOrderLifecycleState LifecycleStateValue);
    bool SetOrderDeferralState(uint32_t OrderIdValue, ECommandOrderDeferralReason DeferralReasonValue,
                               uint64_t CurrentStepValue, uint64_t CurrentGameLoopValue);
    bool ClearOrderDeferralState(uint32_t OrderIdValue);
    bool SetOrderDispatchState(uint32_t OrderIdValue, uint64_t DispatchStepValue, uint64_t DispatchGameLoopValue,
                               uint32_t ObservedCountValue, uint32_t ObservedInConstructionCountValue);
    bool SetOrderReservedPlacementSlot(uint32_t OrderIdValue, const FBuildPlacementSlotId& BuildPlacementSlotIdValue);
    bool ClearOrderReservedPlacementSlot(uint32_t OrderIdValue);
    bool CompactTerminalOrders(const FOpeningPlanExecutionState* OpeningPlanExecutionStatePtrValue = nullptr);
    void RebuildDerivedQueues();
    size_t GetActiveOrderCountForLayer(ECommandAuthorityLayer SourceLayerValue) const;

public:
    uint32_t NextOrderId;
    EPlanningProcessorState ProcessorState;
    EIntentPlaybackState PlaybackState;
    uint32_t MaxStrategicOrdersPerStep;
    uint32_t MaxArmyOrdersPerStep;
    uint32_t MaxSquadOrdersPerStep;
    uint32_t MaxUnitIntentsPerStep;
    uint32_t MaxActiveStrategicOrders;
    uint32_t MaxActivePlanningOrders;
    uint32_t MaxActiveUnitExecutionOrders;
    uint32_t MaxBlockedStrategicTasks;
    uint32_t MaxBlockedPlanningTasks;
    uint32_t MutationBatchDepth;
    uint32_t RejectedUnitExecutionAdmissionCount;
    uint32_t SupersededUnitExecutionOrderCount;
    uint64_t RecentBlockedTaskCounterWindowStartStep;
    uint32_t TotalBufferedBlockedTaskCount;
    uint32_t TotalCoalescedBlockedTaskCount;
    uint32_t TotalDroppedBlockedTaskCount;
    uint32_t TotalReactivatedBlockedTaskCount;
    uint32_t TotalRejectedMustRunBlockedTaskCount;
    uint32_t RecentBufferedBlockedTaskCount;
    uint32_t RecentCoalescedBlockedTaskCount;
    uint32_t RecentDroppedBlockedTaskCount;
    uint32_t RecentReactivatedBlockedTaskCount;
    uint32_t RecentRejectedMustRunBlockedTaskCount;
    bool bDerivedQueuesDirty;
    bool bPrioritiesDirty;
    FSchedulerStimulusState SchedulerStimulusState;

    std::vector<uint32_t> OrderIds;
    std::vector<uint32_t> ParentOrderIds;
    std::vector<uint32_t> SourceGoalIds;
    std::vector<ECommandAuthorityLayer> SourceLayers;
    std::vector<EOrderLifecycleState> LifecycleStates;
    std::vector<ECommandTaskPackageKind> TaskPackageKinds;
    std::vector<ECommandTaskNeedKind> TaskNeedKinds;
    std::vector<ECommandTaskType> TaskTypes;
    std::vector<ECommandTaskOrigin> TaskOrigins;
    std::vector<ECommandCommitmentClass> CommitmentClasses;
    std::vector<ECommandTaskExecutionGuarantee> ExecutionGuarantees;
    std::vector<ECommandTaskRetentionPolicy> RetentionPolicies;
    std::vector<EBlockedTaskWakeKind> BlockedTaskWakeKinds;
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
    std::vector<EBuildPlacementSlotType> PreferredProducerPlacementSlotIdTypes;
    std::vector<uint8_t> PreferredProducerPlacementSlotIdOrdinals;
    std::vector<EBuildPlacementSlotType> ReservedPlacementSlotTypes;
    std::vector<uint8_t> ReservedPlacementSlotOrdinals;
    std::vector<ECommandOrderDeferralReason> LastDeferralReasons;
    std::vector<uint64_t> LastDeferralSteps;
    std::vector<uint64_t> LastDeferralGameLoops;
    std::vector<uint32_t> ConsecutiveDeferralCounts;
    std::vector<uint64_t> DispatchSteps;
    std::vector<uint64_t> DispatchGameLoops;
    std::vector<uint32_t> ObservedCountsAtDispatch;
    std::vector<uint32_t> ObservedInConstructionCountsAtDispatch;
    std::vector<uint32_t> DispatchAttemptCounts;

    std::unordered_map<uint32_t, size_t> OrderIdToIndex;
    std::unordered_map<Tag, size_t> ActiveExecutionOrderIndexByActorTag;
    std::unordered_map<uint32_t, size_t> ActiveStrategicOrderIndexByGoalId;
    std::unordered_map<FCommandTaskSignatureKey, uint32_t, FCommandTaskSignatureKeyHash> ActiveTaskSignatureCounts;
    std::unordered_map<uint64_t, size_t> ActiveChildOrderIndexByParentAndLayer;
    std::array<uint32_t, 6U> ActiveOrderCountsByLayer;
    std::vector<size_t> StrategicOrderIndices;
    std::vector<size_t> PlanningProcessIndices;
    std::vector<size_t> ArmyOrderIndices;
    std::vector<size_t> SquadOrderIndices;
    std::vector<size_t> ReadyIntentIndices;
    std::vector<size_t> DispatchedOrderIndices;
    std::vector<size_t> CompletedOrderIndices;
    std::array<std::vector<size_t>, CommandPriorityTierCountValue> StrategicQueues;
    std::array<std::vector<size_t>, CommandPriorityTierCountValue> PlanningQueues;
    std::array<std::vector<size_t>, CommandPriorityTierCountValue> ArmyQueues;
    std::array<std::vector<size_t>, CommandPriorityTierCountValue> SquadQueues;
    std::array<std::array<std::vector<size_t>, IntentDomainCountValue>, CommandPriorityTierCountValue>
        ReadyIntentQueues;
    FBlockedTaskRingBuffer BlockedStrategicTasks;
    FBlockedTaskRingBuffer BlockedPlanningTasks;

private:
    void MarkDerivedQueuesDirty();
    void RebuildProcessorState();
    void RebuildPlaybackState();
    void AppendQueuedOrderIndex(size_t OrderIndexValue);
    void SortDerivedQueues();
};

}  // namespace sc2
