# Sc2 Bot API Skill Automation Report

## Run Summary

- RunTimeUtc: 2026-03-14T08:40:58Z
- CompletedTopicId: SC2-API-ACTION-DISPATCH-EVENT-ORDERING
- UpdatedSurface:
  - L:\Sc2_Bot\Documentation\Sc2Api\ActionDispatchAndEventOrdering.md
- ChangeSurface: Sc2ApiDoc

## Current Open Findings

- FL-001: Terran feature-layer draw helper ownership remains ambiguous because `DrawFeatureLayer1BPP`, `DrawFeatureLayerUnits8BPP`, and `DrawFeatureLayerHeightMap8BPP` are declared on `TerranAgent` but have no active call sites in `examples\terran`.

## Notes

- Candidate set had no suitable open ledger topics; this run added one newly discovered coordinator-seam topic per automation rules.
- Low-repetition scoring selected `SC2-API-ACTION-DISPATCH-EVENT-ORDERING` (penalty `0`).
- This pass remained local-source-first and did not require external references.