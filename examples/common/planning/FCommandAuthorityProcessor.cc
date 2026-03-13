#include "common/planning/FCommandAuthorityProcessor.h"

#include "common/build_orders/FOpeningPlanRegistry.h"

namespace sc2
{
namespace
{

constexpr uint32_t WorkerGoalStepIdValue = 9000U;

EIntentDomain DetermineIntentDomain(const FCommandOrderRecord& CommandOrderRecordValue)
{
    switch (CommandOrderRecordValue.AbilityId.ToType())
    {
        case ABILITY_ID::TRAIN_SCV:
        case ABILITY_ID::TRAIN_MARINE:
        case ABILITY_ID::TRAIN_HELLION:
        case ABILITY_ID::TRAIN_CYCLONE:
        case ABILITY_ID::TRAIN_MEDIVAC:
        case ABILITY_ID::TRAIN_LIBERATOR:
        case ABILITY_ID::TRAIN_SIEGETANK:
            return EIntentDomain::UnitProduction;
        default:
            return EIntentDomain::StructureBuild;
    }
}

uint32_t GetBuildingCount(const std::array<uint16_t, NUM_TERRAN_BUILDINGS>& BuildingCountsValue,
                          const UNIT_TYPEID BuildingTypeIdValue)
{
    switch (BuildingTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
            return 0U;
        case UNIT_TYPEID::TERRAN_SUPPLYDEPOT:
            return static_cast<uint32_t>(BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_SUPPLYDEPOT)]) +
                   static_cast<uint32_t>(BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_SUPPLYDEPOTLOWERED)]);
        case UNIT_TYPEID::TERRAN_BARRACKS:
            return static_cast<uint32_t>(BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_BARRACKS)]) +
                   static_cast<uint32_t>(BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_BARRACKSFLYING)]);
        case UNIT_TYPEID::TERRAN_FACTORY:
            return static_cast<uint32_t>(BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_FACTORY)]) +
                   static_cast<uint32_t>(BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_FACTORYFLYING)]);
        case UNIT_TYPEID::TERRAN_STARPORT:
            return static_cast<uint32_t>(BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_STARPORT)]) +
                   static_cast<uint32_t>(BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_STARPORTFLYING)]);
        case UNIT_TYPEID::TERRAN_REFINERY:
            return static_cast<uint32_t>(BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_REFINERY)]) +
                   static_cast<uint32_t>(BuildingCountsValue[GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_REFINERYRICH)]);
        default:
        {
            const size_t BuildingTypeIndexValue = GetTerranBuildingTypeIndex(BuildingTypeIdValue);
            return IsTerranBuildingTypeIndexValid(BuildingTypeIndexValue)
                       ? static_cast<uint32_t>(BuildingCountsValue[BuildingTypeIndexValue])
                       : 0U;
        }
    }
}

uint32_t GetUnitCount(const std::array<uint16_t, NUM_TERRAN_UNITS>& UnitCountsValue, const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_HELLION:
            return static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_HELLION)]) +
                   static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_HELLIONTANK)]);
        case UNIT_TYPEID::TERRAN_LIBERATOR:
            return static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_LIBERATOR)]) +
                   static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_LIBERATORAG)]);
        case UNIT_TYPEID::TERRAN_SIEGETANK:
            return static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_SIEGETANK)]) +
                   static_cast<uint32_t>(UnitCountsValue[GetTerranUnitTypeIndex(UNIT_TYPEID::TERRAN_SIEGETANKSIEGED)]);
        default:
        {
            const size_t UnitTypeIndexValue = GetTerranUnitTypeIndex(UnitTypeIdValue);
            return IsTerranUnitTypeIndexValid(UnitTypeIndexValue) ? static_cast<uint32_t>(UnitCountsValue[UnitTypeIndexValue])
                                                                  : 0U;
        }
    }
}

}  // namespace

void FCommandAuthorityProcessor::ProcessSchedulerStep(FGameStateDescriptor& GameStateDescriptorValue) const
{
    InitializeOpeningPlan(GameStateDescriptorValue);
    UpdateCompletedOpeningSteps(GameStateDescriptorValue);
    if (GameStateDescriptorValue.OpeningPlanExecutionState.LifecycleState == EOpeningPlanLifecycleState::Completed)
    {
        return;
    }

    SeedReadyStrategicOrders(GameStateDescriptorValue);
    EnsureWorkerGoalOrder(GameStateDescriptorValue);
    EnsureStrategicChildOrders(GameStateDescriptorValue);
    UpdateCompletedOpeningSteps(GameStateDescriptorValue);
}

