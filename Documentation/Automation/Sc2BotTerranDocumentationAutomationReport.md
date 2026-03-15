# Sc2 Bot Terran Documentation Automation Report

## Run Summary

- RunTimeUtc: 2026-03-15T05:59:14Z
- Scope:
  - `L:\Sc2_Bot\examples\common`
  - `L:\Sc2_Bot\examples\terran`
  - focused supporting tests under `L:\Sc2_Bot\tests`
  - `L:\Sc2_Bot\Documentation\TerranBot`
- Updated docs:
  - `L:\Sc2_Bot\Documentation\TerranBot\TerranAgentCoordinatorPath.md`
  - `L:\Sc2_Bot\Documentation\TerranBot\IntentSchedulingAndCommandAuthority.md`
  - `L:\Sc2_Bot\Documentation\TerranBot\TelemetryAndGameRecordStore.md`

## Current Open Findings

1. `TelemetryAndGameRecordStore.md` documents an implementation gap that still exists: there is no persistent match-level telemetry record store in the current owned Terran path, only in-memory `FAgentExecutionTelemetry` state.
2. The following pages remain roadmap-oriented and are not authoritative for current runtime behavior in `TerranAgent`; they need explicit status framing or implementation-aligned rewrites in a future pass to avoid reader confusion:
   - `L:\Sc2_Bot\Documentation\TerranBot\ArchitectureOverview.md`
   - `L:\Sc2_Bot\Documentation\TerranBot\ArmyAndSquadDomain.md`
   - `L:\Sc2_Bot\Documentation\TerranBot\BuildPlanningAndBudgeting.md`
   - `L:\Sc2_Bot\Documentation\TerranBot\GameStateDescriptors.md`
   - `L:\Sc2_Bot\Documentation\TerranBot\SpatialFieldsAndHotspots.md`

## Notes

- No coordinator-path source ambiguity was found for:
  - `OnGameStart`
  - `OnStep`
  - `UpdateAgentState`
  - `RebuildGameStateDescriptor`
  - `UpdateDispatchedSchedulerOrders`
  - scheduler intent production
  - `UpdateExecutionTelemetry`
  - `FIntentArbiter::Resolve`
  - `ExecuteResolvedIntents`
  - `CaptureNewlyDispatchedSchedulerOrders`