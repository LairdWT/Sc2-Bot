# Telemetry And Game Record Store

## Purpose

This page documents the current telemetry implementation used by the Terran coordinator path, and the currently missing game-record-store layer.

Primary code surfaces:

- `L:\Sc2_Bot\examples\terran\terran.cc`
- `L:\Sc2_Bot\examples\common\telemetry\FAgentExecutionTelemetry.h`
- `L:\Sc2_Bot\examples\common\telemetry\FAgentExecutionTelemetry.cc`
- `L:\Sc2_Bot\tests\test_agent_execution_telemetry.cc`

## Current Runtime Owner

`TerranAgent` owns one `FAgentExecutionTelemetry ExecutionTelemetry` instance and updates it from:

- `OnGameStart()` via `ExecutionTelemetry.Reset()`
- `OnStep()` via `UpdateExecutionTelemetry(const FFrameContext& Frame)`

`PrintAgentState()` reads telemetry counters and prints recent event summaries.

## Telemetry State Model

`FAgentExecutionTelemetry` currently owns:

- supply-block state and duration
- mineral-bank state, duration, and last mineral amount
- total counters:
  - `TotalActorIntentConflictCount`
  - `TotalIdleProductionConflictCount`
  - `TotalSchedulerOrderDeferralCount`
- rolling event buffer: `RecentEvents`

Deduplication caches prevent repeated spam for the same actor/order within cooldown windows:

- `LastActorConflictStepByActor`
- `LastIdleProductionConflictStepByActor`
- `LastSchedulerDeferralStepByOrderId`
- `LastSchedulerDeferralReasonByOrderId`

## Event Contract

`FAgentExecutionTelemetry` appends `FExecutionEventRecord` events for:

- `SupplyBlockedStarted`
- `SupplyBlockedEnded`
- `MineralBankStarted`
- `MineralBankEnded`
- `ActorIntentConflict`
- `IdleProductionStructure`
- `SchedulerOrderDeferred`
- `WallDescriptorInvalid`
- `WallThreatDetected`
- `WallOpened`
- `WallClosed`

Buffer rules:

- cooldown: 120 steps per actor/order key for conflict/deferral classes
- ring size: `MaxRecentEventCountValue = 16`

## Coordinator Integration Order

`TerranAgent::OnStep()` calls telemetry update after all producer stages and before intent resolution:

1. Producer stages populate `IntentBuffer` and scheduler deferral metadata.
2. `UpdateExecutionTelemetry(Frame)` consumes `IntentBuffer` and `CommandAuthoritySchedulingState`.
3. `FIntentArbiter::Resolve(...)` runs after telemetry capture.

This makes telemetry describe production-time conflicts and deferrals for the current frame before resolution drops losing intents.

## Current Detection Rules

`UpdateExecutionTelemetry(...)` records:

- supply blocked when `ObservationPtr->GetFoodCap() < 200` and `BuildPlanning.AvailableSupply == 0`
- mineral banking when `BuildPlanning.AvailableMinerals >= 400`
- actor-intent conflicts when the same actor receives non-matching intents in a single frame
- idle marine production conflicts for idle completed `TERRAN_BARRACKS` when marine demand exists
- idle worker production conflicts for idle town halls when worker demand exists
- scheduler deferral events for orders deferred on the current step

Wall-related events are recorded from other coordinator functions:

- `RecordWallDescriptorInvalid(...)` in `InitializeRampWallDescriptor(...)`
- `RecordWallThreatDetected(...)`, `RecordWallClosed(...)`, `RecordWallOpened(...)` in `ProduceWallGateIntents(...)`

## Test Coverage

`L:\Sc2_Bot\tests\test_agent_execution_telemetry.cc` verifies:

- state transitions for supply-block and mineral-bank conditions
- event generation for all public record methods
- cooldown coalescing for actor conflict, idle production, and scheduler deferral events
- reset behavior for counters and buffers
- preservation of deferral reason and order id fields

## Open Gap: Match Record Store

A persistent match-level telemetry store is not implemented in the current owned Terran path.

There is no current `FMatchRecord` or durable event export surface in:

- `L:\Sc2_Bot\examples\terran`
- `L:\Sc2_Bot\examples\common\telemetry`

Current telemetry is in-memory per run and console-observable only.