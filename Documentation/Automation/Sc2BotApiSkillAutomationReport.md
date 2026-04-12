# Sc2 Bot API Skill Automation Report

## Run Summary

- RunTimeUtc: 2026-03-25T16:59:14Z
- CompletedTopicId: SC2-API-QUERY-FAILURE-SENTINEL-INTENT-VALIDATION-BOUNDARY
- UpdatedSurface:
  - L:\Sc2_Bot\Documentation\Sc2Api\ObservationAndFrameContext.md
- ChangeSurface: Sc2ApiDoc
- ClosedFindingIds: []

## Current Open Findings

- FL-001: Terran feature-layer draw helper ownership remains open. `DrawFeatureLayer1BPP(...)`, `DrawFeatureLayerUnits8BPP(...)`, and `DrawFeatureLayerHeightMap8BPP(...)` are declared on `TerranAgent` in `examples\terran\terran.h`, while active draw call sites are source-proven in `examples\feature_layers.cc`.
- FL-003: Coordinate conversion helper ownership remains open. `FAgentSpatialChannels::ConvertWorldToMinimap(...)` and `FAgentSpatialChannels::ConvertWorldToCamera(...)` are defined in `examples\common\agent_framework.h`, but no source-proven `TerranAgent::OnStep()` scheduler or execution path currently consumes these helper outputs.
- FL-004: Height-map channel consumer boundary remains open. `FAgentSpatialChannels::Update(...)` requires `minimap_renders().height_map()` and `MinimapHeightMap.Load(...)`, but no source-proven `TerranAgent` decision path or `FAgentSpatialMetrics::Update(...)` consumer currently reads `MinimapHeightMap`.
- FL-005: Setup-to-payload dimension parity boundary remains open. `FAgentSpatialChannels::Update(...)` loads channel dimensions from payload images and copies `FeatureLayerSetup` from `GameInfo::options.feature_layer`, while conversion helpers scale from setup resolution fields with no source-proven parity check against loaded channel dimensions.

## Notes

- Candidate set contained no open ledger topics; this run selected one newly discovered coordinator-surface topic per run rules.
- Repetition penalty for selected topic: 0 (`TopicId +0`, `Domain +0`, `TargetPath +0`).
- No skill folder changes were made in this run.
