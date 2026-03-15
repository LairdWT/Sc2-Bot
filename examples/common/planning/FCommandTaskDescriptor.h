#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "common/planning/EBlockedTaskWakeKind.h"
#include "common/planning/ECommandCommitmentClass.h"
#include "common/planning/ECommandTaskOrigin.h"
#include "common/planning/ECommandTaskExecutionGuarantee.h"
#include "common/planning/ECommandTaskRetentionPolicy.h"
#include "common/planning/ECommandTaskType.h"
#include "common/services/FBuildPlacementSlotId.h"
#include "sc2api/sc2_gametypes.h"
#include "sc2api/sc2_typeenums.h"

namespace sc2
{

enum class ECommandTaskPackageKind : uint8_t
{
    Unknown,
    Opening,
    Expansion,
    TechTransition,
    TimingAttack,
    Recovery,
    Macro,
    Supply,
    Defense,
    ProductionScale
};

enum class ECommandTaskNeedKind : uint8_t
{
    Unknown,
    Structure,
    AddOn,
    Expansion,
    Unit,
    Upgrade
};

enum class ECommandTaskActionKind : uint8_t
{
    Unknown,
    BuildStructure,
    BuildAddon,
    Expand,
    MorphStructure,
    TrainUnit,
    ResearchUpgrade
};

enum class ECommandTaskCompletionKind : uint8_t
{
    Unknown,
    CountAtLeast
};

struct FCommandTaskDescriptor
{
public:
    FCommandTaskDescriptor();

    void Reset();

public:
    uint32_t TaskId;
    ECommandTaskPackageKind PackageKind;
    ECommandTaskNeedKind NeedKind;
    ECommandTaskActionKind ActionKind;
    ECommandTaskCompletionKind CompletionKind;
    ECommandTaskType TaskType;
    ECommandTaskOrigin Origin;
    ECommandCommitmentClass CommitmentClass;
    ECommandTaskExecutionGuarantee ExecutionGuarantee;
    ECommandTaskRetentionPolicy RetentionPolicy;
    EBlockedTaskWakeKind BlockedTaskWakeKind;
    int BasePriorityValue;
    uint32_t SourceGoalId;
    uint64_t TriggerMinGameLoop;
    uint32_t TriggerMinSupplyUsed;
    uint32_t TriggerMaxSupplyUsed;
    uint64_t TriggerRetryCadenceGameLoops;
    std::string TriggerReferenceClockTime;
    std::vector<uint32_t> TriggerRequiredCompletedTaskIds;
    AbilityID ActionAbilityId;
    UNIT_TYPEID ActionProducerUnitTypeId;
    UNIT_TYPEID ActionResultUnitTypeId;
    UpgradeID ActionUpgradeId;
    uint32_t ActionTargetCount;
    uint32_t ActionRequestedQueueCount;
    uint32_t ParallelGroupId;
    EBuildPlacementSlotType ActionPreferredPlacementSlotType;
    FBuildPlacementSlotId ActionPreferredPlacementSlotId;
    uint32_t CompletionObservedCountAtLeast;
    uint64_t CompletionExpectedCompleteByGameLoop;
};

}  // namespace sc2
