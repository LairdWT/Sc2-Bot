# Feature Layer Usage Surfaces

## Purpose

This page documents how feature-layer payloads enter the owned Terran integration path, how `FAgentSpatialMetrics` is derived, and which consumers are active decision inputs versus status-only outputs.

Primary local sources:

- `L:\Sc2_Bot\examples\terran\terran.h`
- `L:\Sc2_Bot\examples\terran\terran.cc`
- `L:\Sc2_Bot\examples\common\agent_framework.h`
- `L:\Sc2_Bot\examples\common\bot_status_models.h`
- `L:\Sc2_Bot\src\sc2api\sc2_interfaces.h`
- `L:\Sc2_Bot\src\sc2api\sc2_coordinator.cc`
- `L:\Sc2_Bot\src\sc2api\sc2_game_settings.h`
- `L:\Sc2_Bot\tests\test_singularity_framework.cc`

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

## FAgentSpatialMetrics Derivation Contract

- `FAgentSpatialMetrics::Update(const FAgentSpatialChannels& Channels)`:
  - calls `Reset()`
  - returns immediately when `Channels.Valid == false`
  - runs `AccumulateChannel(...)` for:
    - `Channels.MapPlayerRelative` into `Map`
    - `Channels.MinimapPlayerRelative` into `Minimap`
  - sets `Valid = true` after both accumulations
- `AccumulateChannel(...)` computes only occupancy summaries (`SelfCount`, `EnemyCount`, `NeutralCount`, centroids, and `Has*` flags).
- `Channels.MinimapHeightMap` is loaded into `FAgentSpatialChannels` but is not currently consumed by `FAgentSpatialMetrics::Update(...)`.

## Consumer Classification (Source-Proven)

### Active Decision Input

- No active `TerranAgent` decision producer currently reads `AgentState.SpatialMetrics` or `AgentState.SpatialChannels` to emit intents.
- `TerranAgent::OnStep()` runs:
  - `UpdateAgentState(Frame)`
  - `RebuildGameStateDescriptor(Frame)`
  - scheduler intent production and execution
- The feature-layer metrics path is updated before planning, but no planner or intent producer in `examples\terran\terran.cc` reads those metric fields as command authority.

### Debug Or Status Helper

- `FAgentState::PrintAgentState()` prints:
  - `SpatialMetrics.Valid`
  - `SpatialMetrics.Minimap.HasEnemy`
  - `SpatialMetrics.Map.HasSelf`
- These fields are currently surfaced as status output only.

### Unresolved Ownership Or Dormant Surface

- `TerranAgent` declares:
  - `DrawFeatureLayer1BPP(...)`
  - `DrawFeatureLayerUnits8BPP(...)`
  - `DrawFeatureLayerHeightMap8BPP(...)`
- No call site in `examples\terran\terran.cc` currently uses these inline helpers.
- `examples\feature_layers.cc` has similarly named free functions for a separate sample agent path; this does not establish a `TerranAgent` owner for the inline helpers.

## API Surfaces Used For Feature Layers

- Observation raw payload access:
  - `ObservationInterface::GetRawObservation() const`
- Observation action mirror (available but not used by `TerranAgent`):
  - `ObservationInterface::GetFeatureLayerActions() const`
- Feature-layer action command API (available but not used by `TerranAgent`):
  - `ActionFeatureLayerInterface`
- Scheduler execution API currently used by `TerranAgent`:
  - `ActionInterface::UnitCommand(...)`

## Feature-Layer Action Mirror Cadence And Authority Boundary

- `ObservationImp::UpdateObservation()` updates `feature_layer_actions_` through `ConvertFeatureLayerActions(response_, feature_layer_actions_)`.
- In `ObservationImp::UpdateObservation()`, `feature_layer_actions_` is reset only when `is_new_frame == true` (`next_game_loop != current_game_loop_`).
- `ConvertFeatureLayerActions(...)` delegates to `ConvertSpatialAction(...)`, which appends to `SpatialActions` collections.
- `ControlImp::IssueEvents(const Tags& commands)` returns `false` and does not call `client_.OnStep()` when
  `observation_imp_->current_game_loop_ == observation_imp_->previous_game_loop`.
