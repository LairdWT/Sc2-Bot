# TerranAgent Coordinator Path

## Purpose

This page documents the current authoritative coordinator order in:

- `L:\Sc2_Bot\examples\terran\terran.h`
- `L:\Sc2_Bot\examples\terran\terran.cc`

Use this as the first runtime entrypoint reference before domain pages.

## Runtime Entry Points

`TerranAgent` derives from `sc2::Agent` and implements:

- `OnGameStart()`
- `OnStep()`
- `OnGameEnd()`
- `OnUnitIdle(const Unit* UnitPtr)`
- `OnUnitCreated(const Unit* UnitPtr)`
- `OnBuildingConstructionComplete(const Unit* UnitPtr)`

`OnGameStart()` and `OnStep()` own the frame pipeline.

## OnGameStart Pipeline

`OnGameStart()` executes this ordered sequence:

1. Set `CurrentStep = 0`.
2. Cache `Observation()` in `ObservationPtr`; return if null.
3. Rebuild `ExpansionLocations` from `search::CalculateExpansionLocations(...)` when `Query()` is valid.
4. Reset runtime state:
   - `GameStateDescriptor`
   - `EconomyDomainState`
   - `ExecutionTelemetry`
   - `ProductionRallyStates`
   - `PendingProductionRallyIntents`
   - wall and scheduler counters (`CurrentWallGateState`, `LastArmyExecutionOrderCount`, `LastTerminalCompactionStep`, and related timing and count fields)
5. Build `FFrameContext Frame = FFrameContext::Create(ObservationPtr, Query(), CurrentStep)`.
6. Call, in order:
   - `UpdateAgentState(Frame)`
   - `InitializeRampWallDescriptor(Frame)`
   - `InitializeMainBaseLayoutDescriptor(Frame)`
   - `RebuildObservedGameStateDescriptor(Frame)`
   - `RebuildForecastState()`
   - `RebuildExecutionPressureDescriptor(Frame)`
   - `UpdateStrategicAndPlanningState()`
   - `PrintAgentState()`

## OnStep Pipeline

`OnStep()` executes this authoritative per-frame order:

1. Increment `CurrentStep`.
2. Cache `Observation()` in `ObservationPtr`; return if null.
3. Build `FFrameContext Frame`.
4. `UpdateAgentState(Frame)`.
5. `UpdateDispatchedSchedulerOrders(Frame)`.
6. Descriptor and planning refresh:
   - `RebuildObservedGameStateDescriptor(Frame)`
   - `RebuildForecastState()`
   - `RebuildExecutionPressureDescriptor(Frame)`
   - `UpdateStrategicAndPlanningState()`
7. Reset per-step buffers:
   - `IntentBuffer.Reset()`
   - `PendingProductionRallyIntents.clear()`
8. Produce intents in fixed order:
   - `ProduceSchedulerIntents(Frame)`
   - `ProduceProductionRallyIntents()`
   - `ProduceWallGateIntents(Frame)`
   - `ProduceWorkerHarvestIntents(Frame)`
   - `ProduceRecoveryIntents(Frame)`
9. `UpdateExecutionTelemetry(Frame)`.
10. `ResolvedIntents = IntentArbiter.Resolve(Frame, AgentState.UnitContainer, IntentBuffer)`.
11. Command issue:
   - `ExecuteResolvedIntents(Frame, ResolvedIntents)`
   - `ExecuteProductionRallyIntents()`
12. `CaptureNewlyDispatchedSchedulerOrders(Frame)`.
13. `PrintAgentState()` every 120 steps.

`OnStep()` also records phase durations in:

- `LastAgentStateUpdateMicroseconds`
- `LastDispatchMaintenanceMicroseconds`
- `LastDescriptorRebuildMicroseconds`
- `LastSchedulerStrategicProcessingMicroseconds`
- `LastSchedulerEconomyProcessingMicroseconds`
- `LastSchedulerArmyProcessingMicroseconds`
- `LastSchedulerSquadProcessingMicroseconds`
- `LastSchedulerUnitExecutionProcessingMicroseconds`
- `LastSchedulerDrainMicroseconds`
- `LastIntentResolutionMicroseconds`
- `LastIntentExecutionMicroseconds`
- `LastDispatchCaptureMicroseconds`
- `LastStepMicroseconds`

## Descriptor And Planning Boundary

The descriptor and planning refresh is split across three functions:

1. `RebuildObservedGameStateDescriptor(const FFrameContext& Frame)`
2. `RebuildForecastState()`
3. `RebuildExecutionPressureDescriptor(const FFrameContext& Frame)`
4. `UpdateStrategicAndPlanningState()`

`RebuildExecutionPressureDescriptor(...)` rebuilds `GameStateDescriptor.ExecutionPressure` from current observed build-planning values plus recent `ExecutionTelemetry` signals (supply pressure, mineral banking pressure, deferral breakdown, and idle producer counts without active scheduled work).

`UpdateStrategicAndPlanningState()` applies:

- `StrategicDirector->UpdateGameStateDescriptor(...)`
- `BuildPlanner->ProduceBuildPlan(...)`
- `ArmyPlanner->ProduceArmyPlan(...)`
- `UpdateRallyAnchor()`

## Scheduler Boundary

