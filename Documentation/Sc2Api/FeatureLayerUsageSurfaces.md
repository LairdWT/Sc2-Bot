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
