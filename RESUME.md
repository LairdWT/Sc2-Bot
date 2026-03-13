# Resume Prompt

You are resuming work in `L:\Sc2_Bot` on Friday, March 13, 2026.

Read and follow:
- `L:\Sc2_Bot\AGENTS.md`
- `L:\Sc2_Bot\CodingStandards.md`

Key project constraints:
- Use `apply_patch` for file edits.
- Do not use single-letter variable names.
- Prefer PascalCase and explicit types.
- Prefer `const` and references when reasonable.
- Do not revert unrelated user changes.
- Keep working until the issue is actually verified.

Current user-reported problems:
- The first depot slots at the top of the ramp are now correct in at least one live match.
- The wall barracks then fails to build at all.
- The first refinery can be built at the natural instead of the main, which is incorrect.
- The user asked to create this `RESUME.md`, then continue the previous work.

What was already fixed before this resume:
- Marine production concurrency was improved in `examples/common/planning/FTerranEconomyProductionOrderExpander.cc` so multiple barracks and reactor capacity are not blocked by unrelated scheduler orders.
- Gas harvesting was added in `examples/terran/terran.cc` and `examples/terran/terran.h` so completed refineries get worker harvest intents.
- Ramp-wall discovery in `examples/common/services/FTerranBuildPlacementService.cc` was rewritten from a guessed corridor heuristic to a python-sc2-style ramp-tile model:
  - discover ramp tiles from `pathable && !placeable && mixed terrain height`
  - flood-fill ramp groups
  - choose the main ramp group near the start and natural direction
  - derive wall geometry from upper and lower ramp tiles using circle intersections
  - validate exact left depot, barracks, and right depot slots
- `tests/test_terran_build_placement_service.cc` now includes a synthetic discovered-ramp test in addition to the deterministic fallback checks.

Recent live test result before this resume:
- Command used: `cmd /c LaunchTerranEasyComputerMatch.bat`
- Outcome:
  - the top-of-ramp depot placement was correct
  - the barracks never got built
  - the bot later collapsed into recovery
- The most likely remaining barracks blocker is in economy placement validation, not in ramp discovery.

Strong current hypotheses:
- `examples/common/planning/FTerranEconomyProductionOrderExpander.cc` had an overly broad addon-clearance blocker:
  - `DoesAddonFootprintAvoidObservedStructures(...)` used a simple radius check around the addon center
  - this can falsely reject a valid wall barracks after the first depot exists nearby
- The same file also had overly coarse slot-occupancy detection:
  - `FindObservedStructureOccupyingPlacementSlot(...)` originally used a center-radius test
  - for tight wall layouts, footprint overlap is the correct check
- Refinery selection currently iterates completed town halls in observation order and can therefore choose natural gas first if the order is delayed.
  - It should prefer the town hall nearest the start location, then geysers nearest that town hall.

Edits already in progress when this file was created:
- In `examples/common/planning/FTerranEconomyProductionOrderExpander.cc`:
  - added footprint helpers:
    - `GetStructureFootprintHalfExtentsForUnitType(...)`
    - `GetStructureFootprintHalfExtentsForAbility(...)`
    - `DoAxisAlignedFootprintsOverlap(...)`
  - changed `FindObservedStructureOccupyingPlacementSlot(...)` to use footprint overlap and to take `ABILITY_ID`
  - updated slot-selection call sites to pass the structure ability into occupancy checks
  - changed `DoesAddonFootprintAvoidObservedStructures(...)` to use addon-rectangle overlap instead of the old radius test
  - changed refinery selection to:
    - sort completed town halls by distance to the start location
    - sort nearby geysers by distance to the selected town hall
- In `tests/test_terran_economy_production_order_expander.cc`:
  - `FakeObservation` was updated to initialize simple pathing, placement, and terrain grids
  - a new scenario was added to verify that a wall barracks order still creates a child order after the first wall depot is already present
  - a new scenario was added to verify that the first refinery targets a main-base geyser before natural gas

Important status note:
- After those edits, `cmake --build 'L:\Sc2_Bot\out\build\codex-x64-Debug-20260311' --config Debug --target all_tests` succeeded.
- The new tests were added but had not yet been rerun at the moment this file was written.

Likely next commands:
- `& 'L:\\Sc2_Bot\\RunTests.bat' --filter 'sc2::TestTerranEconomyProductionOrderExpander' --timeout 120`
- `& 'L:\\Sc2_Bot\\RunTests.bat' --filter 'sc2::TestTerranBuildPlacementService' --timeout 120`
- `cmd /c Build.bat --target tutorial`
- `cmd /c LaunchTerranEasyComputerMatch.bat`

Relevant files:
- `L:\Sc2_Bot\examples/common/services/FTerranBuildPlacementService.cc`
- `L:\Sc2_Bot\examples/common/planning/FTerranEconomyProductionOrderExpander.cc`
- `L:\Sc2_Bot\examples/terran/terran.cc`
- `L:\Sc2_Bot\examples/terran/terran.h`
- `L:\Sc2_Bot\tests/test_terran_build_placement_service.cc`
- `L:\Sc2_Bot\tests/test_terran_economy_production_order_expander.cc`

Relevant web research already performed:
- python-sc2 ramp handling:
  - `upper`, `lower`, `upper2_for_ramp_wall`
  - `depot_in_middle`, `corner_depots`, `barracks_correct_placement`
- This informed the current ramp-tile rewrite in the C++ placement service.

Git status note:
- The worktree is dirty with intentional in-progress changes.
- Do not revert user changes.
- Existing modified files before this resume include:
  - `examples/common/planning/FTerranEconomyProductionOrderExpander.cc`
  - `examples/common/services/FTerranBuildPlacementService.cc`
  - `examples/terran/terran.cc`
  - `examples/terran/terran.h`
  - `tests/CMakeLists.txt`
  - `tests/all_tests.cc`
  - `tests/test_terran_build_placement_service.cc`
  - `tests/test_terran_economy_production_order_expander.cc`
  - `tests/test_terran_economy_production_order_expander.h`

Immediate objective:
- Finish validating the barracks occupancy and refinery selection fixes.
- Rebuild, run focused tests, then rerun the live Easy AI match.
- If the barracks still fails live, print or inspect the live wall slot coordinates next and trace the exact deferral reason for the wall barracks economy order.
