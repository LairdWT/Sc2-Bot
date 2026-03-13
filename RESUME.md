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
- `examples/common/descriptors/FGameStateDescriptor.cc`
- `examples/common/descriptors/FGameStateDescriptor.h`
- `examples/common/planning/FTerranEconomyProductionOrderExpander.cc`
- `examples/common/services/EBuildPlacementSlotType.cc`
- `examples/common/services/EBuildPlacementSlotType.h`
- `examples/common/services/FBuildPlacementContext.cc`
- `examples/common/services/FBuildPlacementContext.h`
- `examples/common/services/FTerranBuildPlacementService.cc`
- `examples/common/services/FTerranBuildPlacementService.h`
- `examples/common/services/IBuildPlacementService.h`
- `examples/terran/terran.cc`
- `examples/terran/terran.h`
- `tests/test_terran_build_placement_service.cc`
- `tests/test_terran_economy_production_order_expander.cc`
- `tests/test_terran_opening_plan_scheduler.cc`

Untracked:
- `examples/common/services/FMainBaseLayoutDescriptor.cc`
- `examples/common/services/FMainBaseLayoutDescriptor.h`
- `notes/MainBaseLayoutNotes.md`
- `.codex/` (do not commit)

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

### 4. Opening Plan Bindings
`FOpeningPlanRegistry` currently binds:
- Step 1 depot -> `MainRampDepotLeft`
- Step 2 barracks -> `MainRampBarracksWithAddon`
- Step 8 depot -> `MainRampDepotRight`
- Step 9 factory -> `MainFactoryWithAddon`
- Step 14 starport -> `MainStarportWithAddon`
- Step 26 second barracks -> `MainBarracksWithAddon`

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
- first main barracks slot
- first factory slot
- first starport slot

This is necessary to determine whether the runtime actually resolved deterministic production slots.

## Current Runtime Status

### What Is Working
- The first depot, opening barracks, and second depot on the main ramp were reported by the user as being placed perfectly in a recent live run.
- The main layout descriptor can now resolve production anchor points and at least some production slots on the live map.

### What Is Still Broken
- Factory, starport, and second barracks placement remains unreliable or stalls entirely.
- Earlier live logs repeatedly showed `NoValidPlacement` on:
  - opening factory step
  - second barracks step
- Some runs eventually resolved main production slots, but the scheduler still did not consistently build those structures.
- The user previously observed gas oversaturation in the main. Gas assignment logic has been updated, but it still needs live validation.

### Most Important Current Diagnosis
The project appears to have moved past pure slot-discovery failure. The next likely failure surface is one or more of:
- exact-slot runtime validation still rejecting otherwise good slots
- scheduler claim or occupancy state not matching the new layout slots correctly
- fallback search or occupancy checks interfering with reserved deterministic slots
- anchor-derived production formation still not being curated enough for the live map

## Verified Builds And Tests
The following succeeded before this resume snapshot:
- `cmake --build 'L:\Sc2_Bot\out\build\codex-x64-Debug-20260311' --config Debug --target all_tests`
- `cmd /c Build.bat --target tutorial`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestTerranBuildPlacementService' --timeout 120`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestTerranOpeningPlanScheduler' --timeout 120`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestTerranEconomyProductionOrderExpander' --timeout 120`
- Earlier in the same effort: `sc2::TestTerranDescriptorPipeline`

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
1. Commit and push the current worktree, excluding `.codex/`.
2. Review the external references above and summarize the transferable model.
3. Start the curated map-layout registry with the current live map and both spawn positions if data is available.
4. Move main-base production slots for the opening barracks, first factory, first starport, and second barracks onto curated exact-slot data.
5. Re-run focused placement tests, then a live Terran versus Easy AI match.

## Notes
- `notes/MainBaseLayoutNotes.md` exists but needs to be updated once the next architectural step is in place.
- A previous live run emitted civetweb port bind noise on `8080`. That looked environmental rather than placement-specific.
- Do not forget to commit the restored `RESUME.md` itself.
