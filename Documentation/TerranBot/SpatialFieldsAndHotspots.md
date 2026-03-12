# Spatial Fields And Hotspots

## Purpose

The bot should eventually drive portions of army movement, squad behavior, and micro from spatial analysis rather than only from unit counts and direct target positions.

The intended model is:

- maintain overlapping fields of threat, safety, influence, and opportunity
- derive hotspots from those fields
- let armies use theater-scale summaries
- let squads use local micro-scale summaries

## Core Spatial Model

The planned authoritative owner is `FSpatialFieldSet`.

The first implementation should use a coarse map-aligned grid so the system stays understandable and cheap to rebuild. Later revisions can add higher-resolution layers from feature layer and render-target information.

Planned field layers:

- `EnemyGroundThreat`
- `EnemyAirThreat`
- `EnemyDetectionCoverage`
- `EnemyStaticDefenseThreat`
- `FriendlyGroundInfluence`
- `FriendlyAirInfluence`
- `FriendlyDetectionCoverage`
- `RetreatSafety`
- `EngageOpportunity`
- `VisionConfidence`
- `PathingCost`
- `SiegeSuitability`

## Field Inputs

Initial input sources:

- unit observations
- structure observations
- current visibility and map exploration state
- pathing and placement queries
- known enemy building positions

Future input sources:

- minimap feature layers
- screen feature layers
- render-target interpretation
- learned threat weighting by matchup or unit composition

## Hotspot Model

Hotspots should be reconstructed descriptors over the field layers.

Planned hotspot descriptors:

- `FThreatHotspotDescriptor`
- `FSafetyHotspotDescriptor`
- `FOpportunityHotspotDescriptor`
- `FStagingRegionDescriptor`

Each hotspot should describe:

- center position
- radius or bounds
- peak strength
- contributing field types
- owning or nearby base region
- confidence score

## Army-Level Usage

Armies should consume larger-scale field summaries to choose:

- where to assemble
- where to push
- which lane to approach from
- when to disengage
- where to stage for a timing attack
- which base to defend first

## Squad-Level Usage

Squads should consume local fields to choose:

- short-term move targets
- local flanks
- retreat vectors
- drop unload zones
- siege setup regions
- safe harass paths

The important distinction is that armies should read aggregated theater intent, while squads should read the local geometry of risk and opportunity.

## Interaction With Map And Placement Services

The spatial field system should inform:

- wall-off evaluation
- build slot safety
- expansion safety scoring
- rally point safety
- home-defense tank positions
- medivac drop staging

## Initial Implementation Scope

The first implementation should stay limited:

- coarse enemy threat fields
- coarse friendly influence fields
- coarse retreat safety fields
- hotspot extraction for bases and attack lanes

That is enough to start informing army goal selection and basic squad posture without overbuilding a micro engine too early.

## Validation Rules

- Field rebuilds must be deterministic for the same observation set.
- Hotspots must be reproducible from the same fields.
- Army planners should consume summarized hotspot descriptors rather than raw pixel-by-pixel fields where possible.
- Squad planners may consume finer local fields, but only through stable service interfaces.
