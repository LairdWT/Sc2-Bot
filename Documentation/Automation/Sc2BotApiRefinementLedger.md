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
| `SC2-API-UNIT-ORDER-MODEL-BOUNDARIES` | `ApiUnitsAndOrders` | `L:\Sc2_Bot\Documentation\Sc2Api\UnitAndOrderModelBoundaries.md` | `Completed` | `2026-03-14T05:40:16Z` | Completed source-backed boundary mapping for `Unit`, `UnitOrder`, `Tag`, `ABILITY_ID`, and `UNIT_TYPEID` across scheduler bookkeeping, worker-recovery classification, and resolved `ActionInterface::UnitCommand` execution. |
| `SC2-API-FEATURE-LAYER-USAGE-SURFACES` | `FeatureLayers` | `L:\Sc2_Bot\Documentation\Sc2Api\FeatureLayerUsageSurfaces.md` | `Completed` | `2026-03-14T04:40:35Z` | Completed source-backed ingestion and authority-boundary mapping from `ObservationInterface::GetRawObservation()` to `FAgentSpatialChannels` and `FAgentSpatialMetrics`; logged open ambiguity `FL-001` for unused Terran draw-helper ownership. |
| `SC2-ECOSYSTEM-DETERMINISTIC-PLACEMENT-PATTERNS` | `EcosystemPlacement` | `L:\Sc2_Bot\Documentation\Ecosystem\DeterministicPlacementPatterns.md` | `Completed` | `2026-03-14T06:40:54Z` | Completed local-source-first placement seam mapping. Confirmed authored layout registry preference, runtime anchor fallback, production-rail derivation, and ability-routed slot emission in `FTerranBuildPlacementService`; no new ambiguity opened. |
| `SC2-ECOSYSTEM-SOA-STATE-ACCOUNTING` | `EcosystemDataOrientedDesign` | `L:\Sc2_Bot\Documentation\Ecosystem\SoAStateAccountingPatterns.md` | `Completed` | `2026-03-14T07:40:35Z` | Completed local-source SoA accounting map for `FTerranUnitContainer`, `FCommandAuthoritySchedulingState`, and `FBuildPlanningState` rebuild seam; no new ambiguity opened. |
| `SC2-API-AGENT-EVENT-DISPATCH-ACTION-FLUSH` | `ApiEventDispatch` | `L:\Sc2_Bot\Documentation\Sc2Api\AgentEventDispatchAndActionFlushOrder.md` | `Completed` | `2026-03-14T08:40:18Z` | Completed source-backed callback and action ordering map across `CoordinatorImp::CallOnStep`, `ControlImp::IssueEvents`, `ActionImp::SendActions`, and `TerranAgent` callback-to-intent handoff; no new ambiguity opened. |
| `SC2-API-ACTION-DISPATCH-EVENT-ORDERING` | `ApiActionDispatch` | `L:\Sc2_Bot\Documentation\Sc2Api\ActionDispatchAndEventOrdering.md` | `Completed` | `2026-03-14T08:40:58Z` | Added precise source-backed ordering boundaries between `IssueEvents`, `OnStep`, and `SendActions` for the Terran intent execution seam; no new ambiguity opened. |

## Promotion Rule

If the same narrow topic family repeats enough times to justify reusable instructions, the automation may create or deepen one narrow Sc2 skill under `C:\Users\laird\.codex\skills` instead of continuing to accumulate broad narrative notes.