void FCommandAuthorityProcessor::InitializeOpeningPlan(FGameStateDescriptor& GameStateDescriptorValue) const
{
    FOpeningPlanExecutionState& OpeningPlanExecutionStateValue = GameStateDescriptorValue.OpeningPlanExecutionState;
    if (OpeningPlanExecutionStateValue.LifecycleState != EOpeningPlanLifecycleState::Uninitialized)
    {
        return;
    }

    OpeningPlanExecutionStateValue.SetActivePlan(EOpeningPlanId::TerranTwoBaseMMMFrameOpening);
}

void FCommandAuthorityProcessor::UpdateCompletedOpeningSteps(FGameStateDescriptor& GameStateDescriptorValue) const
{
    FOpeningPlanExecutionState& OpeningPlanExecutionStateValue = GameStateDescriptorValue.OpeningPlanExecutionState;
    const FOpeningPlanDescriptor& OpeningPlanDescriptorValue =
        FOpeningPlanRegistry::GetOpeningPlanDescriptor(OpeningPlanExecutionStateValue.ActivePlanId);

    for (const FOpeningPlanStep& OpeningPlanStepValue : OpeningPlanDescriptorValue.Steps)
    {
        if (OpeningPlanExecutionStateValue.IsStepCompleted(OpeningPlanStepValue.StepId))
        {
            continue;
        }

        FCommandOrderRecord CompletionProbeValue;
        CompletionProbeValue.ResultUnitTypeId = OpeningPlanStepValue.ResultUnitTypeId;
        CompletionProbeValue.TargetCount = OpeningPlanStepValue.TargetCount;
        if (!DoesOrderTargetMatchObservedState(GameStateDescriptorValue.BuildPlanning, CompletionProbeValue))
        {
            continue;
        }

        OpeningPlanExecutionStateValue.MarkStepCompleted(OpeningPlanStepValue.StepId);

        uint32_t StrategicOrderIdValue = 0U;
        if (OpeningPlanExecutionStateValue.TryGetPlanOrderId(OpeningPlanStepValue.StepId, StrategicOrderIdValue))
        {
            GameStateDescriptorValue.CommandAuthoritySchedulingState.SetOrderLifecycleState(StrategicOrderIdValue,
                                                                                           EOrderLifecycleState::Completed);

            size_t EconomyChildOrderIndexValue = 0U;
            if (GameStateDescriptorValue.CommandAuthoritySchedulingState.TryGetChildOrderIndex(
                    StrategicOrderIdValue, ECommandAuthorityLayer::EconomyAndProduction, EconomyChildOrderIndexValue))
            {
                const uint32_t EconomyChildOrderIdValue =
                    GameStateDescriptorValue.CommandAuthoritySchedulingState.OrderIds[EconomyChildOrderIndexValue];
                GameStateDescriptorValue.CommandAuthoritySchedulingState.SetOrderLifecycleState(
                    EconomyChildOrderIdValue, EOrderLifecycleState::Completed);
            }
        }
    }

    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState;
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size(); ++OrderIndexValue)
    {
        const EOrderLifecycleState LifecycleStateValue =
            CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue];
        if (LifecycleStateValue == EOrderLifecycleState::Completed || LifecycleStateValue == EOrderLifecycleState::Expired ||
            LifecycleStateValue == EOrderLifecycleState::Aborted)
        {
            continue;
        }

        const FCommandOrderRecord CommandOrderRecordValue =
            CommandAuthoritySchedulingStateValue.GetOrderRecord(OrderIndexValue);
        if (CommandOrderRecordValue.TargetCount == 0U ||
            !DoesOrderTargetMatchObservedState(GameStateDescriptorValue.BuildPlanning, CommandOrderRecordValue))
        {
            continue;
        }

        CommandAuthoritySchedulingStateValue.SetOrderLifecycleState(CommandOrderRecordValue.OrderId,
                                                                    EOrderLifecycleState::Completed);
    }

    if (OpeningPlanExecutionStateValue.CompletedStepIds.size() >= OpeningPlanDescriptorValue.Steps.size())
    {
        OpeningPlanExecutionStateValue.LifecycleState = EOpeningPlanLifecycleState::Completed;
    }
}

