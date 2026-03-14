# TerranAgent Coordinator Path

## Purpose

This document captures the current coordinator path implemented by `TerranAgent` in:

- `L:\Sc2_Bot\examples\terran\terran.h`
- `L:\Sc2_Bot\examples\terran\terran.cc`

`TerranAgent` is the orchestration seam. Builder, planner, scheduler, placement, and telemetry types own domain logic.

## Current Entry Points

`TerranAgent` derives from `sc2::Agent` and currently owns these runtime callbacks:

- `OnGameStart()`
- `OnStep()`
- `OnGameEnd()`
- `OnUnitIdle(const Unit* UnitPtr)`
- `OnUnitCreated(const Unit* UnitPtr)`

`OnGameStart()` and `OnStep()` are the coordinator entry points.

## Game Start Path

`OnGameStart()` executes in this order:

1. Reset `CurrentStep`.
2. Cache `Observation()` into `ObservationPtr` and return on null.
3. Rebuild `ExpansionLocations` from `search::CalculateExpansionLocations(ObservationPtr, Query())`.
4. Reset runtime state:
   - `GameStateDescriptor`
   - `ExecutionTelemetry`
   - `PendingProductionRallyStructureTags`
   - `CurrentWallGateState`
   - `LastArmyExecutionOrderCount`
5. Build `FFrameContext` through `FFrameContext::Create(ObservationPtr, Query(), CurrentStep)`.
6. Call:
   - `UpdateAgentState(Frame)`
   - `InitializeRampWallDescriptor(Frame)`
   - `InitializeMainBaseLayoutDescriptor(Frame)`
   - `RebuildGameStateDescriptor(Frame)`
   - `PrintAgentState()`

## Per-Step Coordinator Order

`OnStep()` is the authoritative frame pipeline:

1. Increment `CurrentStep`.
2. Refresh `ObservationPtr` and return on null.
3. Build `FFrameContext`.
4. `UpdateAgentState(Frame)`.
5. `RebuildGameStateDescriptor(Frame)`.
6. `UpdateDispatchedSchedulerOrders(Frame)`.
7. `IntentBuffer.Reset()`.
8. Run intent producers in this fixed order:
   - `ProduceSchedulerIntents(Frame)`
   - `ProduceProductionRallyIntents()`
   - `ProduceWallGateIntents(Frame)`
   - `ProduceWorkerHarvestIntents(Frame)`
   - `ProduceRecoveryIntents(Frame)`
9. `UpdateExecutionTelemetry(Frame)`.
10. `ResolvedIntents = IntentArbiter.Resolve(Frame, AgentState.UnitContainer, IntentBuffer)`.
11. `ExecuteResolvedIntents(Frame, ResolvedIntents)`.
12. `CaptureNewlyDispatchedSchedulerOrders(Frame)`.
13. `PrintAgentState()` every 120 steps.

## Descriptor And Planning Rebuild Path

`RebuildGameStateDescriptor(const FFrameContext& Frame)` currently runs:

1. `GameStateDescriptorBuilder->RebuildGameStateDescriptor(...)` when configured.
2. `StrategicDirector->UpdateGameStateDescriptor(GameStateDescriptor)`.
3. `BuildPlanner->ProduceBuildPlan(GameStateDescriptor, GameStateDescriptor.BuildPlanning)`.
4. `ArmyPlanner->ProduceArmyPlan(GameStateDescriptor, GameStateDescriptor.ArmyState)`.
5. `UpdateRallyAnchor()`.

This keeps descriptor rebuild ahead of all producer and scheduler stages.

## Scheduler Producer Path

`ProduceSchedulerIntents(const FFrameContext& Frame)` performs scheduler-side production in this order:

1. `CommandAuthorityProcessor.ProcessSchedulerStep(GameStateDescriptor)`.
2. `CommandTaskPriorityService->UpdateTaskPriorities(GameStateDescriptor)` when available.
3. `EconomyProductionOrderExpander->ExpandEconomyAndProductionOrders(...)`.
4. Priority refresh.
5. `ArmyOrderExpander->ExpandArmyOrders(...)`.
6. `ArmyPlanner->ProduceArmyPlan(...)`.
7. Priority refresh.
8. `SquadOrderExpander->ExpandSquadOrders(...)`.
9. Priority refresh.
10. `UnitExecutionPlanner->ExpandUnitExecutionOrders(...)` and store `LastArmyExecutionOrderCount`.
11. Final priority refresh.
12. `IntentSchedulingService.DrainReadyIntents(...)` into `IntentBuffer`.

## Dispatched Scheduler Order Maintenance

`UpdateDispatchedSchedulerOrders(const FFrameContext& Frame)` inspects `UnitExecution` orders in `Dispatched` lifecycle state and applies:

- `Completed` when observed completed count or in-construction count increases after dispatch snapshot.
- `Completed` when producer confirmation detects matching active order/add-on/morph state.
- `Aborted` when actor unit is missing after dispatch game loop.
- `Aborted` when dispatch confirmation timeout exceeds `96` game loops.

`CaptureNewlyDispatchedSchedulerOrders(const FFrameContext& Frame)` records first dispatch snapshots (`CurrentStep`, `Frame.GameLoop`, observed counts) for `UnitExecution` orders that are `Dispatched` with zero dispatch attempts.

## Intent Arbiter Boundary

`FIntentArbiter::Resolve(...)` is the arbitration boundary between production and command execution:

- select one winner per actor by higher `Priority`, then by `EIntentDomain` order for ties
- preserve original buffer order for equal winners
- validate actor and target existence
- enforce one structure-build actor reservation in a resolve pass
- for point targets: clamp to playable bounds and run optional placement/pathing validation when requested

Only resolved intents pass into execution.

## Command Execution Boundary

`ExecuteResolvedIntents(...)` is the only stage that issues SC2 commands. It dispatches by `EIntentTargetKind`:

- `None` -> `Actions()->UnitCommand(actor, ability, queued)`
- `Point` -> `Actions()->UnitCommand(actor, ability, point, queued)`
- `Unit` -> `Actions()->UnitCommand(actor, ability, targetTag, queued)`

## Additional Producer Responsibilities

- `ProduceProductionRallyIntents()`:
  - refreshes rally commands on a `120`-step cadence and for newly created production structures
  - uses `PendingProductionRallyStructureTags` to force one-time rally refresh on creation
- `ProduceWallGateIntents(...)`:
  - computes desired `EWallGateState` and emits transition-only gate intents
  - records wall threat/open/close telemetry transitions
- `ProduceWorkerHarvestIntents(...)`:
  - balances refinery fill and gas relief using commitment checks and reservation maps
  - avoids units with active intents, active scheduler orders, or construction commitments
- `ProduceRecoveryIntents(...)`:
  - repairs idle worker behavior by sending workers to minerals, falling back to enemy attack-move if no patch is found

## Runtime Invariants

- `ObservationPtr` must be valid before state rebuild, planning, and execution work.
- `FFrameContext` is the authoritative per-frame transport.
- Descriptor and planner rebuild occurs before all intent production.
- Telemetry updates before arbitration and execution.
- `TerranAgent` owns sequence only; composed services own domain decisions.
