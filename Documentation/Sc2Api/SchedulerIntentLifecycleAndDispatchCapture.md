# Scheduler Intent Lifecycle And Dispatch Capture

## Scope

- TopicId: `SC2-API-SCHEDULER-INTENT-LIFECYCLE`
- Domain: `ApiSchedulerLifecycle`
- Focus: `TerranAgent::OnStep()` scheduler-intent lifecycle from order production through dispatch capture in `FCommandAuthoritySchedulingState`.

## Source Anchors

- `L:\Sc2_Bot\examples\terran\terran.cc`
  - `TerranAgent::OnStep()`
  - `TerranAgent::ProduceSchedulerIntents(const FFrameContext& Frame)`
  - `TerranAgent::UpdateExecutionTelemetry(const FFrameContext& Frame)`
  - `TerranAgent::ExecuteResolvedIntents(const FFrameContext& Frame, const std::vector<FUnitIntent>& Intents)`
  - `TerranAgent::UpdateDispatchedSchedulerOrders(const FFrameContext& Frame)`
  - `TerranAgent::CaptureNewlyDispatchedSchedulerOrders(const FFrameContext& Frame)`
- `L:\Sc2_Bot\examples\common\planning\FCommandAuthorityProcessor.cc`
  - `FCommandAuthorityProcessor::ProcessSchedulerStep(FGameStateDescriptor& GameStateDescriptorValue)`
  - `FCommandAuthorityProcessor::EnsureStrategicChildOrders(FGameStateDescriptor& GameStateDescriptorValue)`
- `L:\Sc2_Bot\examples\common\planning\FIntentSchedulingService.cc`
  - `FIntentSchedulingService::DrainReadyIntents(...)`
- `L:\Sc2_Bot\examples\common\planning\FCommandAuthoritySchedulingState.h`
  - `struct FCommandAuthoritySchedulingState`
- `L:\Sc2_Bot\examples\common\planning\FCommandAuthoritySchedulingState.cc`
  - `FCommandAuthoritySchedulingState::RebuildDerivedQueues()`
  - `FCommandAuthoritySchedulingState::SetOrderDispatchState(...)`
  - `FCommandAuthoritySchedulingState::CompactTerminalOrders()`
- `L:\Sc2_Bot\examples\common\planning\FTerranEconomyProductionOrderExpander.cc`
  - child order creation path setting `UnitExecutionOrderValue.LifecycleState = EOrderLifecycleState::Ready`
- `L:\Sc2_Bot\examples\common\planning\FTerranArmyUnitExecutionPlanner.cc`
  - execution-order emission path setting `LifecycleState = EOrderLifecycleState::Ready`

## Ordered Lifecycle (Per Step)

1. `TerranAgent::OnStep()` updates observation-backed state and descriptor (`UpdateAgentState`, `RebuildGameStateDescriptor`).
2. `UpdateDispatchedSchedulerOrders` maintains previously dispatched `ECommandAuthorityLayer::UnitExecution` orders.
3. `ProduceSchedulerIntents` runs scheduler producers:
   - `FCommandAuthorityProcessor::ProcessSchedulerStep(...)` seeds strategic and child orders.
   - economy, army, squad, and unit-execution expanders mutate scheduler state in batch.
   - `FIntentSchedulingService::DrainReadyIntents(...)` converts `ReadyIntentIndices` into `FIntentBuffer` entries and marks those orders `Dispatched`.
4. non-scheduler intent producers add recovery and wall/rally intents.
5. `UpdateExecutionTelemetry` records conflicts, idle production conflicts, and step-local deferrals from `FCommandAuthoritySchedulingState`.
6. `FIntentArbiter::Resolve(...)` picks one validated winner per actor from combined intents.
7. `ExecuteResolvedIntents` sends `ActionInterface::UnitCommand(...)` calls.
8. `CaptureNewlyDispatchedSchedulerOrders` stamps dispatch metadata (`DispatchStep`, `DispatchGameLoop`, observed counters) only for newly dispatched scheduler orders where `DispatchAttemptCounts == 0`.

## Authority Containers And State Separation

- Authoritative scheduler order storage:
  - `FCommandAuthoritySchedulingState` SoA vectors (`OrderIds`, `SourceLayers`, `LifecycleStates`, `Dispatch*`, `LastDeferral*`, etc.).
- Derived lifecycle views are rebuilt by `RebuildDerivedQueues()`:
  - `ReadyIntentIndices` contains only `EOrderLifecycleState::Ready`.
  - `DispatchedOrderIndices` contains only `EOrderLifecycleState::Dispatched`.
  - `CompletedOrderIndices` contains only terminal states (`Completed`, `Aborted`, `Expired`).
- Distinct lifecycle boundaries observed in code:
  - queued/preprocessing/ready are producer and scheduling stages.
  - dispatched begins at `DrainReadyIntents` lifecycle mutation.
  - completion or abort is assigned by dispatch maintenance or producer-observed completion checks.

