# Resume Context

You are resuming work in `L:\Sc2_Bot` on the Terran deterministic building placement effort.

## Immediate User Requests
- Push the current progress to git before continuing.
- Keep `RESUME.md` current.
- Continue the deterministic placement work.
- Examine how other C++ and Python StarCraft II bots handle walling and deterministic placement, then adapt the useful parts to this codebase's standards and architecture.

## Mandatory Local Standards
- Read `L:\Sc2_Bot\CodingStandards.md` before making code changes.
- Use `apply_patch` for file edits.
- Use PascalCase names unless the project has a stricter established convention.
- Do not use single-letter variable names.
- Use explicit types, prefer `const`, references, and `const` member functions when reasonable.
- Prefer interfaces and composition over inheritance.
- Comments must be objective and informative.
- Do not revert unrelated user changes.

## Current Worktree State Before The Next Commit
Modified:
- `examples/common/CMakeLists.txt`
- `examples/common/build_orders/FOpeningPlanStep.cc`
- `examples/common/build_orders/FOpeningPlanStep.h`
- `examples/common/build_orders/FOpeningPlanRegistry.cc`
- `examples/common/planning/FCommandAuthorityProcessor.cc`
- `examples/common/planning/FCommandAuthoritySchedulingState.cc`
- `examples/common/planning/FCommandAuthoritySchedulingState.h`
- `examples/common/planning/FCommandOrderRecord.cc`
- `examples/common/planning/FCommandOrderRecord.h`
- `examples/common/planning/FTerranEconomyProductionOrderExpander.cc`
- `examples/common/services/FBuildPlacementContext.cc`
- `examples/common/services/FBuildPlacementContext.h`
- `examples/common/services/FMainBaseLayoutDescriptor.cc`
- `examples/common/services/FMainBaseLayoutDescriptor.h`
- `examples/common/services/FTerranBuildPlacementService.cc`
- `examples/tutorial.cc`
- `examples/terran/terran.cc`
- `notes/MainBaseLayoutNotes.md`
- `src/sc2utils/sc2_manage_process.cc`
- `tests/test_command_authority_scheduling.cc`
- `tests/test_terran_build_placement_service.cc`
- `tests/test_terran_economy_production_order_expander.cc`
- `tests/test_terran_opening_plan_scheduler.cc`

Untracked:
- `.codex/` (do not commit)
- `examples/common/services/FTerranMainBaseLayoutRegistry.cc`
- `examples/common/services/FTerranMainBaseLayoutRegistry.h`

## What Has Already Been Implemented

### 1. Main-Base Layout Descriptor Plumbing
New type:
- `FMainBaseLayoutDescriptor`

Threaded through:
- `FBuildPlacementContext`
- `FGameStateDescriptor`
- `IBuildPlacementService`
- `FTerranBuildPlacementService`
- `TerranAgent`
- `FTerranEconomyProductionOrderExpander`
- common CMake sources

### 2. New Placement Slot Families
Added in `EBuildPlacementSlotType`:
- `MainSupportDepot`
- `MainBarracksWithAddon`
- `MainFactoryWithAddon`
- `MainStarportWithAddon`

### 3. Placement Service Changes
In `FTerranBuildPlacementService`:
- Main-base layout is now resolved relative to ramp depth and lateral directions instead of pure world-axis guesses.
- The lateral direction uses the main mineral centroid when available and flips away from the mineral line if needed.
- Resource-line clearance checks reject production placements too close to minerals or gas.
- Added exact-slot resolution helpers.
- Added template-slot resolution that validates the exact point first, then optionally does a constrained local search fallback.
- Added production layout anchor discovery.
- Added wider anchor search offsets.
- Added mirrored diagonal fallback if the first diagonal does not resolve.
- If anchor discovery fails, production slots fall back to slot-local search rather than disappearing completely.

Current production template offsets:
- Barracks: `{8,0}`, `{-8,0}`, `{12,8}`, `{-12,8}`
- Factory: `{4,8}`, `{-4,8}`, `{8,16}`, `{-8,16}`
- Starport: `{0,16}`, `{12,16}`, `{4,24}`, `{-4,24}`

