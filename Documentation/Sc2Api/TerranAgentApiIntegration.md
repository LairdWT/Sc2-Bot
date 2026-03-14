# TerranAgent API Integration

## Purpose

This page documents how the owned Terran bot integrates with the checked-in SC2 API at the current coordinator seam.

Primary integration sources:

- `L:\Sc2_Bot\examples\terran\terran.h`
- `L:\Sc2_Bot\examples\terran\terran.cc`
- `L:\Sc2_Bot\examples\common\agent_framework.h`
- `L:\Sc2_Bot\src\sc2api\sc2_agent.h`
- `L:\Sc2_Bot\src\sc2api\sc2_interfaces.h`
- `L:\Sc2_Bot\src\sc2api\sc2_unit.h`

## Agent Lifecycle Surface

`TerranAgent` derives from `sc2::Agent`.

That inheritance provides the callback-based runtime contract used by the coordinator:

- `OnGameStart()`
- `OnStep()`
- `OnGameEnd()`
- `OnUnitIdle(const Unit* UnitPtr)`
- `OnUnitCreated(const Unit* UnitPtr)`

The local Terran bot does not replace the API lifecycle. It specializes it by filling those callbacks with bot-owned orchestration.

## Observation And Query Acquisition

The API entrypoints used most directly by `TerranAgent` are:

- `Observation()`
- `Query()`

Those pointers are wrapped into `FFrameContext` through:

- `FFrameContext::Create(const ObservationInterface* ObservationPtr, QueryInterface* QueryPtr, uint64_t CurrentStepValue)`

`FFrameContext` is the bot-owned integration bridge between the callback layer and the planning stack. It carries:

- `const ObservationInterface* Observation`
- `QueryInterface* Query`
- `const SC2APIProtocol::Observation* RawObservation`
- `const GameInfo* GameInfo`
- `Point2D CameraWorld`
- `uint64_t CurrentStep`
- `uint64_t GameLoop`

This wrapper keeps the downstream Terran code from repeatedly re-fetching each API surface independently.

## ObservationInterface Usage

The current coordinator path relies on these `ObservationInterface` behaviors:

- `GetUnits(...)`
  - fetches self, enemy, or neutral unit sets
- `GetRawObservation()`
  - exposes raw protocol feature-layer data
- `GetGameInfo()`
  - supplies map bounds, feature-layer setup, and other static match metadata
- `GetCameraPos()`
  - seeds `FFrameContext::CameraWorld`
- `GetGameLoop()`
  - seeds `FFrameContext::GameLoop`
- `GetStartLocation()`
  - anchors placement context and expansion reasoning
- `GetUnit(Tag)`
  - resolves target units when validating worker commitments and orders

The owned Terran code treats `ObservationInterface` as the authoritative live-state read surface.

## QueryInterface Usage

`QueryInterface` is acquired at the same seam and is currently consumed in two ways:

- directly through `search::CalculateExpansionLocations(ObservationPtr, Query())` during `OnGameStart()`
- indirectly through downstream planner, scheduling, and placement services that receive `FFrameContext`

The coordinator itself stays narrow. It obtains the query surface once, puts it in `FFrameContext`, and lets focused subsystems perform the actual placement or pathing work.

## Unit, Order, Tag, And Ability Abstractions

The integration seam depends on the checked-in unit and order model rather than raw protocol structs.

Frequently used abstractions include:

- `Unit`
- `Units`
- `UnitOrder`
- `Tag`
- `ABILITY_ID`
- `UNIT_TYPEID`
- `Point2D`

Examples in the current Terran flow:

- `OnUnitIdle(const Unit* UnitPtr)` checks `UnitPtr->unit_type.ToType()`
- recovery and harvest logic reads `WorkerUnitValue.orders`
- scheduler and execution logic compare `ABILITY_ID` and `UNIT_TYPEID`
- commitment checks compare `target_unit_tag`
- lookup helpers resolve concrete `Unit*` values from stored `Tag` values

This means the Terran bot is already using the higher-level C++ object model exposed by the repository, not the protocol layer directly.

## Feature-Layer Integration

The current feature-layer integration path is:

1. `FFrameContext::Create(...)` stores `RawObservation`.
2. `FAgentSpatialChannels::Update(const FFrameContext& Frame)` reads:
   - `Frame.RawObservation->feature_layer_data()`
   - map `player_relative`
   - minimap `player_relative`
   - minimap `height_map`
3. The loaded channels are normalized into bot-owned `FSpatialChannel8BPP` and `FAgentSpatialChannels` state.

The API responsibility ends at providing feature-layer payloads and setup data. The owned bot code is responsible for converting those payloads into planner-friendly state.

## Intent Seam Versus Direct Commands

One of the most important API integration decisions in the current code is the explicit intent seam.

The flow is:

- bot-owned producers create `FUnitIntent` records
- `FIntentArbiter` resolves those records
- `ExecuteResolvedIntents(...)` performs final command dispatch

This matters because the SC2 API command surface is not called opportunistically throughout the planning code. The coordinator concentrates final execution after arbitration.

## Current Integration Boundaries

The current boundary between checked-in API code and bot-owned code is:

- checked-in API code provides lifecycle, observation, query, unit, order, and map abstractions
- bot-owned code builds descriptors, chooses plans, schedules work, produces intents, records telemetry, and executes resolved commands

`TerranAgent` is the bridge. It is the first owned type that has a complete view of both sides.

## Immediate Follow-On Topics

The next source-backed API topics that branch directly from this page are:

- `ObservationInterface` and `QueryInterface` usage patterns across `FFrameContext`
- unit and order model boundaries that matter for scheduler bookkeeping
- feature-layer usage surfaces and current gaps between loaded data and gameplay decisions
