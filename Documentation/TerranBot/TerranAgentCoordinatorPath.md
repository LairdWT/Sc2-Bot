# TerranAgent Coordinator Path

## Purpose

This document captures the current coordinator path implemented by `TerranAgent` in:

- `L:\Sc2_Bot\examples\terran\terran.h`
- `L:\Sc2_Bot\examples\terran\terran.cc`

The goal is to keep the orchestration seam explicit. `TerranAgent` owns sequencing and execution, while builders, planners, schedulers, services, and telemetry types own focused domain work.

## Current Entry Points

`TerranAgent` derives from `sc2::Agent` and currently owns these runtime callbacks:

- `OnGameStart()`
- `OnStep()`
- `OnGameEnd()`
- `OnUnitIdle(const Unit* UnitPtr)`
- `OnUnitCreated(const Unit* UnitPtr)`

`OnGameStart()` and `OnStep()` are the coordinator entry points. The two unit callbacks only feed recovery and rally bookkeeping.

## Game Start Path

`OnGameStart()` performs one-time coordinator setup in this order:

1. Reset `CurrentStep`.
2. Cache `Observation()` into `ObservationPtr`.
3. Query expansion locations through `search::CalculateExpansionLocations(ObservationPtr, Query())`.
4. Reset long-lived runtime containers:
   - `GameStateDescriptor`
   - `ExecutionTelemetry`
   - `PendingProductionRallyStructureTags`
   - `CurrentWallGateState`
5. Build one `FFrameContext` through `FFrameContext::Create(ObservationPtr, Query(), CurrentStep)`.
6. Call:
   - `UpdateAgentState(Frame)`
   - `InitializeRampWallDescriptor(Frame)`
   - `InitializeMainBaseLayoutDescriptor(Frame)`
   - `RebuildGameStateDescriptor(Frame)`
   - `PrintAgentState()`

The start path establishes descriptor and placement state before the first live step executes intent production.

## Per-Step Coordinator Order

`OnStep()` is the authoritative frame pipeline. The current order is:

1. Increment `CurrentStep`.
2. Refresh `ObservationPtr` from `Observation()`.
3. Create `FFrameContext`.
4. Rebuild live state through `UpdateAgentState(Frame)`.
5. Rebuild derived planning state through `RebuildGameStateDescriptor(Frame)`.
6. Advance scheduler bookkeeping through `UpdateDispatchedSchedulerOrders(Frame)`.
7. Reset `IntentBuffer`.
8. Produce intents in this fixed order:
   - `ProduceSchedulerOpeningIntents(Frame)`
   - `ProduceWallGateIntents(Frame)`
   - `ProduceWorkerHarvestIntents(Frame)`
   - `ProduceRecoveryIntents(Frame)`
   - `ProduceArmyIntents(Frame)`
9. Record frame execution telemetry through `UpdateExecutionTelemetry(Frame)`.
10. Resolve intents through `IntentArbiter.Resolve(Frame, AgentState.UnitContainer, IntentBuffer)`.
11. Execute only resolved intents through `ExecuteResolvedIntents(Frame, ResolvedIntents)`.
12. Capture newly dispatched scheduler orders through `CaptureNewlyDispatchedSchedulerOrders(Frame)`.
13. Print status every 120 steps.

This is the coordinator contract that future documentation and refactors should preserve unless the execution architecture is intentionally changed.

## Direct Domain Seams

`TerranAgent` currently composes these focused domain types directly:

- Descriptor rebuild:
  - `FTerranGameStateDescriptorBuilder`
  - `IGameStateDescriptorBuilder`
- Strategic and planning seams:
  - `FDefaultStrategicDirector`
  - `IStrategicDirector`
  - `FTerranTimingAttackBuildPlanner`
  - `IBuildPlanner`
  - `FTerranArmyPlanner`
  - `IArmyPlanner`
- Scheduling and expansion:
  - `FCommandAuthorityProcessor`
  - `FTerranEconomyProductionOrderExpander`
  - `IEconomyProductionOrderExpander`
  - `FIntentSchedulingService`
- Placement and wall control:
  - `FTerranRampWallController`
  - `IWallGateController`
  - `FTerranBuildPlacementService`
  - `IBuildPlacementService`
- Telemetry:
  - `FAgentExecutionTelemetry`
- Final execution seam:
  - `FIntentBuffer`
  - `FIntentArbiter`
  - `FUnitIntent`

The coordinator owns ordering between these seams. The seams themselves own plan derivation, descriptor rebuild, queue draining, wall behavior, placement reasoning, and telemetry detail.

## Rebuild And Planning Path

`RebuildGameStateDescriptor(const FFrameContext& Frame)` is the coordinator handoff into the planning stack.

The current order inside that function is:

1. `GameStateDescriptorBuilder->RebuildGameStateDescriptor(...)`
2. `StrategicDirector->UpdateGameStateDescriptor(GameStateDescriptor)`
3. `BuildPlanner->ProduceBuildPlan(GameStateDescriptor, GameStateDescriptor.BuildPlanning)`
4. `ArmyPlanner->ProduceArmyPlan(GameStateDescriptor, GameStateDescriptor.ArmyState)`
5. `UpdateRallyAnchor()`

That order matters:

- descriptor rebuild happens before any strategic or planner mutation
- strategic direction sets the high-level plan before build and army planners consume the descriptor
- rally anchor selection happens after placement and planning state are available

## Intent Production Responsibilities

The intent producers currently divide responsibility like this:

- `ProduceSchedulerOpeningIntents(Frame)`
  - advances `FCommandAuthorityProcessor`
  - expands economy and production orders through `FTerranEconomyProductionOrderExpander`
  - drains ready intents from `FIntentSchedulingService`
- `ProduceWallGateIntents(Frame)`
  - evaluates desired wall state
  - emits wall open or close intents only on transitions
  - records wall transition telemetry
- `ProduceWorkerHarvestIntents(Frame)`
  - balances refinery fill and relief work
  - avoids conflicting with active scheduler orders and existing intents
- `ProduceRecoveryIntents(Frame)`
  - repairs idle worker behavior when no higher-priority intent exists
- `ProduceArmyIntents(Frame)`
  - handles production rally behavior
  - can fall back to `AllMarinesAttack()`

The coordinator does not send commands during these stages. It only fills `IntentBuffer`.

## Execution Boundary

The execution boundary is explicit:

- producers create `FUnitIntent` records
- `FIntentArbiter` resolves conflicts and invalid combinations
- `ExecuteResolvedIntents(...)` is the only step that issues final commands

This boundary is one of the most important architectural constraints in the current Terran bot. Future planning or service work should feed the intent seam rather than bypassing it.

## Runtime Invariants

The current coordinator path relies on these invariants:

- `ObservationPtr` must be valid before state update, descriptor rebuild, or execution work begins
- `FFrameContext` is the per-frame transport for observation, query, raw observation, game info, camera, and loop values
- descriptor rebuild happens before scheduler drain and planner-owned intent production
- telemetry update happens before resolved intents are executed
- `TerranAgent` owns orchestration order, not the detailed planning rules inside the composed services

## Immediate Follow-On Documentation

The next documentation deepening steps that branch directly from this coordinator path are:

- `FTerranGameStateDescriptorBuilder` and the descriptor rebuild contract
- `FCommandAuthorityProcessor` plus `FIntentSchedulingService`
- `FTerranEconomyProductionOrderExpander`
- `FTerranBuildPlacementService`
- `FAgentExecutionTelemetry`
