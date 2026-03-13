# Ramp Wall Placement Notes

## Objective

Make Terran opening wall placement deterministic so the first depot, opening barracks, and second depot build into stable ramp-top slots instead of drifting onto generic production-grid offsets.

## What Changed

### Ramp wall discovery

The old wall placement path used a start-to-natural corridor heuristic. That was replaced with a ramp-tile model in `examples/common/services/FTerranBuildPlacementService.cc`.

The new discovery flow:
- scans map tiles for ramp candidates using `pathable && !placeable && mixed terrain height`
- flood-fills connected ramp groups
- selects the main ramp group by proximity to the start location and alignment toward the natural
- derives wall geometry from upper and lower ramp tiles
- computes exact depot and barracks positions using the same circle-intersection pattern used by python-sc2 ramp wall helpers
- validates the resulting left depot, barracks, and right depot slots before publishing the descriptor

This moved wall placement from an estimated anchor to a topology-driven descriptor.

### Slot-backed opening wall

The opening already had slot metadata, but the wall now benefits from a descriptor that actually represents the ramp:
- `MainRampDepotLeft`
- `MainRampBarracksWithAddon`
- `MainRampDepotRight`

That lets the scheduler reserve exact wall slots instead of competing with later production placement.

### Barracks placement unblock

Once the depot slots were correct, the next blocker was in `examples/common/planning/FTerranEconomyProductionOrderExpander.cc`.

The barracks slot was being rejected by coarse proximity checks:
- structure-slot occupancy used a radius test around the slot center
- addon clearance used a radius test around the addon center

Those were replaced with footprint-overlap checks:
- structure occupancy now compares the planned structure footprint against observed building footprints
- addon clearance now compares the addon rectangle against observed building footprints

This removed false negatives that occurred once the first wall depot existed near the opening barracks slot.

### Refinery selection correction

The first refinery could drift to the natural if town halls were iterated in an unfavorable order.

The refinery builder now:
- sorts completed town halls by distance to the start location
- sorts nearby geysers by distance to that town hall

This keeps the first refinery in the main before moving outward.

## Test Coverage Added

### Placement service

`tests/test_terran_build_placement_service.cc` now covers:
- deterministic fallback descriptor behavior
- synthetic discovered-ramp wall geometry
- ordered wall-slot exposure ahead of production-grid slots

### Economy production expander

`tests/test_terran_economy_production_order_expander.cc` now covers:
- reactor-backed marine concurrency
- wall barracks child-order creation when the first wall depot is already present
- first-refinery selection preferring main-base geysers before natural gas

## Live Validation Outcome

The top-of-ramp wall placement was visually confirmed in live Easy AI matches:
- first depot correct
- opening barracks correct
- second depot correct

## Follow-On Work

The next placement problem is broader main-base structure layout:
- factory and starport slots should stay inside the main
- production structures need addon-safe deterministic slots
- the layout should preserve movement lanes and avoid trapping units
- the scheduler should reserve and monitor these slots the same way it now does for wall slots
