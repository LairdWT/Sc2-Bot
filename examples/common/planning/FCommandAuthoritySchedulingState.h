#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "common/planning/ECommandAuthorityLayer.h"
#include "common/planning/EIntentPlaybackState.h"
#include "common/planning/EOrderLifecycleState.h"
#include "common/planning/EPlanningProcessorState.h"
#include "common/planning/FCommandOrderRecord.h"

namespace sc2
{

enum class EIntentDomain : uint8_t;
enum class EIntentTargetKind : uint8_t;

struct FCommandAuthoritySchedulingState
{
public:
    FCommandAuthoritySchedulingState();

    void Reset();
    void Reserve(size_t OrderCapacityValue);
    uint32_t EnqueueOrder(const FCommandOrderRecord& CommandOrderRecordValue);
    bool IsOrderIndexValid(size_t OrderIndexValue) const;
    bool TryGetOrderIndex(uint32_t OrderIdValue, size_t& OutOrderIndexValue) const;
    size_t GetOrderCount() const;
    FCommandOrderRecord GetOrderRecord(size_t OrderIndexValue) const;
    bool SetOrderLifecycleState(uint32_t OrderIdValue, EOrderLifecycleState LifecycleStateValue);
    void RebuildDerivedQueues();

public:
    uint32_t NextOrderId;
    EPlanningProcessorState ProcessorState;
    EIntentPlaybackState PlaybackState;
    uint32_t MaxStrategicOrdersPerStep;
    uint32_t MaxArmyOrdersPerStep;
    uint32_t MaxSquadOrdersPerStep;
    uint32_t MaxUnitIntentsPerStep;

    std::vector<uint32_t> OrderIds;
    std::vector<uint32_t> ParentOrderIds;
    std::vector<ECommandAuthorityLayer> SourceLayers;
    std::vector<EOrderLifecycleState> LifecycleStates;
    std::vector<int> PriorityValues;
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

    std::unordered_map<uint32_t, size_t> OrderIdToIndex;
    std::vector<size_t> StrategicOrderIndices;
    std::vector<size_t> PlanningProcessIndices;
    std::vector<size_t> ArmyOrderIndices;
    std::vector<size_t> SquadOrderIndices;
    std::vector<size_t> ReadyIntentIndices;
    std::vector<size_t> CompletedOrderIndices;

private:
    void RebuildProcessorState();
    void RebuildPlaybackState();
    void AppendQueuedOrderIndex(size_t OrderIndexValue);
};

}  // namespace sc2
