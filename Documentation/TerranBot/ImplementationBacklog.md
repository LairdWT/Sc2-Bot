# Terran Bot Implementation Backlog

## Purpose

This backlog breaks Phase 0 and Phase 1 of [ExecutionRoadmap.md](ExecutionRoadmap.md) into concrete implementation work items. It also records which scaffolding items have already been started in code.

## Phase 0: Foundation Corrections

### P0-001: Economic Data Table Safety

- Status: Completed
- Scope:
  replace fixed hard-coded table lengths in `economic_models.h` with sizing tied to `NUM_TERRAN_UNITS`, `NUM_TERRAN_BUILDINGS`, and `NUM_TERRAN_UPGRADES`
- Target files:
  [economic_models.h](/L:/Sc2_Bot/examples/common/economic_models.h)
- Acceptance criteria:
  no economic lookup can index beyond a backing array for any supported Terran type

### P0-002: Safe Terran Type Lookup Contract

- Status: Completed
- Scope:
  introduce safe lookup helpers so invalid Terran type requests do not produce `255` indexing in bot-owned code
- Target files:
  [terran_models.h](/L:/Sc2_Bot/examples/common/terran_models.h)
  [bot_status_models.h](/L:/Sc2_Bot/examples/common/bot_status_models.h)
  [economic_models.h](/L:/Sc2_Bot/examples/common/economic_models.h)
- Acceptance criteria:
  unsupported type lookups fail safely and do not perform unchecked array access

### P0-003: Intent Pathing Validation Hardening

- Status: Completed
- Scope:
  tighten point intent validation so unreachable or invalid pathing results are rejected before execution
- Target files:
  [agent_framework.h](/L:/Sc2_Bot/examples/common/agent_framework.h)
- Acceptance criteria:
  failed pathing queries do not survive arbitration for ground units

### P0-004: Deterministic Structure Placement Anchors

- Status: Completed
- Scope:
  remove worker-relative random structure placement and introduce a service seam for deterministic placement anchors
- Target files:
  [terran.cc](/L:/Sc2_Bot/examples/terran/terran.cc)
  [IBuildPlacementService.h](/L:/Sc2_Bot/examples/common/services/IBuildPlacementService.h)
- Acceptance criteria:
  initial structures place from stable base-relative logic rather than local worker scatter

### P0-005: Army Command Queue Semantics

- Status: Completed
- Scope:
  stop cadence-driven marine control from replacing valid combat orders without need
- Target files:
  [terran.cc](/L:/Sc2_Bot/examples/terran/terran.cc)
  [agent_framework.h](/L:/Sc2_Bot/examples/common/agent_framework.h)
- Acceptance criteria:
  combat control refreshes current intent only when the existing combat order is stale or mismatched

## Phase 1: Domain Scaffolding

### P1-001: Domain Folder Layout

- Status: Started
- Scope:
  create bot-owned folders for descriptors, armies, build planning, planning, services, and spatial state
- Target files:
  [examples/common/descriptors](/L:/Sc2_Bot/examples/common/descriptors)
  [examples/common/armies](/L:/Sc2_Bot/examples/common/armies)
  [examples/common/build_planning](/L:/Sc2_Bot/examples/common/build_planning)
  [examples/common/planning](/L:/Sc2_Bot/examples/common/planning)
  [examples/common/services](/L:/Sc2_Bot/examples/common/services)
  [examples/common/spatial](/L:/Sc2_Bot/examples/common/spatial)
- Acceptance criteria:
  new domain code has a stable location and does not continue growing inside the legacy monolithic headers

### P1-002: Root Descriptor Scaffold

- Status: Started
- Scope:
  introduce the root game state descriptor and the first macro descriptor surface
- Target files:
  [FGameStateDescriptor.h](/L:/Sc2_Bot/examples/common/descriptors/FGameStateDescriptor.h)
  [FMacroStateDescriptor.h](/L:/Sc2_Bot/examples/common/descriptors/FMacroStateDescriptor.h)
  [EGamePlan.h](/L:/Sc2_Bot/examples/common/descriptors/EGamePlan.h)
  [EMacroPhase.h](/L:/Sc2_Bot/examples/common/descriptors/EMacroPhase.h)