void FCommandAuthorityProcessor::SeedReadyStrategicOrders(FGameStateDescriptor& GameStateDescriptorValue) const
{
    FOpeningPlanExecutionState& OpeningPlanExecutionStateValue = GameStateDescriptorValue.OpeningPlanExecutionState;
    const FOpeningPlanDescriptor& OpeningPlanDescriptorValue =
        FOpeningPlanRegistry::GetOpeningPlanDescriptor(OpeningPlanExecutionStateValue.ActivePlanId);
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState;

    uint32_t SeededOrderCountValue = 0U;
    for (const FOpeningPlanStep& OpeningPlanStepValue : OpeningPlanDescriptorValue.Steps)
    {
        if (SeededOrderCountValue >= CommandAuthoritySchedulingStateValue.MaxStrategicOrdersPerStep)
        {
            return;
        }
        if (OpeningPlanExecutionStateValue.HasSeededStep(OpeningPlanStepValue.StepId) ||
            OpeningPlanExecutionStateValue.IsStepCompleted(OpeningPlanStepValue.StepId))
        {
            continue;
        }
        if (GameStateDescriptorValue.CurrentGameLoop < OpeningPlanStepValue.MinGameLoop)
        {
            return;
        }
        if (!AreRequiredStepsCompleted(OpeningPlanExecutionStateValue, OpeningPlanStepValue))
        {
            continue;
        }

        FCommandOrderRecord StrategicOrderValue = FCommandOrderRecord::CreateNoTarget(
            ECommandAuthorityLayer::StrategicDirector, NullTag, OpeningPlanStepValue.AbilityId,
            OpeningPlanStepValue.PriorityValue, EIntentDomain::StructureBuild, OpeningPlanStepValue.MinGameLoop);
        StrategicOrderValue.PlanStepId = OpeningPlanStepValue.StepId;
        StrategicOrderValue.TargetCount = OpeningPlanStepValue.TargetCount;
        StrategicOrderValue.ProducerUnitTypeId = OpeningPlanStepValue.ProducerUnitTypeId;
        StrategicOrderValue.ResultUnitTypeId = OpeningPlanStepValue.ResultUnitTypeId;
        StrategicOrderValue.UpgradeId = OpeningPlanStepValue.UpgradeId;
        StrategicOrderValue.IntentDomain = DetermineIntentDomain(StrategicOrderValue);

        const uint32_t StrategicOrderIdValue =
            CommandAuthoritySchedulingStateValue.EnqueueOrder(StrategicOrderValue);
        OpeningPlanExecutionStateValue.RecordSeededStep(OpeningPlanStepValue.StepId, StrategicOrderIdValue);
        ++SeededOrderCountValue;
    }
}

void FCommandAuthorityProcessor::EnsureWorkerGoalOrder(FGameStateDescriptor& GameStateDescriptorValue) const
{
    const FOpeningPlanDescriptor& OpeningPlanDescriptorValue =
        FOpeningPlanRegistry::GetOpeningPlanDescriptor(GameStateDescriptorValue.OpeningPlanExecutionState.ActivePlanId);
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState;

    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size(); ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.PlanStepIds[OrderIndexValue] == WorkerGoalStepIdValue &&
            CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue] != EOrderLifecycleState::Completed &&
            CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue] != EOrderLifecycleState::Expired)
        {
            return;
        }
    }

    if (OpeningPlanDescriptorValue.Goals.TargetWorkerCount == 0U)
    {
        return;
    }

    FCommandOrderRecord WorkerGoalOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::TRAIN_SCV, 110, EIntentDomain::UnitProduction,
        0U);
    WorkerGoalOrderValue.PlanStepId = WorkerGoalStepIdValue;
    WorkerGoalOrderValue.TargetCount = OpeningPlanDescriptorValue.Goals.TargetWorkerCount;
    WorkerGoalOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_COMMANDCENTER;
    WorkerGoalOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
    WorkerGoalOrderValue.IntentDomain = EIntentDomain::UnitProduction;
    CommandAuthoritySchedulingStateValue.EnqueueOrder(WorkerGoalOrderValue);
}

