# Main Base Layout Notes

## Objective

Keep Terran barracks, factories, and starports inside the main base in deterministic addon-safe slots instead of letting later production drift toward the natural or compete for one generic production bucket.

## What Changed

### Added a main-base layout descriptor

The placement system now has `FMainBaseLayoutDescriptor` in `examples/common/services/FMainBaseLayoutDescriptor.h`.

It stores explicit slot lists for:
- natural-approach depots
- support depots
- barracks with addon clearance
- factory with addon clearance
- starport with addon clearance

The descriptor is computed once at game start and cached in:
- `FGameStateDescriptor::MainBaseLayoutDescriptor`
- `FBuildPlacementContext::MainBaseLayoutDescriptor`

That means scheduler expansion and later placement queries all see the same ordered slot layout.

### Split the generic production bucket

`EBuildPlacementSlotType` now distinguishes:
- `MainBarracksWithAddon`
- `MainFactoryWithAddon`
- `MainStarportWithAddon`
- `MainSupportDepot`

This matters because the scheduler reserves exact slot ids. Factories and starports no longer contend with barracks for a single `MainProductionWithAddon` pool.

### Built the layout around a deterministic anchor

`FTerranBuildPlacementService` now derives a layout anchor from the ramp wall:
- use the ramp wall center when the wall is valid
- shift horizontally onto the main-base side of the wall
- keep the layout centered around the main command center y-position

The slot templates are then resolved from that anchor into explicit rows and columns:
- barracks closest to the wall inside the main
- factories deeper in the main
- starports deepest in the main
- depot rows outside the addon lanes

### Added self-conflict checks while resolving slots

The descriptor builder validates more than simple terrain placement:
- structure footprints cannot overlap earlier resolved slots
- addon footprints cannot overlap earlier structures or earlier addon footprints
- slots must remain on the main-base side of the ramp wall when the wall is valid

This is why the first natural-approach depot row had to move farther out. The original template intersected the future addon footprint for the first main-base barracks slot.

### Scheduler integration

The scheduler did not need a new reservation system. It already reserves exact `FBuildPlacementSlotId` claims.

The important change was making the placement service return explicit ordered slot families:
- barracks requests get wall barracks first, then `MainBarracksWithAddon`
- factory requests get `MainFactoryWithAddon`
- starport requests get `MainStarportWithAddon`
- supply depots get wall depots, then natural-approach depots, then support depots

This keeps production deterministic and prevents one structure type from consuming another type's intended space.

## Latest Finding

The ramp wall works because it is modeled as exact named slots. The main-base production layout is still weaker because only a small authored subset exists and the rest of the family still falls back to inferred template placement.

Recent live feedback narrowed the remaining issue:
- the opening ramp wall is correct
- the second barracks position is acceptable
- the first factory and starport are still too far back

That means the next improvement should mimic the ramp model more closely for main-base production:
- use authored ordered slots, not “find something near this anchor”
- keep the early production line on one authored rail behind the ramp
- expose that rail clearly in live debug output

The latest direct fix moved the authored Bel'Shir main production line from:
- barracks `{8, 0}`
- factory `{4, 8}`
- starport `{0, 16}`

to:
- barracks `{8, 0}`
- factory `{12, 8}`
- starport `{16, 16}`

These are local offsets in the ramp-derived production frame. The purpose of this change is to keep barracks, factory, and starport on the same authored ramp-back rail instead of sending factory and starport onto the previous opposite-side diagonal.

## Verification

Focused tests updated and passing:
- `sc2::TestTerranBuildPlacementService`
- `sc2::TestTerranEconomyProductionOrderExpander`
- `sc2::TestTerranDescriptorPipeline`
- `sc2::TestTerranOpeningPlanScheduler`

Recent live behavior confirmed:
- the first depot, opening barracks, and second depot can now resolve correctly on the main ramp
- the main-base layout descriptor can resolve at least one barracks, factory, and starport slot on the live map

Live behavior still failing:
- factory, starport, and second barracks construction is still unreliable or stalls entirely
- some runs still defer those steps with `NoValidPlacement`
- gas saturation changes still need live validation in the same match
- some bounded launcher-script reruns still fail the initial websocket connection before gameplay, which blocks live verification of the latest authored-slot changes

## Next Direction

The current anchor-and-search layout is better than the generic production bucket, but it is still heuristic. The likely next step is a curated per-map and per-start-position slot registry for each base, so the scheduler consumes exact authored slots instead of relying on local placement search during a live game.

The cleaner version of that model is a shared authored production rail:
- exact ordered addon-capable pads for the main base
- opening steps bind factory, starport, and follow-up barracks to specific rail ordinals
- scheduler reservations and occupancy tracking work on those exact pad ids

That matches the way the ramp wall already behaves and is closer to how Ares-style placement dictionaries are structured.