These offsets are expressed in main-base layout depth/lateral space, not fixed map axes.

### 3A. Shared Bel'Shir Main Production Rail
`FTerranMainBaseLayoutRegistry` now provides a curated shared production rail for `Bel'Shir Vestige` keyed by start-location bucket:
- rail ordinal `0` is the barracks pad at local offset `{8, 0}`
- rail ordinal `1` is the factory pad at local offset `{12, 8}`
- rail ordinal `2` is the starport pad at local offset `{16, 16}`

Important detail:
- the registry now authors those early pads as `MainProductionWithAddon` slots instead of separate first barracks, factory, and starport families
- `FTerranBuildPlacementService` exposes the shared rail before the later family-specific fallback slots
- later `MainBarracksWithAddon`, `MainFactoryWithAddon`, and `MainStarportWithAddon` slots still exist for capacity after the opening rail

### 4. Opening Plan Bindings
`FOpeningPlanRegistry` currently binds:
- Step 1 depot -> `MainRampDepotLeft`
- Step 2 barracks -> `MainRampBarracksWithAddon`
- Step 8 depot -> `MainRampDepotRight`
- Step 9 factory -> `MainProductionWithAddon` ordinal `1`
- Step 14 starport -> `MainProductionWithAddon` ordinal `2`
- Step 26 second barracks -> `MainProductionWithAddon` ordinal `0`

The order plumbing now carries both:
- `PreferredPlacementSlotType`
- exact `PreferredPlacementSlotId`

Selection precedence in `FTerranEconomyProductionOrderExpander` is now:
- `ReservedPlacementSlotId`
- exact `PreferredPlacementSlotId`
- `PreferredPlacementSlotType`
- first eligible slot from the ordered placement list

Exact preferred slot ids are intentionally non-drifting:
- if the named slot is missing, the order defers as `ReservedSlotInvalidated`
- if the named slot is present but unusable, the order defers as `ReservedSlotOccupied`
- the order does not silently switch to a different rail pad

### 5. Gas Saturation Fixes
In `examples/terran/terran.cc`:
- Added refinery commitment tracking helpers.
- Gas assignment now accounts for already-committed workers before assigning more.
- Gas relief now actively reassigns oversaturated refinery workers back to minerals.
- Relief selection now targets workers assigned to the specific oversaturated refinery.

### 6. Debug Output
`PrintAgentState()` now prints:
- `Main Layout: Valid/Invalid`
- anchor location
- shared production rail ordinals with occupancy labels
- fallback main barracks slots
- fallback factory slots
- fallback starport slots

This is necessary to determine whether the runtime actually resolved deterministic production slots.

## Current Runtime Status

### What Is Working
- The first depot, opening barracks, and second depot on the main ramp were reported by the user as being placed perfectly in a recent live run.
- The second barracks was later reported to be in a good main-base position.
- The scheduler stale wall-child recovery fix is in place and covered by unit tests.
- The shared `MainProductionWithAddon` rail is now threaded through the main-base layout descriptor, opening-plan metadata, scheduler persistence, economy expansion, and live debug output.
- The curated Bel'Shir main production rail now builds from the live ramp wall frame instead of the earlier bad absolute coordinates.
- Focused placement, scheduler, and economy tests pass after the shared-rail change.
- The stock visible launch path is now frozen again to `cmd /c LaunchTerranEasyComputerMatch.bat`, and a normal visible launch succeeded without any detached or bounded-run workaround.

### What Is Still Broken
- Factory and starport still do not build in live play.
- The live main-base descriptor is only resolving a partial shared production rail instead of all three opening ordinals.
- The user previously observed gas oversaturation in the main. Gas assignment logic has been updated, but it still needs live validation once launch is healthy.

