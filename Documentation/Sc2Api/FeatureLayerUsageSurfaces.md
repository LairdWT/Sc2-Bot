# Feature Layer Usage Surfaces

## Purpose

This page documents how feature-layer payloads enter the owned Terran integration path and where those surfaces stop short of gameplay decision authority.

Primary local sources:

- `L:\Sc2_Bot\examples\terran\terran.h`
- `L:\Sc2_Bot\examples\terran\terran.cc`
- `L:\Sc2_Bot\examples\common\agent_framework.h`
- `L:\Sc2_Bot\examples\common\bot_status_models.h`
- `L:\Sc2_Bot\src\sc2api\sc2_interfaces.h`
- `L:\Sc2_Bot\src\sc2api\sc2_coordinator.cc`
- `L:\Sc2_Bot\src\sc2api\sc2_game_settings.h`

## Owned Ingestion Path

- `Coordinator::SetFeatureLayers(const FeatureLayerSettings& settings)` enables feature-layer output before launch and stores settings into `interface_settings_`.
- `FFrameContext::Create(const ObservationInterface* ObservationPtr, QueryInterface* QueryPtr, uint64_t CurrentStepValue)` copies:
  - `RawObservation` from `ObservationPtr->GetRawObservation()`
  - `GameInfo` from `ObservationPtr->GetGameInfo()`
  - `CameraWorld` from `ObservationPtr->GetCameraPos()`
- `FAgentState::Update(const FFrameContext& Frame)` calls:
  - `SpatialChannels.Update(Frame)`
  - `SpatialMetrics.Update(SpatialChannels)`

## FAgentSpatialChannels Feature-Layer Contract

`FAgentSpatialChannels::Update(const FFrameContext& Frame)` requires all of the following:

- `Frame.IsValid()`
- `Frame.RawObservation != nullptr`
- `Frame.RawObservation->has_feature_layer_data()`
- `feature_layer_data().has_renders()`
- `feature_layer_data().has_minimap_renders()`
- `renders().has_player_relative()`
- `minimap_renders().has_player_relative()`
- `minimap_renders().has_height_map()`

Loaded channels:

- `MapPlayerRelative` from `renders().player_relative()`
- `MinimapPlayerRelative` from `minimap_renders().player_relative()`
- `MinimapHeightMap` from `minimap_renders().height_map()`

Derived setup copied from `Frame.GameInfo`:

- `FeatureLayerSetup = Frame.GameInfo->options.feature_layer`
- `CameraWorld = Frame.CameraWorld`
- `PlayableMin`, `PlayableMax`
- `MapWorldWidth`, `MapWorldHeight`

If any load fails, `Reset()` is called and `Valid` remains false.

## Decision-Authority Boundary In Current Terran Seam

Feature-layer surfaces currently provide state and metrics, not command authority:

- `TerranAgent::OnStep()` flow uses `UpdateAgentState(Frame)` then planner and scheduler paths.
- `TerranAgent` does not call `ActionsFeatureLayer()`.
- `TerranAgent` does not read `ObservationInterface::GetFeatureLayerActions()`.
- Intent execution is issued through `Actions()->UnitCommand(...)` in `ExecuteResolvedIntents(...)`.

Current practical boundary:

- Feature layers feed `FAgentSpatialChannels` and `FAgentSpatialMetrics` for status and derived occupancy signals.
- Build planning, scheduler lifecycle, and resolved command dispatch do not consume feature-layer action streams as authority in this Terran integration seam.

## API Surfaces Used For Feature Layers

- Observation raw payload access:
  - `ObservationInterface::GetRawObservation() const`
- Observation action mirror (available but not used by `TerranAgent`):
  - `ObservationInterface::GetFeatureLayerActions() const`
- Feature-layer action command API (available but not used by `TerranAgent`):
  - `ActionFeatureLayerInterface`

## Remaining Ambiguities After This Pass

- `FL-001`
  - `TerranAgent` declares `DrawFeatureLayer1BPP(...)`, `DrawFeatureLayerUnits8BPP(...)`, and `DrawFeatureLayerHeightMap8BPP(...)` in `terran.h`.
  - No call path from `TerranAgent::OnStep()` currently uses those helpers.
  - Follow-up needed: keep as dormant debug hooks with an explicit enable path, or remove from coordinator-facing class surface.