## Dispatch Maintenance And Correlation

- `UpdateDispatchedSchedulerOrders` evaluates each dispatched `UnitExecution` order against:
  - observed result-count deltas (`GetObservedCountForOrder`, `GetObservedInConstructionCountForOrder`),
  - producer-side command confirmation (`HasProducerConfirmedDispatchedOrder`),
  - actor disappearance after dispatch,
  - timeout (`DispatchConfirmationTimeoutGameLoopsValue = 96`).
- Successful or terminal outcomes update lifecycle via `SetOrderLifecycleState(...)`.
- `CompactTerminalOrders()` compacts terminal `UnitExecution` rows after maintenance; non-terminal and non-`UnitExecution` records remain.

## Dispatch Attempt Ownership And Retry Boundary

- Dispatch-attempt mutation is owned by `FCommandAuthoritySchedulingState::SetOrderDispatchState(...)`:
  - writes `DispatchStep`, `DispatchGameLoop`, `ObservedCountAtDispatch`, and `ObservedInConstructionCountAtDispatch`
  - increments `DispatchAttemptCount`
  - clears per-order deferral state fields (`LastDeferralReason`, `LastDeferralStep`, `LastDeferralGameLoop`, `ConsecutiveDeferralCount`)
- In the Terran integration seam, `TerranAgent::CaptureNewlyDispatchedSchedulerOrders(...)` is the only call site that stamps dispatch state for scheduler orders, and it gates on:
  - `SourceLayers[OrderIndex] == ECommandAuthorityLayer::UnitExecution`
  - `LifecycleStates[OrderIndex] == EOrderLifecycleState::Dispatched`
  - `DispatchAttemptCounts[OrderIndex] == 0U`
- `FIntentSchedulingService::DrainReadyIntents(...)` transitions lifecycle from `Ready` to `Dispatched` but does not increment `DispatchAttemptCount` itself.
- Source-proven implication for this path:
  - a row becomes "dispatched" at drain-time
  - dispatch metadata is stamped once when capture sees first-dispatch rows
  - if a row remains `Dispatched`, this loop does not emit additional dispatch-attempt stamps for that same row
- Coverage anchors:
  - `tests\test_command_authority_scheduling.cc` verifies default `DispatchAttemptCount == 0U` and increment on `SetOrderDispatchState(...)`
  - `tests\test_terran_economy_production_order_expander.cc` exercises replacement child-order creation after a completed dispatched child, preserving slot continuity through a new order row

## Deferral And Telemetry Boundary

- Expanders record blocking reasons with `SetOrderDeferralState(...)`.
- `UpdateExecutionTelemetry` emits `RecordSchedulerOrderDeferred(...)` only when `LastDeferralSteps[OrderIndex] == CurrentStep` and reason is non-`None`.
- This keeps telemetry aligned to the same step boundary as intent production, before arbitration and command issue.

## Deferral Cooldown And Dispatch Metadata Boundary

- `FAgentExecutionTelemetry::RecordSchedulerOrderDeferred(...)` coalesces repeated deferrals for the same `OrderId` and same `ECommandOrderDeferralReason` for `EventCooldownStepsValue = 120` steps.
- Source boundary in `TerranAgent::UpdateExecutionTelemetry(...)`:
  - Deferral telemetry eligibility is step-local (`LastDeferralSteps == CurrentStep`) and reason-gated (`LastDeferralReasons != ECommandOrderDeferralReason::None`).
  - Event emission rate is additionally telemetry-local because `FAgentExecutionTelemetry` suppresses same-order same-reason repeats inside the cooldown window.
- Source boundary in `FCommandAuthoritySchedulingState::SetOrderDispatchState(...)`:
  - Dispatch metadata stamp (`DispatchStep`, `DispatchGameLoop`, observed counters, `DispatchAttemptCount`) clears deferral fields (`LastDeferralReason`, `LastDeferralStep`, `LastDeferralGameLoop`, `ConsecutiveDeferralCount`).
  - A subsequent deferral must be explicitly re-written by producer/expander code through `SetOrderDeferralState(...)` before telemetry can emit another event.
- Coverage anchor:
  - `tests\test_agent_execution_telemetry.cc` asserts same-reason coalescing and reason-change re-emission for `RecordSchedulerOrderDeferred(...)`.

## Verified Invariants

- Produced ready orders and dispatched captures are correlated by the same `OrderId` row in `FCommandAuthoritySchedulingState`.
- Ready and dispatched state are distinct lifecycle states, not inferred from intent buffer presence.
- Capture does not fabricate dispatch rows; it only annotates pre-existing `Dispatched` rows with zero dispatch attempts.
- Dispatch maintenance and telemetry both operate on the same authoritative scheduler state object for the current frame.

## Ambiguities

- None opened in this topic pass.