### Most Important Current Diagnosis
The live run exposed the exact failure mode:
- the main layout printed `ProductionRail 0` and `2`, but ordinal `1` was missing entirely
- step `9` of the opening is bound to exact slot id `MainProductionWithAddon:1`
- because that exact slot id does not exist at runtime, the scheduler keeps deferring the factory step as `ReservedSlotInvalidated`
- the same partial-rail problem then prevents the starport from ever reaching a valid opening slot

This is the key difference between the successful ramp wall and the failing main production rail:
- the ramp wall is discovered as one coherent exact descriptor at game start
- the wall slots only publish when the depot-barracks-depot pattern is resolved as a complete group
- the current main production rail still resolves authored slots one-by-one, so a partial rail can survive

The most recent code change improved authored-slot snapping and mirrored side selection, but that still is not enough:
- exact authored pads now do a bounded local snap search instead of disappearing on the first invalid query
- the authored main layout now evaluates both lateral rail orientations and keeps the stronger one
- live play still produced only ordinals `0` and `2`, which proves partial per-slot resolution is still the wrong model for opening-critical production

The correct next direction is now clear:
- move main production onto the same exact-group model as the ramp wall
- resolve candidate main-production layouts as complete descriptors instead of independent slots
- only publish the opening production rail when all required ordinals `0`, `1`, and `2` resolve together
- if a candidate group does not fully resolve, reject that whole group and try the alternate candidate instead of leaking a partial rail into runtime

