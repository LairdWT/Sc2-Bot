# TerranAgent Coordinator Path

## Purpose

This document captures the current coordinator path implemented by `TerranAgent` in:

- `L:\Sc2_Bot\examples\terran\terran.h`
- `L:\Sc2_Bot\examples\terran\terran.cc`

`TerranAgent` owns frame orchestration order. Builder, planner, scheduler, placement, and telemetry types own domain behavior.

## Runtime Entry Points

`TerranAgent` derives from `sc2::Agent` and implements:

- `OnGameStart()`
- `OnStep()`
- `OnGameEnd()`
- `OnUnitIdle(const Unit* UnitPtr)`
- `OnUnitCreated(const Unit* UnitPtr)`

`OnGameStart()` and `OnStep()` are the authoritative coordinator paths.

## OnGameStart Pipeline

`OnGameStart()` executes in this order:

1. Set `CurrentStep = 0`.
2. Cache `Observation()` into `ObservationPtr`; return if null.
3. Rebuild `ExpansionLocations` from `search::CalculateExpansionLocations(ObservationPtr, Query())` when `Query()` is valid.
4. Reset runtime state:
   - `GameStateDescriptor`
   - `ExecutionTelemetry`
   - `PendingProductionRallyStructureTags`
   - `CurrentWallGateState`
   - `LastArmyExecutionOrderCount`
5. Build `FFrameContext` through `FFrameContext::Create(ObservationPtr, Query(), CurrentStep)`.
6. Call, in order:
   - `UpdateAgentState(Frame)`
   - `InitializeRampWallDescriptor(Frame)`
   - `InitializeMainBaseLayoutDescriptor(Frame)`
   - `RebuildGameStateDescriptor(Frame)`
   - `PrintAgentState()`

## OnStep Pipeline

`OnStep()` is the authoritative per-frame order:

1. Increment `CurrentStep`.
2. Cache `Observation()` into `ObservationPtr`; return if null.
3. Build `FFrameContext`.
4. `UpdateAgentState(Frame)`.
5. `RebuildGameStateDescriptor(Frame)`.
6. `UpdateDispatchedSchedulerOrders(Frame)`.
7. `IntentBuffer.Reset()`.
8. Produce intents in fixed order:
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

`OnStep()` also records phase durations in:

- `LastAgentStateUpdateMicroseconds`
- `LastDescriptorRebuildMicroseconds`
- `LastDispatchMaintenanceMicroseconds`
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

## Descriptor Rebuild Boundary

`RebuildGameStateDescriptor(const FFrameContext& Frame)` runs:

1. `GameStateDescriptorBuilder->RebuildGameStateDescriptor(...)` when `GameStateDescriptorBuilder` is valid.
2. Fallback assignment of `CurrentStep` and `CurrentGameLoop` when no builder is configured.
3. `StrategicDirector->UpdateGameStateDescriptor(GameStateDescriptor)`.
4. `BuildPlanner->ProduceBuildPlan(GameStateDescriptor, GameStateDescriptor.BuildPlanning)`.
5. `ArmyPlanner->ProduceArmyPlan(GameStateDescriptor, GameStateDescriptor.ArmyState)`.
6. `UpdateRallyAnchor()`.

This keeps descriptor and planning state rebuild ahead of scheduler production and arbitration.

## Scheduler Production Boundary

`ProduceSchedulerIntents(const FFrameContext& Frame)` runs this order:

1. `CommandAuthorityProcessor.ProcessSchedulerStep(GameStateDescriptor, *CommandTaskAdmissionService)` when admission service exists, else `ProcessSchedulerStep(GameStateDescriptor)`.
2. Economy stage under mutation batch:
   - `EconomyProductionOrderExpander->ExpandEconomyAndProductionOrders(...)`
   - `CommandTaskAdmissionService->ParkDeferredOrders(GameStateDescriptor)` when admission service exists.
3. Army stage under mutation batch:
   - `ArmyOrderExpander->ExpandArmyOrders(...)`
   - `ArmyPlanner->ProduceArmyPlan(...)`.
4. Squad stage under mutation batch:
   - `SquadOrderExpander->ExpandSquadOrders(...)`.