- Acceptance criteria:
  a future coordinator can hold one root descriptor with explicit macro state instead of scattering those concerns

### P1-003: Army Domain Scaffold

- Status: Started
- Scope:
  introduce a persistent army domain store and the first army goal enum
- Target files:
  [FArmyDomainState.h](/L:/Sc2_Bot/examples/common/armies/FArmyDomainState.h)
  [EArmyGoal.h](/L:/Sc2_Bot/examples/common/armies/EArmyGoal.h)
- Acceptance criteria:
  the codebase has an explicit persistent army domain that guarantees at least one army anchor

### P1-004: Build Planning Scaffold

- Status: Started
- Scope:
  introduce the initial build planning state and resource reservation surface
- Target files:
  [FBuildPlanningState.h](/L:/Sc2_Bot/examples/common/build_planning/FBuildPlanningState.h)
- Acceptance criteria:
  build planning has a dedicated home for reserved and committed resource state

### P1-005: Spatial Field Scaffold

- Status: Started
- Scope:
  introduce a dedicated spatial field owner for coarse threat and influence grids
- Target files:
  [FSpatialFieldSet.h](/L:/Sc2_Bot/examples/common/spatial/FSpatialFieldSet.h)
- Acceptance criteria:
  future spatial analysis has an authoritative home instead of being embedded directly in Terran behavior logic

### P1-006: Planner Interface Scaffold

- Status: Started
- Scope:
  introduce strategic and planner interfaces that `TerranAgent` can eventually depend on
- Target files:
  [IStrategicDirector.h](/L:/Sc2_Bot/examples/common/planning/IStrategicDirector.h)
  [IArmyPlanner.h](/L:/Sc2_Bot/examples/common/planning/IArmyPlanner.h)
  [IBuildPlanner.h](/L:/Sc2_Bot/examples/common/planning/IBuildPlanner.h)
- Acceptance criteria:
  future planning logic can hang off interfaces instead of concrete branches inside `TerranAgent`

### P1-007: Service Interface Scaffold

- Status: Started
- Scope:
  introduce the first service seams for build placement, expansion selection, and spatial field generation
- Target files:
  [IBuildPlacementService.h](/L:/Sc2_Bot/examples/common/services/IBuildPlacementService.h)
  [IExpansionSelectionService.h](/L:/Sc2_Bot/examples/common/services/IExpansionSelectionService.h)
  [ISpatialFieldBuilder.h](/L:/Sc2_Bot/examples/common/services/ISpatialFieldBuilder.h)
- Acceptance criteria:
  deterministic placement, expansion logic, and field rebuilding can be implemented behind stable interfaces

### P1-008: Command Authority Queue Topology

- Status: Ready
- Scope:
  define the layered command authority queues, ready buffers, and lifecycle enums for strategic, macro, army, squad, and unit work
- Target files:
  [IntentSchedulingAndCommandAuthority.md](/L:/Sc2_Bot/Documentation/TerranBot/IntentSchedulingAndCommandAuthority.md)
  planned bot-owned files under `examples/common/planning`
- Acceptance criteria:
  the bot has a documented queue topology that preserves order, bounds work per frame, and feeds the existing intent seam

### P1-009: Gameplay Telemetry Schema

- Status: Ready
- Scope:
  define the versioned match-record, decision-event, and spatial export schema for future analytics and replay correlation
- Target files:
  [TelemetryAndGameRecordStore.md](/L:/Sc2_Bot/Documentation/TerranBot/TelemetryAndGameRecordStore.md)
  planned bot-owned files under `examples/common`
- Acceptance criteria:
  a future telemetry implementation has a documented authoritative storage shape and export contract

## Recommended Execution Order

1. Complete `P0-001` and `P0-002` before any behavior expansion.
2. Complete `P0-003`, `P0-004`, and `P0-005` so the current bot behavior stops fighting the architecture work.
3. Keep Phase 1 scaffolding compileable while it remains only partially integrated.
4. After Phase 0 is stable, begin replacing direct Terran behavior branches with planner and service dependencies one seam at a time.
