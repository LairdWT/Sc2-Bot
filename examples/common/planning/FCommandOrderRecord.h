#pragma once

#include <cstdint>

#include "common/planning/ECommandAuthorityLayer.h"
#include "common/planning/ECommandOrderDeferralReason.h"
#include "common/planning/EIntentDomain.h"
#include "common/planning/EOrderLifecycleState.h"
#include "sc2api/sc2_common.h"
#include "sc2api/sc2_gametypes.h"
#include "sc2api/sc2_typeenums.h"

namespace sc2
{

enum class EIntentTargetKind : uint8_t;

struct FCommandOrderRecord
{
public:
    FCommandOrderRecord();

    void Reset();

    static FCommandOrderRecord CreateNoTarget(ECommandAuthorityLayer SourceLayerValue, Tag ActorTagValue,
                                              AbilityID AbilityIdValue, int PriorityValue, EIntentDomain IntentDomainValue,
                                              uint64_t CreationStepValue, uint64_t DeadlineStepValue = 0U,
                                              uint32_t ParentOrderIdValue = 0U, int32_t OwningArmyIndexValue = -1,
                                              int32_t OwningSquadIndexValue = -1, bool QueuedValue = false);

    static FCommandOrderRecord CreatePointTarget(ECommandAuthorityLayer SourceLayerValue, Tag ActorTagValue,
                                                 AbilityID AbilityIdValue, const Point2D& TargetPointValue,
                                                 int PriorityValue, EIntentDomain IntentDomainValue,
                                                 uint64_t CreationStepValue, uint64_t DeadlineStepValue = 0U,
                                                 uint32_t ParentOrderIdValue = 0U, int32_t OwningArmyIndexValue = -1,
                                                 int32_t OwningSquadIndexValue = -1,
                                                 bool RequiresPathingValidationValue = false,
                                                 bool RequiresPlacementValidationValue = false,
                                                 bool QueuedValue = false);

    static FCommandOrderRecord CreateUnitTarget(ECommandAuthorityLayer SourceLayerValue, Tag ActorTagValue,
                                                AbilityID AbilityIdValue, Tag TargetUnitTagValue, int PriorityValue,
                                                EIntentDomain IntentDomainValue, uint64_t CreationStepValue,
                                                uint64_t DeadlineStepValue = 0U, uint32_t ParentOrderIdValue = 0U,
                                                int32_t OwningArmyIndexValue = -1,
                                                int32_t OwningSquadIndexValue = -1, bool QueuedValue = false);

public:
    uint32_t OrderId;
    uint32_t ParentOrderId;
    ECommandAuthorityLayer SourceLayer;
    EOrderLifecycleState LifecycleState;
    int PriorityValue;
    EIntentDomain IntentDomain;
    uint64_t CreationStep;
    uint64_t DeadlineStep;
    int32_t OwningArmyIndex;
    int32_t OwningSquadIndex;
    Tag ActorTag;
    AbilityID AbilityId;
    EIntentTargetKind TargetKind;
    Point2D TargetPoint;
    Tag TargetUnitTag;
    bool Queued;
    bool RequiresPlacementValidation;
    bool RequiresPathingValidation;
    uint32_t PlanStepId;
    uint32_t TargetCount;
    UNIT_TYPEID ProducerUnitTypeId;
    UNIT_TYPEID ResultUnitTypeId;
    UpgradeID UpgradeId;
    ECommandOrderDeferralReason LastDeferralReason;
    uint64_t LastDeferralStep;
    uint64_t LastDeferralGameLoop;
    uint64_t DispatchStep;
    uint64_t DispatchGameLoop;
    uint32_t ObservedCountAtDispatch;
    uint32_t ObservedInConstructionCountAtDispatch;
    uint32_t DispatchAttemptCount;
};

}  // namespace sc2
