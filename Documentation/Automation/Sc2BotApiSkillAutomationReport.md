# Sc2 Bot API Skill Automation Report

## Run Summary

- RunTimeUtc: 2026-03-15T11:13:13Z
- CompletedTopicId: SC2-API-OBSERVATION-UNIT-SNAPSHOT-POINTER-LIFETIME-BOUNDARY
- UpdatedSurface:
  - L:\Sc2_Bot\Documentation\Sc2Api\ObservationAndFrameContext.md
- ChangeSurface: Sc2ApiDoc
- ClosedFindingIds: []

## Current Open Findings

- FL-001: Terran feature-layer draw helper ownership remains open. DrawFeatureLayer1BPP(...), DrawFeatureLayerUnits8BPP(...), and DrawFeatureLayerHeightMap8BPP(...) are declared on TerranAgent in examples\terran\terran.h, while active draw call sites are source-proven in examples\feature_layers.cc.
- FL-003: Coordinate conversion helper ownership remains open. FAgentSpatialChannels::ConvertWorldToMinimap(...) and FAgentSpatialChannels::ConvertWorldToCamera(...) are defined in examples\common\agent_framework.h, but no source-proven TerranAgent OnStep() scheduler or execution path currently consumes these helper outputs.
- FL-004: Height-map channel consumer boundary remains open. FAgentSpatialChannels::Update(...) requires minimap_renders().height_map() and MinimapHeightMap.Load(...), but no source-proven TerranAgent decision path or FAgentSpatialMetrics::Update(...) consumer currently reads MinimapHeightMap.

## Notes

- Candidate set contained no open ledger topics; this run selected one newly discovered topic per run rules.
- Verified local-source observation snapshot and pointer-lifetime boundaries without changing unresolved feature-layer ownership findings.
- No skill folder changes were made in this run.