## Verified Builds And Tests
The following succeeded before this resume snapshot:
- `cmd /c BuildAllTests.bat`
- `cmd /c Build.bat --target tutorial`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestTerranBuildPlacementService' --timeout 120`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestTerranOpeningPlanScheduler' --timeout 120`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestTerranEconomyProductionOrderExpander' --timeout 120`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestCommandAuthorityScheduling' --timeout 120`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestTerranDescriptorPipeline' --timeout 120`
- `cmd /c Build.bat --target tutorial`

The following additional focused validations succeeded after the latest mirrored-rail changes:
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestTerranBuildPlacementService' --timeout 120`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestTerranOpeningPlanScheduler' --timeout 120`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestTerranEconomyProductionOrderExpander' --timeout 120`
- `cmd /c Build.bat --target tutorial`

The following live commands were attempted and did not reach gameplay:
- `cmd /c LaunchTerranEasyComputerMatch.bat --skip-build`
- bounded launch with `SC2_TUTORIAL_MAX_GAME_LOOP=4500`
- bounded launch with `SC2_TUTORIAL_MAX_GAME_LOOP=7000`

Observed failure on all recent bounded stock-script attempts:
- repeated `Failed to establish websocket connection: connect(127.0.0.1:16679): timeout`

The following normal visible launches did reach gameplay:
- `cmd /c LaunchTerranEasyComputerMatch.bat`

Live observations from the successful visible launches:
- the ramp depots and opening barracks still placed correctly
- the factory and starport were never built
- the live debug output showed a partial `ProductionRail` instead of the full opening line
- the most telling runtime states were:
  - first successful visible launch: `ProductionRail 0` only
  - later visible launch after mirrored side selection: `ProductionRail 0` and `2`, but still no ordinal `1`
- the scheduler repeatedly logged `PlanStep 9 Ability 328 Reason ReservedSlotInvalidated`, confirming the factory step lost its exact slot id at runtime

## Architectural Direction Agreed With The User
The heuristic approach is still too brittle. The next major step should be a curated per-map, per-start-side, per-base layout registry.

The target model should look like:
- `FMapLayoutKey`: map name plus start-location or spawn bucket
- `EBaseLayoutRole`: `Main`, `Natural`, `Third`, and later additional owned bases if needed
- `FBaseLayoutDescriptor`: typed slot arrays for wall, production, defense, utility, tech, and special-purpose anchors
- `FLayoutSlotDescriptor`: slot id, purpose, exact build point, addon policy, and optional tactical metadata such as rally or siege anchors
- Runtime overlay state layered on top: empty, reserved, occupied, safe, contested, invalid

Desired outcome:
- placement becomes curated-slot lookup first
- scheduler reserves exact slot identifiers
- runtime logic decides whether a slot is currently usable, not where it should be

The production-specific refinement that now looks preferable is:
- keep wall slots as structure-specific exact slots
- replace the current per-slot shared authored production rail with a grouped exact main-production descriptor
- let opening steps bind early factory and starport construction to exact authored rail ordinals only after the full opening line resolves together
- use later authored family-specific or utility slots only where the building truly needs a distinct purpose

## Active Implementation Plan

### Summary
- Replace the current partial per-slot main production rail resolution with a grouped exact main-production descriptor for the current live map and both supported start sides.
- Keep the ramp wall unchanged. The wall remains exact structure-specific slots; only the main-base opening production line moves to the shared rail.
- Lock the physical rail order to `Barracks -> Factory -> Starport` from closest-to-ramp to deepest-in-main, even though the build order constructs factory and starport before the follow-up barracks.
- Keep later production fallback on the existing family-specific main-base slot generation after the shared rail. Do not broaden to natural/defense/utility slots in this slice.
- Use only the stock `LaunchTerranEasyComputerMatch.bat` path for live validation. Do not use direct executable launch paths in this slice.

### Key Changes
- Add `ProductionRailWithAddonSlots` to `FMainBaseLayoutDescriptor` as an ordered vector of exact `FBuildPlacementSlot` entries using `EBuildPlacementSlotType::MainProductionWithAddon`.
- Change `FTerranMainBaseLayoutRegistry` so Bel'Shir main-base authoring produces three exact shared rail pads instead of separate authored first barracks/factory/starport slots.
- Author the shared rail in ramp-local coordinates with exact offsets:
  - rail ordinal `0`: barracks pad at `{8, 0}`
  - rail ordinal `1`: factory pad at `{12, 8}`
  - rail ordinal `2`: starport pad at `{16, 16}`
- Keep the current lower-right mirroring behavior for the opposite spawn. Keep scope limited to the current map and both current start buckets.
- Stop treating the authored early pads as `MainBarracksWithAddon`, `MainFactoryWithAddon`, and `MainStarportWithAddon`. Those family-specific vectors remain for later fallback slots only.
- Extend `FOpeningPlanStep` with an optional exact preferred `FBuildPlacementSlotId`, and thread that through `FCommandOrderRecord` and `FCommandAuthoritySchedulingState` as a separate concept from `ReservedPlacementSlotId`.
- Use slot-selection precedence everywhere in economy expansion as:
  - `ReservedPlacementSlotId`
  - exact preferred `FBuildPlacementSlotId`
  - preferred `EBuildPlacementSlotType`
  - first eligible slot from the ordered placement list
- Make exact preferred slot ids non-drifting: if an order names a preferred slot id and that slot is not present or not usable, do not silently choose a different slot.
- Rebind the opening steps to exact rail ordinals:
  - step `9` factory -> `MainProductionWithAddon` ordinal `1`
  - step `14` starport -> `MainProductionWithAddon` ordinal `2`
  - step `26` follow-up barracks -> `MainProductionWithAddon` ordinal `0`
- Keep wall step bindings unchanged:
  - step `1` left depot
  - step `2` wall barracks
  - step `8` right depot
- Update `FTerranBuildPlacementService::GetStructurePlacementSlots(...)` so:
  - barracks returns wall barracks first, then shared production rail slots, then later barracks fallback slots
  - factory returns shared production rail slots first, then later factory fallback slots
  - starport returns shared production rail slots first, then later starport fallback slots
- Keep the current conflict, addon-clearance, and footprint validation rules for authored exact slots.
- Expand live debug output to print the shared production rail ordinals and occupancy clearly, in addition to the wall state, so the opening line is visible without ambiguity.
- Resolve the opening production rail as a complete candidate group:
  - evaluate each candidate orientation as one exact descriptor
  - reject any candidate that does not resolve all required opening ordinals together
  - never publish a partial opening rail into runtime state

### Test Plan
- Placement tests:
  - verify the three shared production rail pads resolve at the authored offsets for upper-left and mirrored lower-right starts
  - verify barracks, factory, and starport placement queries all expose the shared rail before family-specific fallback slots
  - verify the wall barracks still stays ahead of the shared rail for `BUILD_BARRACKS`
- Scheduler and economy tests:
  - verify `FOpeningPlanStep`, `FCommandOrderRecord`, and scheduling-state persistence preserve exact preferred `FBuildPlacementSlotId`
  - verify step `9`, `14`, and `26` bind to ordinals `1`, `2`, and `0` respectively
  - verify exact preferred slot ids do not drift to another rail pad when the intended pad is unavailable
  - verify reserved-slot claim and release behavior still works for `MainProductionWithAddon`
- Focused validation:
  - `sc2::TestTerranBuildPlacementService`
  - `sc2::TestTerranOpeningPlanScheduler`
  - `sc2::TestTerranEconomyProductionOrderExpander`
  - `sc2::TestCommandAuthorityScheduling`
  - `cmd /c Build.bat --target tutorial`
- Live acceptance:
  - launch only with `LaunchTerranEasyComputerMatch.bat`
  - confirm ramp wall remains correct
  - confirm the live main layout prints all opening ordinals `0`, `1`, and `2`
  - confirm the final landed main production line is `Barracks -> Factory -> Starport` from ramp-back order
  - confirm factory and starport are no longer missing from the opening
  - confirm the follow-up barracks lands on rail ordinal `0` even though it is built after factory and starport

### Assumptions
- This slice is intentionally limited to the current live map and both currently supported start sides. It does not introduce the full multi-base defense/utility registry yet.
- The shared production rail is the authoritative opening-line model; family-specific main barracks/factory/starport slots remain only as later fallback capacity.
- Gas assignment and live-launch reliability are not redesigned in this slice, but the stock launcher script is still required for acceptance testing and regression checks.
- The current authored rail offsets `{8,0}`, `{12,8}`, and `{16,16}` are the fixed v1 pads for this slice unless live validation proves they still need adjustment.

This system should later support:
- turret slots
- sensor tower slots
- tank defensive anchors
- upgrade-building slots
- safe versus contested evaluation per slot

## External References To Review Next
Use browsing because the user explicitly asked for cross-bot comparison.

Primary references already identified:
- Python SC2 ramp and wall placement docs:
  - `https://burnysc2.github.io/python-sc2/text_files/introduction.html?highlight=ramp#ramp-and-wall-placement`