`ProduceSchedulerIntents(const FFrameContext& Frame)` applies this order:

1. `CommandAuthorityProcessor.ProcessSchedulerStep(...)` (with or without `ICommandTaskAdmissionService`).
2. Economy stage (mutation batch):
   - `IEconomyProductionOrderExpander::ExpandEconomyAndProductionOrders(...)`
   - `ICommandTaskAdmissionService::ParkDeferredOrders(...)` when configured.
3. Army stage (mutation batch):
   - `IArmyOrderExpander::ExpandArmyOrders(...)`
   - `IArmyPlanner::ProduceArmyPlan(...)` when configured.
4. Squad stage (mutation batch): `ISquadOrderExpander::ExpandSquadOrders(...)`.
5. Unit execution stage (mutation batch):
   - `IUnitExecutionPlanner::ExpandUnitExecutionOrders(...)`
   - refresh `LastActiveIndexedExecutionOrderCount`.
6. `ICommandTaskPriorityService::UpdateTaskPriorities(...)` when configured.
7. `FIntentSchedulingService::DrainReadyIntents(...)` into `IntentBuffer`, bounded by `MaxUnitIntentsPerStep`.

## Dispatch Maintenance And Capture Boundary

`UpdateDispatchedSchedulerOrders(const FFrameContext& Frame)` inspects only `UnitExecution` orders in `EOrderLifecycleState::Dispatched` and marks:

- `Completed` when observed completed count increases since dispatch snapshot.
- `Completed` when observed in-construction count increases since dispatch snapshot.
- `Completed` when `HasProducerConfirmedDispatchedOrder(...)` succeeds.
- `Aborted` when actor is missing after dispatch loop.
- `Aborted` when dispatch confirmation exceeds `DispatchConfirmationTimeoutGameLoopsValue` (96 loops).

Compaction runs only when both conditions hold:

- interval reached: `CurrentStep >= LastTerminalCompactionStep + TerminalOrderCompactionIntervalStepCountValue`
- threshold reached: `OrderIds.size() >= TerminalOrderCompactionTriggerCountValue`

`CaptureNewlyDispatchedSchedulerOrders(const FFrameContext& Frame)` captures first dispatch snapshots through `SetOrderDispatchState(...)` for `UnitExecution` orders where:

- `LifecycleStates[OrderIndex] == EOrderLifecycleState::Dispatched`
- `DispatchAttemptCounts[OrderIndex] == 0`

Captured fields:

- `CurrentStep`
- `Frame.GameLoop`
- `GetObservedCountForOrder(...)`
- `GetObservedInConstructionCountForOrder(...)`

## Telemetry Boundary

`UpdateExecutionTelemetry(const FFrameContext& Frame)` runs after producers and before arbiter resolve. It records:

- supply-block transitions
- mineral-bank transitions
- actor-intent conflicts from `IntentBuffer`
- idle-production conflicts (barracks and town halls)
- scheduler deferral events for orders deferred on `CurrentStep`

Wall telemetry events are emitted in other coordinator functions:

- `InitializeRampWallDescriptor(...)` -> `RecordWallDescriptorInvalid(...)`
- `ProduceWallGateIntents(...)` -> `RecordWallThreatDetected(...)`, `RecordWallClosed(...)`, `RecordWallOpened(...)`

## Arbiter And Execution Boundary

`FIntentArbiter::Resolve(...)` (implemented in `L:\Sc2_Bot\examples\common\agent_framework.h`) is the conflict-resolution boundary before command issue:

- one winner per actor (priority, then `EIntentDomain`, then stable original order)
- actor and target validation against current frame state
- point-target normalization and optional placement/pathing checks
- one structure-build reservation per actor per resolve pass

Execution is split:

- `ExecuteResolvedIntents(...)` issues resolved intents from `IntentBuffer`.
- `ExecuteProductionRallyIntents()` issues deferred rally commands from `PendingProductionRallyIntents`.

## Producer Seams

- `ProduceProductionRallyIntents()`:
  - tracks per-structure state in `ProductionRallyStates`
  - enqueues `RALLY_UNITS` point intents when initial apply is needed or rally point moved
- `ProduceWallGateIntents(...)`:
  - computes desired `EWallGateState`
  - emits transition intents via `IWallGateController`
  - records wall transition telemetry
- `ProduceWorkerHarvestIntents(...)`:
  - fills under-saturated refineries
  - relieves over-saturated refineries
  - excludes workers already reserved by intents or active scheduler orders
- `ProduceRecoveryIntents(...)`:
  - converts idle SCVs and pending recovery callbacks into safe fallback commands

## Cross Links

- [IntentSchedulingAndCommandAuthority.md](./IntentSchedulingAndCommandAuthority.md)
- [TelemetryAndGameRecordStore.md](./TelemetryAndGameRecordStore.md)

## Invariants

- `ObservationPtr` must be valid before descriptor, scheduler, telemetry, and execution work.
- `FFrameContext` is the per-step transport passed across coordinator boundaries.
- Dispatch maintenance executes before descriptor rebuild in `OnStep()`.
- Telemetry executes after intent production and before `FIntentArbiter::Resolve(...)`.
- `CaptureNewlyDispatchedSchedulerOrders(...)` records only orders that are already `Dispatched` and not yet captured.