void FCommandAuthorityProcessor::EnsureStrategicChildOrders(FGameStateDescriptor& GameStateDescriptorValue) const
{
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState;
    const std::vector<size_t> StrategicOrderIndicesValue = CommandAuthoritySchedulingStateValue.StrategicOrderIndices;

    for (const size_t StrategicOrderIndexValue : StrategicOrderIndicesValue)
    {
        const FCommandOrderRecord StrategicOrderValue =
            CommandAuthoritySchedulingStateValue.GetOrderRecord(StrategicOrderIndexValue);
        if (StrategicOrderValue.SourceLayer != ECommandAuthorityLayer::StrategicDirector ||
            StrategicOrderValue.LifecycleState == EOrderLifecycleState::Completed ||
            DoesOrderTargetMatchObservedState(GameStateDescriptorValue.BuildPlanning, StrategicOrderValue))
        {
            continue;
        }

        size_t EconomyChildOrderIndexValue = 0U;
        if (CommandAuthoritySchedulingStateValue.TryGetChildOrderIndex(StrategicOrderValue.OrderId,
                                                                       ECommandAuthorityLayer::EconomyAndProduction,
                                                                       EconomyChildOrderIndexValue))
        {
            continue;
        }

        FCommandOrderRecord EconomyOrderValue = FCommandOrderRecord::CreateNoTarget(
            ECommandAuthorityLayer::EconomyAndProduction, NullTag, StrategicOrderValue.AbilityId,
            StrategicOrderValue.PriorityValue, StrategicOrderValue.IntentDomain,
            GameStateDescriptorValue.CurrentGameLoop, 0U, StrategicOrderValue.OrderId);
        EconomyOrderValue.PlanStepId = StrategicOrderValue.PlanStepId;
        EconomyOrderValue.TargetCount = StrategicOrderValue.TargetCount;
        EconomyOrderValue.ProducerUnitTypeId = StrategicOrderValue.ProducerUnitTypeId;
        EconomyOrderValue.ResultUnitTypeId = StrategicOrderValue.ResultUnitTypeId;
        EconomyOrderValue.UpgradeId = StrategicOrderValue.UpgradeId;
        CommandAuthoritySchedulingStateValue.EnqueueOrder(EconomyOrderValue);
    }
}

bool FCommandAuthorityProcessor::AreRequiredStepsCompleted(
    const FOpeningPlanExecutionState& OpeningPlanExecutionStateValue, const FOpeningPlanStep& OpeningPlanStepValue) const
{
    for (const uint32_t RequiredStepIdValue : OpeningPlanStepValue.RequiredCompletedStepIds)
    {
        if (!OpeningPlanExecutionStateValue.IsStepCompleted(RequiredStepIdValue))
        {
            return false;
        }
    }

    return true;
}

bool FCommandAuthorityProcessor::DoesOrderTargetMatchObservedState(
    const FBuildPlanningState& BuildPlanningStateValue, const FCommandOrderRecord& CommandOrderRecordValue) const
{
    return GetObservedCountForOrder(BuildPlanningStateValue, CommandOrderRecordValue) >= CommandOrderRecordValue.TargetCount;
}

uint32_t FCommandAuthorityProcessor::GetObservedCountForOrder(const FBuildPlanningState& BuildPlanningStateValue,
                                                              const FCommandOrderRecord& CommandOrderRecordValue) const
{
    switch (CommandOrderRecordValue.ResultUnitTypeId)
    {
        case UNIT_TYPEID::TERRAN_COMMANDCENTER:
            return BuildPlanningStateValue.ObservedTownHallCount;
        case UNIT_TYPEID::TERRAN_ORBITALCOMMAND:
            return BuildPlanningStateValue.ObservedOrbitalCommandCount;
        default:
            break;
    }

    if (IsTerranBuilding(CommandOrderRecordValue.ResultUnitTypeId))
    {
        return GetBuildingCount(BuildPlanningStateValue.ObservedBuildingCounts, CommandOrderRecordValue.ResultUnitTypeId);
    }

    return GetUnitCount(BuildPlanningStateValue.ObservedUnitCounts, CommandOrderRecordValue.ResultUnitTypeId);
}

}  // namespace sc2