- Ownership boundary in coordinator loop:
  - `CallOnStep(Agent* a)` calls `control->IssueEvents(action->Commands())` before both `action->SendActions()` and `action_feature_layer->SendActions()`.
  - `TerranAgent` currently executes through `ActionInterface::UnitCommand(...)`, not `ActionFeatureLayerInterface`.
- Local tests (`tests\test_feature_layer.cc`, `tests\test_actions.cc`) prove that feature-layer actions are mirrored through observation on subsequent loops.
- Terran integration implication: same-loop mirror cardinality is not an active `TerranAgent` decision-surface ambiguity because
  `TerranAgent::OnStep()` is frame-gated by `ControlImp::IssueEvents(...)`.

## Test Coverage Boundary

- `tests\test_singularity_framework.cc` proves channel and metric derivation behavior under synthetic frame data.
- Those tests do not prove planner, scheduler, or command-authority consumption of `FAgentSpatialMetrics` in `TerranAgent`.

## Coordinate Conversion Helper Consumer Boundary

- `FAgentSpatialChannels` exposes:
  - `Point2DI ConvertWorldToMinimap(const Point2D& WorldPosition) const`
  - `Point2DI ConvertWorldToCamera(const Point2D& WorldPosition) const`
- Local source search across `examples\` shows no `TerranAgent` lifecycle callsite for either helper.
- Current conversion helper usage is source-proven only in test support and feature-layer tests:
  - `tests\feature_layers_shared.cc`
  - `tests\test_feature_layer.cc`
  - `tests\test_feature_layer_mp.cc`
  - `tests\test_actions.cc`
  - `tests\test_snapshots.cc`
- Ownership boundary for current Terran integration:
  - conversion helper declarations live on `FAgentSpatialChannels`
  - scheduler producers and intent execution in `examples\terran\terran.cc` do not consume those helper outputs
  - no source-proven bridge currently maps conversion outputs into `TerranAgent` command authority

## Feature-Layer Metric Reset Cadence Boundary

- Selected topic: `SC2-API-FEATURE-LAYER-METRICS-RESET-CADENCE-BOUNDARY`
- `TerranAgent` update cadence:
  - `TerranAgent::OnGameStart()` builds `FFrameContext` and calls `UpdateAgentState(Frame)` once for startup snapshot hydration.
  - `TerranAgent::OnStep()` builds `FFrameContext` each step and calls `UpdateAgentState(Frame)` before descriptor rebuild and scheduler production.
- `FAgentState::Update(const FFrameContext& Frame)` cadence:
  - always calls `SpatialChannels.Update(Frame)` then `SpatialMetrics.Update(SpatialChannels)` when `Frame.Observation` exists.
  - this runs on both startup and each scheduler step in Terran.
- Reset boundary is source-proven and deterministic:
  - `FAgentSpatialChannels::Update(...)` starts with `Reset()`, then returns invalid unless all required feature-layer payload gates pass.
  - `FAgentSpatialMetrics::Update(...)` starts with `Reset()`, then returns invalid when `Channels.Valid == false`.
  - result: stale feature-layer occupancy state is not carried across frames when payloads are missing or malformed.
- Test-backed verification:
  - `tests\test_singularity_framework.cc` verifies valid channel + metric derivation and verifies invalid-input reset behavior where missing required textures keeps channels and metrics invalid.

## Feature-Layer Setup Provenance Boundary

- Selected topic: `SC2-API-FEATURE-LAYER-SETUP-PROVENANCE-BOUNDARY`
- Source-proven setup ownership chain:
  - `Coordinator::SetFeatureLayers(const FeatureLayerSettings& settings)` sets `imp_->interface_settings_.use_feature_layers = true` and copies `imp_->interface_settings_.feature_layer_settings = settings` in `src\sc2api\sc2_coordinator.cc`.
  - `ControlImp::RequestJoinGame(...)` writes those values into `SC2APIProtocol::InterfaceOptions::feature_layer` (`width`, `resolution`, `minimap_resolution`) in `src\sc2api\sc2_client.cc`.
  - `ObservationImp::GetGameInfo()` converts `ResponseGameInfo::options` into `GameInfo::options` via `Convert(response_game_info_ptr->options(), game_info.options)` in `src\sc2api\sc2_proto_to_pods.cc`.
  - `FAgentSpatialChannels::Update(const FFrameContext& Frame)` copies `FeatureLayerSetup = Frame.GameInfo->options.feature_layer` in `examples\common\agent_framework.h`.
- Boundary conclusion:
  - `FAgentSpatialChannels::FeatureLayerSetup` is sourced from `GameInfo::options.feature_layer` (join-game interface contract), not from `RawObservation::feature_layer_data()` textures.
  - If per-frame feature-layer textures are missing, `FAgentSpatialChannels::Update(...)` resets `Valid` to `false`, but the setup provenance remains on the `GameInfo` path.

## Feature-Layer Height-Map Gate Versus Consumer Boundary

- Selected topic: `SC2-API-FEATURE-LAYER-HEIGHTMAP-GATE-CONSUMER-BOUNDARY`
- Ingestion gate in `FAgentSpatialChannels::Update(const FFrameContext& Frame)`:
  - requires `minimap_renders().has_height_map()` before channels can become valid
  - requires `MinimapHeightMap.Load(MinimapLayers.height_map())` to succeed, otherwise `Reset()` and invalid channels
- Derivation boundary in `FAgentSpatialMetrics::Update(const FAgentSpatialChannels& Channels)`:
  - reads `Channels.MapPlayerRelative` and `Channels.MinimapPlayerRelative`
  - does not read `Channels.MinimapHeightMap`
- Terran consumer boundary:
  - no source-proven `TerranAgent::OnStep()` planner, scheduler, or intent execution path currently reads `AgentState.SpatialChannels.MinimapHeightMap`
  - no source-proven `TerranAgent` call path reads a metric derived from `MinimapHeightMap`
- Test boundary:
  - `tests\test_singularity_framework.cc` `SetValidFeatureLayers(...)` includes a height-map payload and verifies channels and metrics valid path
  - current test pass verifies that missing required textures keeps channels invalid, but does not isolate a height-map-only omission case
- Source-proven implication:
  - current channel validity depends on height-map availability even when active Terran decision inputs and derived occupancy metrics do not consume height-map data

## Feature-Layer Setup Versus Channel Dimension Parity Boundary

- Selected topic: `SC2-API-FEATURE-LAYER-SETUP-CHANNEL-DIMENSION-PARITY-BOUNDARY`
- Source-proven setup and payload dimension sources:
  - `ControlImp::RequestJoinGame(...)` writes requested feature-layer setup dimensions into `InterfaceOptions::feature_layer` in `src\sc2api\sc2_client.cc`.
  - `ObservationImp::GetGameInfo()` converts `ResponseGameInfo::options` into `GameInfo::options.feature_layer` via `Convert(..., game_info.options)` in `src\sc2api\sc2_proto_to_pods.cc`.
  - `FAgentSpatialChannels::Update(const FFrameContext& Frame)` loads `MapPlayerRelative`, `MinimapPlayerRelative`, and `MinimapHeightMap` dimensions from `RawObservation::feature_layer_data()` payload images in `examples\common\agent_framework.h`.
- Source-proven boundary in conversion helpers:
  - `FAgentSpatialChannels::ConvertWorldToMinimap(...)` computes pixel scaling from `FeatureLayerSetup.minimap_resolution_x` and `FeatureLayerSetup.minimap_resolution_y`, not from `MinimapPlayerRelative.Width` and `MinimapPlayerRelative.Height`.
  - `FAgentSpatialChannels::ConvertWorldToCamera(...)` computes pixel scaling from `FeatureLayerSetup.map_resolution_x` and `FeatureLayerSetup.map_resolution_y`, not from `MapPlayerRelative.Width` and `MapPlayerRelative.Height`.
- Current validation gap:
  - `FAgentSpatialChannels::Update(...)` validates each channel payload independently through `FSpatialChannel8BPP::Load(...)`.
  - No source-proven parity check currently asserts that loaded payload dimensions match `GameInfo::options.feature_layer` resolution fields before marking channels valid.
- Terran integration impact boundary:
  - `TerranAgent` scheduler and intent execution in `examples\terran\terran.cc` do not currently consume `ConvertWorldToMinimap(...)` or `ConvertWorldToCamera(...)`.
  - Parity risk is currently latent for Terran command authority, but active for any future owner that relies on those conversion helpers as decision input.

## Feature-Layer Conversion Test Matrix Coverage Boundary

- Selected topic: `SC2-API-FEATURE-LAYER-CONVERSION-TEST-MATRIX-COVERAGE-BOUNDARY`
- Source-proven end-to-end conversion coverage:
  - `tests\test_feature_layer.cc` runs a matrix of `FeatureLayerSettings` (`sizeSquare`, `sizeLong`, `sizeTall`) across `kMapEmpty`, `kMapEmptyLong`, and `kMapEmptyTall`.
  - `FeatureLayerTestBot` repeatedly executes `TestCoordinateSystemMinimap` and `TestCoordinateSystemMap` (10 iterations each) and validates converted positions are in bounds and hit occupied cells.
  - `tests\feature_layers_shared.cc` conversion helpers (`ConvertWorldToMinimap(...)`, `ConvertWorldToCamera(...)`) apply the same setup-field scaling model as `FAgentSpatialChannels` conversion helpers in `examples\common\agent_framework.h`.
- Source-proven setup propagation boundary covered by the matrix:
  - `coordinator.SetFeatureLayers(settings)` drives setup into `ControlImp::RequestJoinGame(...)`.
  - `ObservationImp::GetGameInfo()` converts `ResponseGameInfo::options` into `GameInfo::options`.
  - The tests then convert world points using `GameInfo::options.feature_layer` and validate against received feature-layer payloads.
- Remaining coverage gap:
  - No local test currently injects or asserts a setup-to-payload dimension mismatch case where `GameInfo::options.feature_layer` diverges from `RawObservation::feature_layer_data()` image dimensions.
  - Current matrix therefore proves expected parity behavior under normal engine responses, but does not close the explicit mismatch-handling ambiguity.

## Remaining Ambiguities After This Pass
- `FL-001`
  - `TerranAgent` declares `DrawFeatureLayer1BPP(...)`, `DrawFeatureLayerUnits8BPP(...)`, and `DrawFeatureLayerHeightMap8BPP(...)` in `terran.h`.
  - No call path from `TerranAgent::OnStep()` currently uses those helpers.
  - Active draw call sites are source-proven in the separate `examples\feature_layers.cc` sample path, not in `examples\terran\terran.cc`.
  - Follow-up needed: keep as dormant debug hooks with an explicit enable path, or remove from coordinator-facing class surface.

- `FL-003`
  - `FAgentSpatialChannels::ConvertWorldToMinimap(...)` and `FAgentSpatialChannels::ConvertWorldToCamera(...)` are declared in `examples\common\agent_framework.h`.
  - No source-proven `TerranAgent` call path currently consumes those helpers during `OnStep()` scheduling or execution.
  - Follow-up needed: establish an owned Terran decision-input consumer or move these helpers to a test-only/shared utility boundary.

- `FL-004`
  - `FAgentSpatialChannels::Update(...)` requires and loads `MinimapHeightMap` from `minimap_renders().height_map()`.
  - `FAgentSpatialMetrics::Update(...)` does not consume `MinimapHeightMap`, and no source-proven `TerranAgent` decision path currently consumes that channel.
  - Follow-up needed: either add an owned consumer for height-map channel data or relax the channel-validity gate so unused height-map absence does not invalidate all feature-layer channels.

- `FL-005`
  - `FAgentSpatialChannels::Update(...)` marks channels valid after successful payload loads and separately copies `FeatureLayerSetup` from `GameInfo::options.feature_layer`.
  - `ConvertWorldToMinimap(...)` and `ConvertWorldToCamera(...)` scale by setup resolution fields, not loaded channel dimensions.
  - No source-proven parity check currently verifies setup dimensions against loaded `MapPlayerRelative` and `MinimapPlayerRelative` dimensions before conversion helpers are used.
  - Follow-up needed: add a setup-to-payload dimension parity guard in `FAgentSpatialChannels::Update(...)` or define explicit precedence when setup and payload dimensions diverge.