- Python SC2 ramp and wall wiki material:
  - look for `barracks_correct_placement`, `corner_depots`, and `depot_in_middle`
- Ares SC2 building placement documentation:
  - `https://aressc2.github.io/ares-sc2/tutorials/building_placements.html`
  - related placement manager API docs

Goal of that review:
- confirm how curated building placements are keyed and stored
- confirm how ramp wall slots differ from general production slots
- adapt those ideas into this codebase's descriptor and scheduler model rather than copying framework-specific behavior

## Recommended Next Steps
1. Replace the current per-slot authored production rail resolution with an all-or-nothing grouped exact descriptor.
2. Keep the stock `LaunchTerranEasyComputerMatch.bat` path frozen and use it to verify that the live main layout now prints ordinals `0`, `1`, and `2`.
3. If the grouped exact descriptor still fails on this map side, move from projected offsets to fully curated world-space opening pads for that spawn, just as the ramp wall effectively does for its discovered frame.
4. After the rail is correct on Bel'Shir, broaden the same per-map/per-start-side base-layout model to additional bases and defensive slot families.

## Notes
- `notes/MainBaseLayoutNotes.md` exists but needs to be updated once the next architectural step is in place.
- A previous live run emitted civetweb port bind noise on `8080`. That looked environmental rather than placement-specific.
- `examples/tutorial.cc` now supports a bounded live-run override through the `SC2_TUTORIAL_MAX_GAME_LOOP` environment variable.
- Do not forget to commit the restored `RESUME.md` itself.
