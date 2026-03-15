# Sc2 Bot API Skill Automation Report

## Run Summary

- RunTimeUtc: 2026-03-15T07:10:12Z
- CompletedTopicId: SC2-API-SCHEDULER-DEFERRAL-TELEMETRY-COOLDOWN-BOUNDARY
- UpdatedSurface:
  - L:\Sc2_Bot\Documentation\Sc2Api\SchedulerIntentLifecycleAndDispatchCapture.md
- ChangeSurface: Sc2ApiDoc
- ClosedFindingIds: []

## Current Open Findings

- FL-001: Terran feature-layer draw helper ownership remains open. `DrawFeatureLayer1BPP(...)`, `DrawFeatureLayerUnits8BPP(...)`, and `DrawFeatureLayerHeightMap8BPP(...)` are declared on `TerranAgent` in `examples\terran\terran.h`, while active draw call sites are source-proven in `examples\feature_layers.cc`.
- FL-003: Coordinate conversion helper ownership remains open. `FAgentSpatialChannels::ConvertWorldToMinimap(...)` and `FAgentSpatialChannels::ConvertWorldToCamera(...)` are defined in `examples\common\agent_framework.h`, but no source-proven `TerranAgent` `OnStep()` scheduler or execution path currently consumes these helper outputs.

## Notes

- Candidate set contained no open ledger topics; this run selected one newly discovered topic per run rules.
- Applied `sc2-terran-scheduler-intent-lifecycle` workflow and traced `OnStep` -> `UpdateExecutionTelemetry` -> `DrainReadyIntents` -> `SetOrderDispatchState` boundaries from local source and tests.
- No skill folder changes were made in this run.
