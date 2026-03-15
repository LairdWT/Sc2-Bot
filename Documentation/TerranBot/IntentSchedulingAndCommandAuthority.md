# Intent Scheduling And Command Authority

## Purpose

This page documents the current scheduling and command-authority implementation used by `TerranAgent`.

Primary code surfaces:

- `L:\Sc2_Bot\examples\terran\terran.cc`
- `L:\Sc2_Bot\examples\common\planning\FCommandAuthoritySchedulingState.h`
- `L:\Sc2_Bot\examples\common\planning\FCommandAuthoritySchedulingState.cc`
- `L:\Sc2_Bot\examples\common\planning\FIntentSchedulingService.h`
- `L:\Sc2_Bot\examples\common\planning\FIntentSchedulingService.cc`
- `L:\Sc2_Bot\examples\common\planning\FCommandAuthorityProcessor.h`
- `L:\Sc2_Bot\examples\common\planning\FCommandAuthorityProcessor.cc`

## Authority Layers

`FCommandAuthoritySchedulingState` stores `SourceLayers` using `ECommandAuthorityLayer`:

- `Agent`
- `StrategicDirector`
- `EconomyAndProduction`
- `Army`
- `Squad`
- `UnitExecution`

`TerranAgent` issues SC2 commands only from resolved unit intents in `ExecuteResolvedIntents(...)`.

## Authoritative Store Shape

`FCommandAuthoritySchedulingState` owns structure-of-arrays scheduler columns including:

- identity and hierarchy: `OrderIds`, `ParentOrderIds`, `SourceGoalIds`, `SourceLayers`
- lifecycle and priority: `LifecycleStates`, `BasePriorityValues`, `EffectivePriorityValues`, `PriorityTiers`
- command payload: `ActorTags`, `AbilityIds`, `TargetKinds`, `TargetPoints`, `TargetUnitTags`
- scheduler metadata: `IntentDomains`, `CreationSteps`, `DeadlineSteps`, `PlanStepIds`, `RequestedQueueCounts`
- placement and producer metadata: `ProducerUnitTypeIds`, `ResultUnitTypeIds`, `PreferredPlacementSlot*`, `ReservedPlacementSlot*`
- deferral metadata: `LastDeferralReasons`, `LastDeferralSteps`, `LastDeferralGameLoops`, `ConsecutiveDeferralCounts`
- dispatch snapshots: `DispatchSteps`, `DispatchGameLoops`, `ObservedCountsAtDispatch`, `ObservedInConstructionCountsAtDispatch`, `DispatchAttemptCounts`

`OrderIdToIndex` remains the authoritative lookup from order id to column index.

## Lifecycle States

The active lifecycle enum is `EOrderLifecycleState` with queue views rebuilt from lifecycle values:

- `Queued`
- `Preprocessing`
- `Ready`
- `Dispatched`
- `Completed`
- `Aborted`
- `Expired`

Derived views are rebuilt by `RebuildDerivedQueues()`:

- `StrategicOrderIndices`
- `PlanningProcessIndices`
- `ArmyOrderIndices`
- `SquadOrderIndices`
- `ReadyIntentIndices`
- `DispatchedOrderIndices`
- `CompletedOrderIndices`

Priority-tier and domain-bucket views are maintained in:

- `StrategicQueues`
- `PlanningQueues`
- `ArmyQueues`
- `SquadQueues`
- `ReadyIntentQueues`

## Mutation Batch Contract

`BeginMutationBatch()` and `EndMutationBatch()` gate derived queue rebuild cost.

- `MarkDerivedQueuesDirty()` tracks pending rebuilds.
- Rebuild occurs immediately when no batch is active.
- Rebuild is deferred until the outermost `EndMutationBatch()` when batching is active.

`TerranAgent::ProduceSchedulerIntents(...)` uses mutation batches around each expansion phase.

## Processor And Playback State

`RebuildProcessorState()` sets:

- `EPlanningProcessorState::ReadyToDrain` when `ReadyIntentIndices` has work
- `EPlanningProcessorState::Processing` when upstream queues have work
- `EPlanningProcessorState::Idle` otherwise

`RebuildPlaybackState()` sets:

- `EIntentPlaybackState::ReadyBufferPending` when `ReadyIntentIndices` has work
- `EIntentPlaybackState::Dispatching` when only `DispatchedOrderIndices` has work
- `EIntentPlaybackState::Blocked` when upstream queues have work but no ready intents
- `EIntentPlaybackState::Idle` otherwise

## Ready-Intent Drain Boundary

`FIntentSchedulingService::DrainReadyIntents(...)` is the scheduler-to-intent handoff.

Drain rules:

1. Effective drain budget is `min(MaxIntentCountValue, MaxUnitIntentsPerStep)`.
2. Iterate `ReadyIntentIndices` in pre-sorted order.
3. Invalid command payload (`ActorTag == NullTag` or invalid ability) is marked aborted.
4. Valid records are converted into `FUnitIntent` via `AppendCommandOrderToIntentBuffer(...)`.
5. Dispatched records transition lifecycle to `Dispatched`.
6. Aborted records transition lifecycle to `Aborted`.
7. Lifecycle mutations are wrapped in one mutation batch.

## Dispatch Tracking And Compaction

`TerranAgent` dispatch tracking uses two distinct phases:

- `UpdateDispatchedSchedulerOrders(...)`:
  - validates existing `Dispatched` `UnitExecution` orders
  - marks orders `Completed` or `Aborted`
  - compacts terminal `UnitExecution` orders through `CompactTerminalOrders()`
- `CaptureNewlyDispatchedSchedulerOrders(...)`:
  - captures dispatch snapshot for newly dispatched orders where `DispatchAttemptCounts == 0`
  - increments dispatch attempts through `SetOrderDispatchState(...)`

`CompactTerminalOrders()` removes only terminal `UnitExecution` rows and rebuilds `OrderIdToIndex`.

## Deferral And Blocked Work Signals

`FCommandAuthoritySchedulingState` tracks deferrals per order through:

- `SetOrderDeferralState(...)`
- `ClearOrderDeferralState(...)`

Per-frame deferral telemetry is emitted by `TerranAgent::UpdateExecutionTelemetry(...)` when:

- `LastDeferralSteps[OrderIndex] == CurrentStep`
- `LastDeferralReasons[OrderIndex] != ECommandOrderDeferralReason::None`

Parking and blocked-task behavior is handled by `FTerranCommandTaskAdmissionService` in the scheduler production path.

## Test Coverage

Focused tests for this seam:

- `L:\Sc2_Bot\tests\test_command_authority_scheduling.cc`
  - lifecycle transitions
  - ready drain budgets
  - dispatch snapshot and attempt counting
  - mutation batch rebuild behavior
  - compaction of terminal `UnitExecution` orders
- `L:\Sc2_Bot\tests\test_terran_opening_plan_scheduler.cc`
  - scheduler seeding and plan-step driven order progression