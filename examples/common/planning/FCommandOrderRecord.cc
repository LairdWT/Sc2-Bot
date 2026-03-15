#include "common/planning/FCommandOrderRecord.h"

#include "common/agent_framework.h"

namespace sc2
{

FCommandOrderRecord::FCommandOrderRecord()
{
    Reset();
}

void FCommandOrderRecord::Reset()
{
    OrderId = 0U;
    ParentOrderId = 0U;
    SourceGoalId = 0U;
    SourceLayer = ECommandAuthorityLayer::Agent;
    LifecycleState = EOrderLifecycleState::Queued;
    TaskPackageKind = ECommandTaskPackageKind::Unknown;
    TaskNeedKind = ECommandTaskNeedKind::Unknown;
    TaskType = ECommandTaskType::Unknown;
    RetentionPolicy = ECommandTaskRetentionPolicy::BufferedRetry;
    BlockedTaskWakeKind = EBlockedTaskWakeKind::CooldownOnly;
    BasePriorityValue = 0;
    EffectivePriorityValue = 0;
    PriorityTier = ECommandPriorityTier::Normal;
    IntentDomain = EIntentDomain::Recovery;
    CreationStep = 0U;
    DeadlineStep = 0U;
    OwningArmyIndex = -1;
    OwningSquadIndex = -1;
    ActorTag = NullTag;
    AbilityId = ABILITY_ID::INVALID;
    TargetKind = EIntentTargetKind::None;
    TargetPoint = Point2D();
    TargetUnitTag = NullTag;
    Queued = false;
    RequiresPlacementValidation = false;
    RequiresPathingValidation = false;
    PlanStepId = 0U;
    TargetCount = 0U;
    RequestedQueueCount = 1U;
    ProducerUnitTypeId = UNIT_TYPEID::INVALID;
    ResultUnitTypeId = UNIT_TYPEID::INVALID;
    UpgradeId = UpgradeID(UPGRADE_ID::INVALID);
    PreferredPlacementSlotType = EBuildPlacementSlotType::Unknown;
    PreferredPlacementSlotId.Reset();
    ReservedPlacementSlotId.Reset();
    LastDeferralReason = ECommandOrderDeferralReason::None;
    LastDeferralStep = 0U;
    LastDeferralGameLoop = 0U;
    ConsecutiveDeferralCount = 0U;
    DispatchStep = 0U;
    DispatchGameLoop = 0U;
    ObservedCountAtDispatch = 0U;
    ObservedInConstructionCountAtDispatch = 0U;
    DispatchAttemptCount = 0U;
}

FCommandOrderRecord FCommandOrderRecord::CreateNoTarget(const ECommandAuthorityLayer SourceLayerValue,
                                                        const Tag ActorTagValue, const AbilityID AbilityIdValue,
                                                        const int BasePriorityValue, const EIntentDomain IntentDomainValue,
                                                        const uint64_t CreationStepValue,
                                                        const uint64_t DeadlineStepValue,
                                                        const uint32_t ParentOrderIdValue,
                                                        const int32_t OwningArmyIndexValue,
                                                        const int32_t OwningSquadIndexValue, const bool QueuedValue)
{
    FCommandOrderRecord CommandOrderRecordValue;
    CommandOrderRecordValue.SourceLayer = SourceLayerValue;
    CommandOrderRecordValue.ActorTag = ActorTagValue;
    CommandOrderRecordValue.AbilityId = AbilityIdValue;
    CommandOrderRecordValue.BasePriorityValue = BasePriorityValue;
    CommandOrderRecordValue.EffectivePriorityValue = BasePriorityValue;
    CommandOrderRecordValue.IntentDomain = IntentDomainValue;
    CommandOrderRecordValue.CreationStep = CreationStepValue;
    CommandOrderRecordValue.DeadlineStep = DeadlineStepValue;
    CommandOrderRecordValue.ParentOrderId = ParentOrderIdValue;
    CommandOrderRecordValue.OwningArmyIndex = OwningArmyIndexValue;
    CommandOrderRecordValue.OwningSquadIndex = OwningSquadIndexValue;
    CommandOrderRecordValue.Queued = QueuedValue;
    return CommandOrderRecordValue;
}

FCommandOrderRecord FCommandOrderRecord::CreatePointTarget(
    const ECommandAuthorityLayer SourceLayerValue, const Tag ActorTagValue, const AbilityID AbilityIdValue,
    const Point2D& TargetPointValue, const int BasePriorityValue, const EIntentDomain IntentDomainValue,
    const uint64_t CreationStepValue, const uint64_t DeadlineStepValue, const uint32_t ParentOrderIdValue,
    const int32_t OwningArmyIndexValue, const int32_t OwningSquadIndexValue,
    const bool RequiresPathingValidationValue, const bool RequiresPlacementValidationValue, const bool QueuedValue)
{
    FCommandOrderRecord CommandOrderRecordValue = CreateNoTarget(SourceLayerValue, ActorTagValue, AbilityIdValue,
                                                                 BasePriorityValue, IntentDomainValue, CreationStepValue,
                                                                 DeadlineStepValue, ParentOrderIdValue,
                                                                 OwningArmyIndexValue, OwningSquadIndexValue,
                                                                 QueuedValue);
    CommandOrderRecordValue.TargetKind = EIntentTargetKind::Point;
    CommandOrderRecordValue.TargetPoint = TargetPointValue;
    CommandOrderRecordValue.RequiresPathingValidation = RequiresPathingValidationValue;
    CommandOrderRecordValue.RequiresPlacementValidation = RequiresPlacementValidationValue;
    return CommandOrderRecordValue;
}

FCommandOrderRecord FCommandOrderRecord::CreateUnitTarget(const ECommandAuthorityLayer SourceLayerValue,
                                                          const Tag ActorTagValue, const AbilityID AbilityIdValue,
                                                          const Tag TargetUnitTagValue, const int BasePriorityValue,
                                                          const EIntentDomain IntentDomainValue,
                                                          const uint64_t CreationStepValue,
                                                          const uint64_t DeadlineStepValue,
                                                          const uint32_t ParentOrderIdValue,
                                                          const int32_t OwningArmyIndexValue,
                                                          const int32_t OwningSquadIndexValue, const bool QueuedValue)
{
    FCommandOrderRecord CommandOrderRecordValue = CreateNoTarget(SourceLayerValue, ActorTagValue, AbilityIdValue,
                                                                 BasePriorityValue, IntentDomainValue, CreationStepValue,
                                                                 DeadlineStepValue, ParentOrderIdValue,
                                                                 OwningArmyIndexValue, OwningSquadIndexValue,
                                                                 QueuedValue);
    CommandOrderRecordValue.TargetKind = EIntentTargetKind::Unit;
    CommandOrderRecordValue.TargetUnitTag = TargetUnitTagValue;
    return CommandOrderRecordValue;
}

}  // namespace sc2
