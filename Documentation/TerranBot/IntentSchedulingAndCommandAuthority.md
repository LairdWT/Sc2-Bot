# Intent Scheduling And Command Authority

## Purpose

This document describes the next scheduling layer above the current `FIntentBuffer` and `FIntentArbiter`.

The design target is:

- explicit command authority by layer
- bounded work per frame
- data-oriented authoritative storage
- stable order preservation when preparation and execution lifecycles differ

The primary reference patterns for this direction are the installed `queue-orchestration`, `structure-of-arrays`, `data-oriented-models`, and `interface-patterns` skills.

## Authority Layers

The intended command authority hierarchy is:

- `Agent`
- `StrategicDirector`
- `EconomyAndProduction`
- `Army`
- `Squad`
- `UnitExecution`

Rules:

- higher layers may emit goals and requests for lower layers
- lower layers may not mutate higher-level goals directly
- no layer outside `UnitExecution` should issue SC2 commands directly
- `TerranAgent` remains the thin consumer that executes only resolved intents

## Queue Topology

The queue model should reuse the `queue-orchestration` pattern instead of one queue doing every job.

Planned queues and buffers:

- `StrategicOrderQueue`
  authoritative order of high-level plan changes
- `PlanningProcessQueue`
  work awaiting descriptor rebuild or planner expansion
- `ArmyOrderQueue`
  army-level orders awaiting squad decomposition
- `SquadOrderQueue`
  squad-level orders awaiting unit assignment or local planning
- `ReadyIntentBuffer`
  validated unit-intent slices ready for arbitration
- `CompletedOrderBuffer`
  completed or aborted work kept briefly for auditing and telemetry

Each queue should have bounded per-frame processing limits such as:

- `MaxStrategicOrdersPerStep`
- `MaxArmyOrdersPerStep`
- `MaxSquadOrdersPerStep`
- `MaxUnitIntentsPerStep`

## Lifecycle State

The scheduling layer should use explicit enums instead of unrelated booleans.

Planned enums:

- `ECommandAuthorityLayer`
- `EOrderLifecycleState`
- `EPlanningProcessorState`
- `EIntentPlaybackState`

Planned lifecycle states:

- `Queued`
- `Preprocessing`
- `Ready`
- `Dispatched`
- `Completed`
- `Aborted`
- `Expired`

## Authoritative Data Shape

The storage should follow a structure-of-arrays layout.

Planned authoritative columns:

- `OrderIds`
- `ParentOrderIds`
- `SourceLayers`
- `LifecycleStates`
- `PriorityValues`
- `CreationSteps`
- `DeadlineSteps`
- `OwningArmyIndices`
- `OwningSquadIndices`
- `ActorTags`
- `AbilityIds`
- `TargetKinds`
- `TargetPoints`
- `TargetUnitTags`

Derived views should remain index-based:

- ready strategic order indices
- army order indices by army
- squad order indices by squad
- unit execution indices by actor

## Interface Boundaries

The queue owners should be exposed through interfaces, not concrete cross-casts.

Planned interfaces:

- `IIntentSchedulingService`
- `IArmyOrderExpander`
- `ISquadOrderExpander`
- `IUnitExecutionPlanner`

The strategic director and planners should only submit orders through these interfaces. They should not know the concrete storage layout.

## Relationship To The Current Intent System

The current intent seam remains valid:

- planners and scheduling services submit work
- the scheduling system expands and stages unit-ready work
- `ReadyIntentBuffer` feeds `FIntentBuffer`
- `FIntentArbiter` resolves conflicts and validates targets
- `TerranAgent` executes only resolved intents

This preserves the current execution path while allowing the planning stack above it to become layered and persistent.

## Immediate Implementation Direction

The first implementation should stay limited:

- one authoritative scheduling owner
- one queue for army orders
- one queue for squad orders
- one ready buffer for unit intents
- explicit reset that clears all queues and ready buffers together

That is enough to validate the pattern before adding replay or telemetry coupling.
