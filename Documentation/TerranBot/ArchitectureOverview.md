# Terran Bot Architecture Overview

## Purpose

This document defines the target runtime architecture for the Terran bot. The goal is to turn the current monolithic control flow into a layered system that can:

- describe the game state with stable, testable descriptors
- express strategic goals and build goals explicitly
- coordinate multiple armies and squads
- evaluate map state and threat fields
- emit deterministic intents through the existing arbitration layer

## Architectural Direction

`TerranAgent` should become a thin coordinator. It should gather frame inputs, trigger state rebuilds, ask planners for goals and intents, run arbitration, and execute resolved commands.

The real behavior should live in focused domains:

- descriptor builders
- strategic direction
- build planning and budgeting
- army and squad coordination
- spatial field generation
- placement and expansion services

## Runtime Layers

### Frame Input Layer

Inputs originate from the current observation, query interface, game info, and feature layer data that are already available through the SC2 API.

Primary responsibilities:

- collect raw units, structures, resources, map visibility, and feature layer data
- normalize current-frame data into authoritative domain state
- keep no strategic branching in this layer

### Authoritative State Layer

This layer owns the persistent, frame-to-frame data-oriented stores.

Planned domain stores:

- `FObservationDomainState`
- `FEconomyDomainState`
- `FTechDomainState`
- `FMapDomainState`
- `FArmyDomainState`
- `FBuildPlanningState`
- `FSpatialFieldSet`

These stores should be the only source of truth. Descriptor structs are rebuilt from them and must not become competing state owners.

### Descriptor Layer

This layer reconstructs high-level views over the authoritative stores.

Examples:

- `FGameStateDescriptor`
- `FMacroStateDescriptor`
- `FThreatStateDescriptor`
- `FMapStateDescriptor`
- `FArmyDescriptor`
- `FSquadDescriptor`
- `FBuildPlanningDescriptor`

This layer exists for planning clarity, logging, testing, and debugging. It should make the current bot state readable without requiring higher-level systems to query raw storage columns directly.

### Planning Layer

This layer chooses goals and desired behavior.

Planned interfaces:

- `IStrategicDirector`
- `IEconomyPlanner`
- `IBuildPlanner`
- `IProductionPlanner`
- `IArmyPlanner`
- `ISquadCoordinator`
- `IUpgradePlanner`

The strategic director should set the current game plan, macro phase, and high-level priorities. Lower-level planners should translate those priorities into concrete packages, squad assignments, and intents.

### Spatial Services Layer

This layer interprets the map and feature-derived data.

Planned interfaces:

- `ISpatialFieldBuilder`
- `IThreatEvaluationService`
- `IBuildPlacementService`
- `IExpansionSelectionService`
- `IPathingAssessmentService`

This layer should provide:

- buildable slots and wall-off placement
- natural, third, and fourth expansion scoring
- threat, safety, and opportunity fields
- staging areas and retreat paths

### Intent Layer

The current intent system in `agent_framework.h` is the correct execution seam and should remain central.

Planned responsibilities:

- planners produce intents without issuing commands directly
- `FIntentBuffer` accumulates produced intents
- `FIntentArbiter` resolves conflicts and validates targets
- `TerranAgent` executes only resolved intents

## Producer, Accumulator, Consumer Mapping

The planned architecture aligns to Producer, Accumulator, Consumer:

- Producers: strategic director, planners, squad coordinator, spatial builders
- Accumulators: authoritative state stores, resource budget ledger, build package store, squad assignment store
- Consumers: intent arbiter, command executor, status reporting, tests

## Command Authority Direction

The next scheduling layer should follow the `queue-orchestration` and `structure-of-arrays` skills rather than storing ad hoc pending state on each gameplay container.

The intended authority hierarchy is:

- agent and strategic direction
- economy, production, and build planning
- army coordination
- squad coordination
- unit execution

Each layer should be allowed to produce work for lower layers, but not issue commands directly outside its own responsibility. When a layer needs preparation work that has a different lifecycle from playback order, it should use:

- an authoritative order queue
- a preprocess or expansion queue
- a ready buffer
- explicit processor and playback enums

The current `FIntentBuffer` and `FIntentArbiter` remain the final execution seam. Future queue work should feed them rather than bypassing them.

## Telemetry Direction

The architecture should also reserve an explicit home for long-lived gameplay records:

- per-match descriptors
- per-frame or per-decision snapshots
- outcome and matchup labels
- spatial exports such as aggregated heatmaps

This data should be versioned and separated from live gameplay state. The live bot should never depend on the exported telemetry files to function in a current match.

## Planned Source Layout

This document does not lock file names yet, but the implementation should trend toward these bot-owned areas:

- `examples/common/descriptors`
- `examples/common/armies`
- `examples/common/build_planning`
- `examples/common/spatial`
- `examples/common/planning`
- `examples/common/services`
- `examples/terran`

Every new enum, interface, struct, and helper type should live in its own file.

## Immediate Architectural Constraint

Before higher-level behavior is expanded, the current foundation issues should be corrected:

- unsafe economic table sizing and indexing
- invalid index fallback behavior
- weak pathing validation
- worker-relative random building placement
- repeated queued attack command accumulation
