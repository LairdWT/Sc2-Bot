#include "common/planning/FTerranSquadOrderExpander.h"

#include "common/armies/FArmyMissionDescriptor.h"

namespace sc2
{
namespace
{

const FArmyMissionDescriptor* GetMissionDescriptorForArmyIndex(const FGameStateDescriptor& GameStateDescriptorValue,
                                                               const int32_t OwningArmyIndexValue)
{
    if (OwningArmyIndexValue < 0)
    {
        return nullptr;
    }

    const size_t ArmyIndexValue = static_cast<size_t>(OwningArmyIndexValue);
    if (ArmyIndexValue >= GameStateDescriptorValue.ArmyState.ArmyMissions.size())
    {
        return nullptr;
    }

    return &GameStateDescriptorValue.ArmyState.ArmyMissions[ArmyIndexValue];
}

}  // namespace

void FTerranSquadOrderExpander::ExpandSquadOrders(
    const FFrameContext& FrameValue, const FAgentState& AgentStateValue,
    const FGameStateDescriptor& GameStateDescriptorValue, const Point2D& RallyPointValue,
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue) const
{
    (void)AgentStateValue;

    uint32_t ExpandedSquadOrderCountValue = 0U;
    const std::vector<size_t> ArmyOrderIndicesValue = CommandAuthoritySchedulingStateValue.ArmyOrderIndices;
    for (const size_t ArmyOrderIndexValue : ArmyOrderIndicesValue)
    {
        if (ExpandedSquadOrderCountValue >= CommandAuthoritySchedulingStateValue.MaxSquadOrdersPerStep)
        {
            break;
        }

        const FCommandOrderRecord ArmyOrderValue = CommandAuthoritySchedulingStateValue.GetOrderRecord(ArmyOrderIndexValue);
        if (ArmyOrderValue.SourceLayer != ECommandAuthorityLayer::Army ||
            IsTerminalLifecycleState(ArmyOrderValue.LifecycleState))
        {
            continue;
        }

        size_t ExistingSquadOrderIndexValue = 0U;
        if (CommandAuthoritySchedulingStateValue.TryGetActiveChildOrderIndex(
                ArmyOrderValue.OrderId, ECommandAuthorityLayer::Squad, ExistingSquadOrderIndexValue))
        {
            continue;
        }

        const FArmyMissionDescriptor* MissionDescriptorPtrValue =
            GetMissionDescriptorForArmyIndex(GameStateDescriptorValue, ArmyOrderValue.OwningArmyIndex);
        const Point2D ObjectivePointValue =
            MissionDescriptorPtrValue != nullptr ? MissionDescriptorPtrValue->ObjectivePoint : RallyPointValue;

        FCommandOrderRecord SquadOrderValue = FCommandOrderRecord::CreatePointTarget(
            ECommandAuthorityLayer::Squad, NullTag, ABILITY_ID::INVALID, ObjectivePointValue,
            ArmyOrderValue.BasePriorityValue, EIntentDomain::ArmyCombat, FrameValue.GameLoop, 0U, ArmyOrderValue.OrderId,
            ArmyOrderValue.OwningArmyIndex, 0);
        SquadOrderValue.SourceGoalId = ArmyOrderValue.SourceGoalId;
        SquadOrderValue.TaskPackageKind = ArmyOrderValue.TaskPackageKind;
        SquadOrderValue.TaskNeedKind = ArmyOrderValue.TaskNeedKind;
        SquadOrderValue.TaskType = ArmyOrderValue.TaskType;
        SquadOrderValue.EffectivePriorityValue = ArmyOrderValue.EffectivePriorityValue;
        SquadOrderValue.PriorityTier = ArmyOrderValue.PriorityTier;

        CommandAuthoritySchedulingStateValue.EnqueueOrder(SquadOrderValue);
        CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(ArmyOrderValue.OrderId,
                                                                   EOrderLifecycleState::Completed);
        ++ExpandedSquadOrderCountValue;
    }
}

}  // namespace sc2