5. Unit execution stage under mutation batch:
   - `LastArmyExecutionOrderCount = UnitExecutionPlanner->ExpandUnitExecutionOrders(...)`.
6. `CommandTaskPriorityService->UpdateTaskPriorities(GameStateDescriptor)` when configured.
7. `IntentSchedulingService.DrainReadyIntents(...)` into `IntentBuffer` bounded by `MaxUnitIntentsPerStep`.

## Dispatch Maintenance And Capture Boundary

`UpdateDispatchedSchedulerOrders(const FFrameContext& Frame)` inspects only `UnitExecution` orders in `EOrderLifecycleState::Dispatched` and applies:

- `Completed` when observed completed count increases from `ObservedCountAtDispatch`.
- `Completed` when observed in-construction count increases from `ObservedInConstructionCountAtDispatch`.
- `Completed` when `HasProducerConfirmedDispatchedOrder(...)` succeeds.
- `Aborted` when the actor is missing after dispatch game loop.
- `Aborted` when dispatch confirmation timeout exceeds 96 game loops.

After updates it calls:

- `CompactTerminalOrders()`
- `EndMutationBatch()`

`CaptureNewlyDispatchedSchedulerOrders(const FFrameContext& Frame)` records first dispatch snapshots through `SetOrderDispatchState(...)` for `UnitExecution` orders that are:

- `EOrderLifecycleState::Dispatched`
- `DispatchAttemptCounts == 0`

Captured fields are `CurrentStep`, `Frame.GameLoop`, `GetObservedCountForOrder(...)`, and `GetObservedInConstructionCountForOrder(...)`.

## Execution Telemetry Boundary

`UpdateExecutionTelemetry(const FFrameContext& Frame)` records:

- supply-block transitions from `BuildPlanning.AvailableSupply` and `ObservationPtr->GetFoodCap()` through `UpdateSupplyBlockState(...)`
- mineral-bank transitions (`AvailableMinerals >= 400`) through `UpdateMineralBankState(...)`
- per-actor conflicting intents in `IntentBuffer` through `RecordActorIntentConflict(...)`
- idle marine production conflicts for idle completed `TERRAN_BARRACKS` when marine demand exists
- idle worker production conflicts for idle town halls when worker demand exists
- scheduler deferral events for orders whose `LastDeferralSteps` equal `CurrentStep` and whose `LastDeferralReasons` are not `None`

## Arbiter And Execution Boundary

`FIntentArbiter::Resolve(...)` is the only conflict-resolution boundary before command issue:

- one winner per actor by higher `Priority`, then `EIntentDomain` order
- stable original-buffer order when winner values match
- actor and target validation against current frame state
- one structure-build reservation per actor in a resolve pass
- point target normalization and optional placement/pathing checks

`ExecuteResolvedIntents(...)` is the only command-issue boundary and dispatches by `EIntentTargetKind`:

- `None` -> `Actions()->UnitCommand(actor, ability, queued)`
- `Point` -> `Actions()->UnitCommand(actor, ability, point, queued)`
- `Unit` -> `Actions()->UnitCommand(actor, ability, targetTag, queued)`

## Additional Producer Seams

- `ProduceProductionRallyIntents()`:
  - uses 120-step cadence full refresh
  - immediately refreshes newly created production structures via `PendingProductionRallyStructureTags`
- `ProduceWallGateIntents(...)`:
  - computes desired `EWallGateState`
  - emits transition-only intents through `IWallGateController`
  - records wall threat/open/close events into `ExecutionTelemetry`
- `ProduceWorkerHarvestIntents(...)`:
  - fills under-saturated refineries
  - relieves over-saturated refineries
  - excludes actors already reserved by intents or active scheduler orders
- `ProduceRecoveryIntents(...)`:
  - converts idle workers and idle callbacks into mineral-smart fallback commands
  - falls back to `ATTACK_ATTACK` toward `GetEnemyTargetLocation()` when no mineral patch exists

## Invariants

- `ObservationPtr` must be valid before descriptor, scheduler, telemetry, and execution work.
- `FFrameContext` is the authoritative per-step frame transport.
- Descriptor and planning rebuild always execute before scheduler intent production.
- Telemetry updates execute after all producers and before arbitration.
- `TerranAgent` orchestrates ordering only; composed services own domain decisions.