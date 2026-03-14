# Sc2 Bot API Refinement Ledger

## Purpose

This ledger is the candidate source for the `DeepenSc2BotApiSkill` automation.

The automation should:

- choose exactly one topic per run
- prefer open topics over broad sweeps
- use local source as the first authority
- promote recurring clusters into narrow skills only when repetition justifies it

## Topics

| TopicId | Domain | TargetPath | Status | LastUpdatedUtc | Notes |
| --- | --- | --- | --- | --- | --- |
| `SC2-API-TERRANAGENT-COORDINATOR-INTEGRATION` | `TerranAgentIntegration` | `L:\Sc2_Bot\Documentation\Sc2Api\TerranAgentApiIntegration.md` | `Completed` | `2026-03-14T02:16:10Z` | Bootstrap topic. Documented how `TerranAgent` uses `Agent`, `ObservationInterface`, `QueryInterface`, unit and order abstractions, feature-layer inputs, and the intent seam. |
| `SC2-API-OBSERVATION-QUERY-FRAME-CONTEXT` | `ApiFrameAcquisition` | `L:\Sc2_Bot\Documentation\Sc2Api\ObservationAndFrameContext.md` | `Completed` | `2026-03-14T03:41:05Z` | Completed source-backed frame context mapping for `FFrameContext`, `ObservationInterface`, and `QueryInterface` across coordinator and downstream planner paths. |
| `SC2-API-UNIT-ORDER-MODEL-BOUNDARIES` | `ApiUnitsAndOrders` | `L:\Sc2_Bot\Documentation\Sc2Api\UnitAndOrderModelBoundaries.md` | `Open` | `` | Document the exact `Unit`, `UnitOrder`, `Tag`, `ABILITY_ID`, and `UNIT_TYPEID` surfaces that matter for scheduler bookkeeping, recovery behavior, and resolved execution. |
| `SC2-API-FEATURE-LAYER-USAGE-SURFACES` | `FeatureLayers` | `L:\Sc2_Bot\Documentation\Sc2Api\FeatureLayerUsageSurfaces.md` | `Open` | `` | Trace how raw feature-layer data enters `FAgentSpatialChannels` and identify where those surfaces still stop short of gameplay decision authority. |
| `SC2-ECOSYSTEM-DETERMINISTIC-PLACEMENT-PATTERNS` | `EcosystemPlacement` | `L:\Sc2_Bot\Documentation\Ecosystem\DeterministicPlacementPatterns.md` | `Open` | `` | Compare proven deterministic placement approaches only after local placement-service review, then record what cleanly maps onto the current Terran placement seams. |
| `SC2-ECOSYSTEM-SOA-STATE-ACCOUNTING` | `EcosystemDataOrientedDesign` | `L:\Sc2_Bot\Documentation\Ecosystem\SoAStateAccountingPatterns.md` | `Open` | `` | Compare other bot approaches to structure-of-arrays state accounting only when the result can sharpen local scheduler, telemetry, or state rebuild design. |

## Promotion Rule

If the same narrow topic family repeats enough times to justify reusable instructions, the automation may create or deepen one narrow Sc2 skill under `C:\Users\laird\.codex\skills` instead of continuing to accumulate broad narrative notes.



