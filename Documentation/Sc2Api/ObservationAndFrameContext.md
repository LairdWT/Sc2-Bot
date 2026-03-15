# Observation And Frame Context

## Purpose

This page deepens the owned Terran integration surface for `FFrameContext`, `ObservationInterface`, and `QueryInterface`.

Primary local sources:

- `L:\Sc2_Bot\examples\terran\terran.h`
- `L:\Sc2_Bot\examples\terran\terran.cc`
- `L:\Sc2_Bot\examples\common\agent_framework.h`
- `L:\Sc2_Bot\examples\common\bot_status_models.h`
- `L:\Sc2_Bot\examples\common\planning\FTerranEconomyProductionOrderExpander.cc`
- `L:\Sc2_Bot\examples\common\services\FTerranBuildPlacementService.cc`
- `L:\Sc2_Bot\src\sc2api\sc2_client.h`
- `L:\Sc2_Bot\src\sc2api\sc2_client.cc`
- `L:\Sc2_Bot\src\sc2api\sc2_interfaces.h`
- `L:\Sc2_Bot\src\sc2api\sc2_proto_to_pods.cc`
- `L:\Sc2_Bot\src\sc2api\sc2_coordinator.cc`
- `L:\Sc2_Bot\tests\test_feature_layer.cc`
- `L:\Sc2_Bot\tests\test_actions.cc`

## API Acquisition Surface

`TerranAgent` acquires API pointers through `sc2::Client`:

- `const ObservationInterface* Observation() const`
- `QueryInterface* Query()`

`TerranAgent` stores observation as member state:

- `const ObservationInterface* ObservationPtr{nullptr};`

`TerranAgent` creates frame context at both coordinator entrypoints:

- `OnGameStart()`
- `OnStep()`

Construction call:

- `FFrameContext::Create(ObservationPtr, Query(), CurrentStep)`

## FFrameContext Contract

`FFrameContext` fields:

- `const ObservationInterface* Observation`
- `QueryInterface* Query`
- `const SC2APIProtocol::Observation* RawObservation`
- `const GameInfo* GameInfo`
- `Point2D CameraWorld`
- `uint64_t CurrentStep`
- `uint64_t GameLoop`

Population behavior in `FFrameContext::Create(...)`:

- always sets `Observation`, `Query`, and `CurrentStep`
- populates `RawObservation`, `GameInfo`, `CameraWorld`, and `GameLoop` only when `ObservationPtr != nullptr`

Validity gate:

- `bool IsValid() const` requires `Observation != nullptr && GameInfo != nullptr`

## ObservationInterface Usage In Owned Terran Flow

Coordinator and state-update paths read from `ObservationInterface` for:

- unit snapshots via `GetUnits(...)`
- per-tag lookup via `GetUnit(Tag)`
- economy counts via `GetMinerals()`, `GetVespene()`, `GetFoodUsed()`, `GetFoodCap()`
- map and start metadata via `GetGameInfo()` and `GetStartLocation()`
- frame timing via `GetGameLoop()`
- feature-layer payload access via `GetRawObservation()`

Downstream surfaces that directly depend on `Frame.Observation`:

- `FAgentState::Update(const FFrameContext& Frame)`
- `FTerranEconomyProductionOrderExpander::ExpandEconomyAndProductionOrders(...)`
- worker assignment and refinery commitment helpers in `terran.cc`

Feature-layer action mirror surface on `ObservationInterface`:

- `const SpatialActions& GetFeatureLayerActions() const`
- `ObservationImp::UpdateObservation()` resets `feature_layer_actions_` only when `is_new_frame` is true (`next_game_loop != current_game_loop_`), then always calls `ConvertFeatureLayerActions(response_, feature_layer_actions_)`.
- `ConvertFeatureLayerActions(...)` appends into `SpatialActions` collections through `ConvertSpatialAction(...)`.
- In stepped mode, `CoordinatorImp::StepAgents()` calls `ControlImp::Step(...)` then `ControlImp::WaitStep()`, and `WaitStep()` calls `GetObservation()` once per step request.
- In realtime mode, `CoordinatorImp::StepAgentsRealtime()` calls `control->GetObservation()` directly each update, so multiple observations can occur without a forced game-loop advance.

## QueryInterface Usage In Owned Terran Flow

Coordinator seam usage:

- `OnGameStart()` computes expansion points through `search::CalculateExpansionLocations(ObservationPtr, Query())`

Frame-distributed usage:

- `FIntentArbiter::ValidateAndNormalize(...)`
  - placement checks through `Frame.Query->Placement(...)`
  - path checks through `Frame.Query->PathingDistance(...)`
- `FTerranEconomyProductionOrderExpander.cc`
  - placement feasibility gates for structure and expansion selection
  - guarded by explicit `FrameValue.Query == nullptr` checks
- `FTerranBuildPlacementService.cc`
  - runtime placement validation path uses `FrameValue.Query->Placement(...)` when query mode is active

## Ownership And Responsibility Boundary

API-owned responsibility:

- deliver lifecycle callbacks
- deliver frame observation snapshots and query services

Bot-owned responsibility:

- build `FFrameContext`
- gate null and validity cases
- decide when query-backed placement or pathing checks are mandatory
- transform raw feature-layer payloads into `FAgentSpatialChannels` and derived metrics

## Source-Backed Constraints

- `OnGameStart()` and `OnStep()` hard-fail early when `Observation()` returns null.
- `Query()` is optional at acquisition time; downstream code decides whether to hard-require it.
- Placement and pathing validation paths explicitly reject intents or orders when `QueryInterface` is required but missing.
- Feature-layer ingestion requires both `Frame.IsValid()` and `Frame.RawObservation->has_feature_layer_data()`.
- `GetFeatureLayerActions()` is a mirror of action data carried by observation updates, not a command authority surface for `TerranAgent` execution.

## Remaining Ambiguities After This Pass

- `FL-002`
  - `ObservationImp::UpdateObservation()` clears `feature_layer_actions_` only when `is_new_frame` is true, while `ConvertFeatureLayerActions(...)` appends entries.
  - Local source proves reset and append boundaries but does not prove server-side semantics for repeated same-loop `ResponseObservation::actions` payloads.
  - Existing tests validate next-loop action visibility, but do not assert same-loop cardinality under repeated `GetObservation()` calls in realtime cadence.
