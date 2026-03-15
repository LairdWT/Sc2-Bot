# Sc2 Bot Terran Documentation Automation Report

## Run Summary

- RunTimeUtc: 2026-03-15T17:56:24Z
- Scope:
  - `L:\Sc2_Bot\examples\common`
  - `L:\Sc2_Bot\examples\terran`
  - focused supporting tests under `L:\Sc2_Bot\tests`
  - `L:\Sc2_Bot\Documentation\TerranBot`
- Updated docs:
  - `L:\Sc2_Bot\Documentation\TerranBot\TerranAgentCoordinatorPath.md`

## Current Open Findings

1. `L:\Sc2_Bot\Documentation\TerranBot\TelemetryAndGameRecordStore.md` still reflects an active implementation gap: there is no persistent match-level telemetry record store in the owned Terran runtime path; telemetry remains in-memory through `FAgentExecutionTelemetry`.
2. The following pages remain architecture-direction or roadmap-oriented rather than implementation-authoritative for current owned runtime behavior and should be split or rewritten in future passes:
   - `L:\Sc2_Bot\Documentation\TerranBot\ArchitectureOverview.md`
   - `L:\Sc2_Bot\Documentation\TerranBot\ArmyAndSquadDomain.md`
   - `L:\Sc2_Bot\Documentation\TerranBot\BuildPlanningAndBudgeting.md`
   - `L:\Sc2_Bot\Documentation\TerranBot\GameStateDescriptors.md`
   - `L:\Sc2_Bot\Documentation\TerranBot\SpatialFieldsAndHotspots.md`

## Notes

- Verified current coordinator path ordering and boundaries from source:
  - `OnGameStart`
  - `OnStep`
  - `UpdateAgentState`
  - `RebuildObservedGameStateDescriptor`
  - `UpdateDispatchedSchedulerOrders`
  - intent producers (`ProduceSchedulerIntents`, `ProduceProductionRallyIntents`, `ProduceWallGateIntents`, `ProduceWorkerHarvestIntents`, `ProduceRecoveryIntents`)
  - `UpdateExecutionTelemetry`
  - `FIntentArbiter::Resolve`
  - `ExecuteResolvedIntents`
  - `CaptureNewlyDispatchedSchedulerOrders`
- `TerranAgentCoordinatorPath.md` now includes `RebuildExecutionPressureDescriptor(const FFrameContext& Frame)` in both `OnGameStart` and `OnStep` ordering and in the descriptor/planning boundary section.
- Focused supporting evidence reviewed in:
  - `L:\Sc2_Bot\tests\test_command_authority_scheduling.cc`
  - `L:\Sc2_Bot\tests\test_terran_descriptor_pipeline.cc`
  - `L:\Sc2_Bot\tests\test_terran_opening_plan_scheduler.cc`
  - `L:\Sc2_Bot\tests\test_agent_execution_telemetry.cc`
- No source ambiguity was found for this coordinator path in this run.
