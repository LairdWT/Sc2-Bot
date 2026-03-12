# Telemetry And Game Record Store

## Purpose

The bot should accumulate a versioned record of what happened in each game so later analysis does not depend on scraping console logs or replay metadata manually.

The primary goals are:

- record what plan and decisions were active
- record what the game state looked like when those decisions were made
- record the outcome and opponent context
- export compact spatial summaries such as heatmaps

The storage direction should follow the installed `structure-of-arrays` and `data-oriented-models` skills.

## Record Layers

Planned record layers:

- `FMatchRecord`
  one manifest per game
- `FDecisionEventStore`
  append-only decision and state transition events
- `FSpatialSnapshotStore`
  coarse field and heatmap exports
- `FOutcomeSummary`
  final result, matchup, and timing summary

## Versioning

Every recorded match should carry explicit version data.

Required version fields:

- telemetry schema version
- bot architecture version
- git commit or build identifier
- StarCraft II version

The telemetry reader should reject or migrate unknown schema versions instead of silently guessing.

## Opponent Classification

Opponent type should be stored with an enum, not free-form strings.

Planned enum:

- `EOpponentCategory::BlizzardComputer`
- `EOpponentCategory::Human`
- `EOpponentCategory::ThirdPartyBot`
- `EOpponentCategory::Unknown`

Optional future labels:

- ladder MMR band
- matchup
- known bot name
- map name and spawn location pairing

## Authoritative Event Shape

Decision and state history should be recorded as a structure-of-arrays event store.

Planned authoritative columns:

- `MatchIds`
- `EventSteps`
- `EventTypes`
- `GamePlanValues`
- `MacroPhaseValues`
- `ArmyIndices`
- `SquadIndices`
- `SourceLayers`
- `DecisionReasonCodes`
- `MineralValues`
- `VespeneValues`
- `SupplyValues`
- `ThreatScores`
- `OutcomeFlags`

Row-style views should be reconstructed only for export, debugging, or analysis queries.

## Spatial Export Direction

Spatial exports should have two forms:

- structured field metadata
- compact image payloads

For aggregate per-race density or hotspot accumulation, one image can encode:

- `R` channel for Zerg-related values
- `G` channel for Protoss-related values
- `B` channel for Terran-related values

That is a good compact export format for aggregated race-conditioned heatmaps. It should not replace the authoritative structured metadata needed to interpret:

- map identifier
- world-to-pixel transform
- normalization rules
- field meaning
- schema version

If one export must carry both race-conditioned data and confidence or recency, use either:

- a second image
- a separate metadata blob
- an additional scalar grid in the structured snapshot store

## Match Output Bundle

The preferred output shape is one folder per match containing:

- match manifest
- decision event file
- outcome summary
- spatial image exports
- spatial metadata manifest
- optional replay correlation data

The storage location should be outside the live gameplay state and treated as analytics output.

## Immediate Implementation Direction

The first implementation should stay compact:

- one match manifest
- one append-only decision event store
- one outcome summary
- one coarse spatial snapshot export

That is enough to start building a reusable database of bot behavior without overbuilding the learning pipeline before the architecture is stable.
