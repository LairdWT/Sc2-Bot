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
    SourceLayer = ECommandAuthorityLayer::Agent;
    LifecycleState = EOrderLifecycleState::Queued;
    PriorityValue = 0;
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
    ProducerUnitTypeId = UNIT_TYPEID::INVALID;
    ResultUnitTypeId = UNIT_TYPEID::INVALID;
    UpgradeId = UpgradeID(UPGRADE_ID::INVALID);
    LastDeferralReason = ECommandOrderDeferralReason::None;
    LastDeferralStep = 0U;
    LastDeferralGameLoop = 0U;
    DispatchStep = 0U;
    DispatchGameLoop = 0U;
    ObservedCountAtDispatch = 0U;
    ObservedInConstructionCountAtDispatch = 0U;
    DispatchAttemptCount = 0U;
}

FCommandOrderRecord FCommandOrderRecord::CreateNoTarget(const ECommandAuthorityLayer SourceLayerValue,
                                                        const Tag ActorTagValue, const AbilityID AbilityIdValue,
                                                        const int PriorityValue, const EIntentDomain IntentDomainValue,
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
    CommandOrderRecordValue.PriorityValue = PriorityValue;
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
    const Point2D& TargetPointValue, const int PriorityValue, const EIntentDomain IntentDomainValue,
    const uint64_t CreationStepValue, const uint64_t DeadlineStepValue, const uint32_t ParentOrderIdValue,
    const int32_t OwningArmyIndexValue, const int32_t OwningSquadIndexValue,
    const bool RequiresPathingValidationValue, const bool RequiresPlacementValidationValue, const bool QueuedValue)
{
    FCommandOrderRecord CommandOrderRecordValue = CreateNoTarget(SourceLayerValue, ActorTagValue, AbilityIdValue,
                                                                 PriorityValue, IntentDomainValue, CreationStepValue,
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
                                                          const Tag TargetUnitTagValue, const int PriorityValue,
                                                          const EIntentDomain IntentDomainValue,
                                                          const uint64_t CreationStepValue,
                                                          const uint64_t DeadlineStepValue,
                                                          const uint32_t ParentOrderIdValue,
                                                          const int32_t OwningArmyIndexValue,
                                                          const int32_t OwningSquadIndexValue, const bool QueuedValue)
{
    FCommandOrderRecord CommandOrderRecordValue = CreateNoTarget(SourceLayerValue, ActorTagValue, AbilityIdValue,
                                                                 PriorityValue, IntentDomainValue, CreationStepValue,
                                                                 DeadlineStepValue, ParentOrderIdValue,
                                                                 OwningArmyIndexValue, OwningSquadIndexValue,
                                                                 QueuedValue);
    CommandOrderRecordValue.TargetKind = EIntentTargetKind::Unit;
    CommandOrderRecordValue.TargetUnitTag = TargetUnitTagValue;
    return CommandOrderRecordValue;
}

}  // namespace sc2
