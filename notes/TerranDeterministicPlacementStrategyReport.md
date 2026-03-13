# Terran Deterministic Placement Strategy Report

## Scope

This report captures the Terran opening placement strategy that produced the first confirmed correct live layout for:
- main-ramp depot
- wall barracks
- second ramp depot
- first factory
- first starport
- second barracks

It also records the next follow-up issue that remained after that placement success:
- the natural expansion command center was never built

## Working Strategy

### Ramp wall

The ramp wall uses an exact discovered descriptor at game start:
- left depot exact slot
- wall barracks exact slot
- right depot exact slot
- inside staging point
- outside staging point

The opening binds directly to those exact slot ids. The scheduler reserves those ids and the wall controller only opens or closes the completed depots.

### Main-base production

The main-base opening production line now uses a reference chain from the resolved wall barracks instead of a loose anchor search:
- the first factory is placed at the wall barracks build point plus a spawn-specific vertical step
- the first starport is placed at the first factory build point plus the same vertical step
- the second barracks is placed at the wall barracks build point plus a spawn-specific horizontal step

This keeps:
- the factory adjacent to the wall barracks
- the factory and barracks addon lanes aligned on the north-south axis
- the starport directly above or below the factory, depending on spawn
- the second barracks on the adjacent side rail closer to the main command center

### Current Bel'Shir spawn-specific steps

For the currently supported starts:
- upper-left start:
  - vertical step = `{0, 4}`
  - horizontal step = `{-6, 0}`
- lower-right start:
  - vertical step = `{0, -4}`
  - horizontal step = `{6, 0}`

### Later main production rail

The later shared `MainProductionWithAddon` rail extends horizontally from the second barracks side pad using the same horizontal step. That preserves addon clearance and keeps later production inside the main instead of drifting toward the natural or behind the mineral line.

## Why This Worked

The successful change was moving away from generic family-wide anchor search for the opening buildings and treating the opening line as a deterministic relationship chain:
- ramp wall first
- use the resolved wall barracks as the reference
- derive factory from barracks
- derive starport from factory
- derive second barracks from barracks

That matched the user's live placement requirements more closely than trying to fit every opening building into one generic rail first.

## Runtime Support

The strategy is supported by:
- exact slot ids for wall structures
- authored family-specific opening pads for the first factory, first starport, and second barracks
- shared production rail pads for later addon-capable production
- scheduler placement-slot reservation
- live debug output for wall state and main layout state

## Remaining Issue After Placement Success

In the latest confirmed live run:
- the building locations were correct
- the natural expansion command center was never built

The likely next fix area is the command-center opening path in `FTerranEconomyProductionOrderExpander`:
- expansion location selection already validates the command-center point globally
- the code then performs a second worker-specific placement query before issuing the SCV order
- the stock Terran example path does not rely on that second worker-specific placement confirmation

That path should be validated with a regression test and likely relaxed so a valid natural expansion point can still produce a build order.

## Next Extension

This same deterministic approach should expand into a curated per-map and per-start-side base layout registry:
- main production rails
- natural depot wall slots
- support depot slots
- turret slots
- sensor tower slots
- tank defensive anchors
- upgrade-building slots

The important architectural rule is unchanged:
- runtime should decide whether a slot is usable now
- authored descriptors should decide where the slot is
