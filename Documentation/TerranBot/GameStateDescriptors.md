# Game State Descriptors

## Purpose

The bot needs a clear hierarchy of descriptors that describe the current game state in a form that planners can consume without directly depending on raw SC2 observations.

These descriptors are derived views. They must be rebuilt from authoritative state each frame and must not become their own source of truth.

## Descriptor Hierarchy

### Root Descriptor

`FGameStateDescriptor` should be the single top-level snapshot exposed to higher-level planning systems.

Planned child domains:

- `FMacroStateDescriptor`
- `FEconomyStateDescriptor`
- `FTechStateDescriptor`
- `FThreatStateDescriptor`
- `FMapStateDescriptor`
- `FArmyStateDescriptor`
- `FBuildPlanningDescriptor`
- `FSpatialSummaryDescriptor`

### Macro State

`FMacroStateDescriptor` should summarize the bot's current strategic posture.

Planned fields:

- `EGamePlan`
- `EMacroPhase`
- `EThreatResponse`
- `EExpansionReadiness`
- `EProductionFocus`
- `EHarassPlan`

### Economy State

`FEconomyStateDescriptor` should summarize present and projected economy.

Planned fields:

- current minerals
- current vespene
- current supply used, cap, and available
- worker count
- worker saturation per base
- refinery saturation
- reserved budget totals
- predicted income at one or more horizons

### Tech State

`FTechStateDescriptor` should summarize available and planned tech.

Planned fields:

- completed production structures
- production add-on coverage
- upgrade status
- key unlock availability
- blocked prerequisites

### Threat State

`FThreatStateDescriptor` should summarize enemy pressure and local danger.

Planned fields:

- enemy air threat level
- enemy ground threat level
- harassment threat
- base-specific defense risk
- attack window confidence
- retreat urgency

### Map State

`FMapStateDescriptor` should summarize positional and terrain-relevant information.

Planned fields:

- start location
- natural expansion choice
- third and fourth expansion candidates
- owned expansion set
- threatened expansion set
- buildable wall-off slots
- staging zones
- attack lanes
- retreat lanes

### Army State

`FArmyStateDescriptor` should summarize all armies and squads.

Planned fields:

- army descriptors
- reserve unit pool summary
- active squad counts by role
- army supply totals
- detached harassment packages

### Build Planning State

`FBuildPlanningDescriptor` should summarize budgeted needs and committed work.

Planned fields:

- active build packages
- blocked needs
- reserved minerals
- reserved vespene
- reserved supply
- soonest timing window
- unmet squad composition requirements

### Spatial Summary

`FSpatialSummaryDescriptor` should present a planner-friendly summary of the more detailed spatial fields.

Planned fields:

- major enemy hotspots
- safest retreat region
- preferred assembly region
- current front line estimate
- base defense hot regions

## Authoritative State Versus Descriptors

Authoritative state should live in domain stores such as `FArmyDomainState`, `FBuildPlanningState`, and `FSpatialFieldSet`.

Descriptors should:

- rebuild from authoritative arrays and index lists
- hold lightweight aggregated values
- be safe to discard and rebuild each frame
- remain deterministic for the same input state

Descriptors should not:

- own long-lived mutable state
- issue commands
- hide resource reservations that only exist in one planner

## Rebuild Order

The descriptor pipeline should follow a stable order each frame:

1. Rebuild authoritative state from observation and retained bot memory.
2. Rebuild spatial fields and map analysis outputs.
3. Rebuild descriptors from the authoritative stores.
4. Run the strategic director against the descriptors.
5. Run domain planners using the strategic result and descriptors.

## Validation Rules

The descriptor system should maintain these invariants:

- every descriptor rebuild is deterministic for the same frame input
- no descriptor duplicates ownership of mutable runtime state
- invalid unit or building type inputs do not index past array bounds
- every army and squad descriptor resolves back to a valid authoritative owner
- the root `FGameStateDescriptor` can be logged or tested without direct SC2 API access
