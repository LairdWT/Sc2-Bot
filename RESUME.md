# Resume Context

You are resuming work in `L:\Sc2_Bot` on the Terran deterministic building placement effort.

## Immediate User Requests
- Commit the current goal-driven scheduler and army-control refactor work to git before continuing.
- Keep `RESUME.md` current with the new goal, priority-tier, and army-pipeline state.
- Preserve the current placement and natural-expansion success in git before further changes.
- Keep `RESUME.md` current.
- Preserve the user-approved live launch path: only `cmd /c LaunchTerranEasyComputerMatch.bat`.
- The Terran opening plan must not remain a time-gated special case; it should seed the command scheduler and then hand off to normal scheduler-driven follow-up packages and triggers.
- Use a Medium opponent for live validation so stalled combat logic does not leave near-endless visible test runs.
- Create a standardized scheduler task descriptor so the opener and future runtime-triggered packages share one authored task shape.

## Mandatory Local Standards
- Read `L:\Sc2_Bot\CodingStandards.md` before making code changes.
- Use `apply_patch` for file edits.
- Use PascalCase names unless the project has a stricter established convention.
- Do not use single-letter variable names.
- Use explicit types, prefer `const`, references, and `const` member functions when reasonable.
- Prefer interfaces and composition over inheritance.
- Comments must be objective and informative.
- Do not revert unrelated user changes.

## Latest Scheduler Refactor Checkpoint

### Current Slice
- The active slice is the user-approved "Goal-Driven Scheduler, Bucketed Queues, And Scheduler-Owned Army Control" refactor.
- The current work is focused on making goals authoritative, priority tiers explicit, and army control flow through scheduler layers instead of direct `TerranAgent` combat helpers.

### What Was Added In This Slice
- `FDefaultStrategicDirector` now rebuilds `GoalSet` first and derives macro intent from those goals:
  - immediate goals for defense, workers, and supply
  - near-term goals for expansion, refinery count, production capacity, tech unlocks, and upgrades
  - strategic goals for army production, pressure, cleanup, and scouting
- `FTerranTimingAttackBuildPlanner` no longer drives live behavior from frame thresholds. It now projects desired counts from the active goal set.
- `FCommandAuthorityProcessor` now seeds strategic orders from active goals instead of relying only on opener seeding:
  - worker production
  - supply
  - expansions
  - production structures
  - tech unlocks
  - upgrades
  - army-production goals
- Strategic `ArmyMission` tasks now produce `Army`-layer child orders instead of going through the economy child path.
- The scheduler interfaces were widened so army, squad, and unit-execution expansion stages can consume frame state, agent state, rally data, and the authoritative scheduling store:
  - `IArmyOrderExpander`
  - `ISquadOrderExpander`
  - `IUnitExecutionPlanner`
- The concrete Terran army pipeline now exists as separate scheduler-owned components:
  - `examples/common/planning/FTerranArmyOrderExpander.h`
  - `examples/common/planning/FTerranArmyOrderExpander.cc`
  - `examples/common/planning/FTerranSquadOrderExpander.h`
  - `examples/common/planning/FTerranSquadOrderExpander.cc`
  - `examples/common/planning/FTerranArmyUnitExecutionPlanner.h`
  - `examples/common/planning/FTerranArmyUnitExecutionPlanner.cc`
- `examples/common/CMakeLists.txt` now includes the new goal, priority-tier, army-mission, and tactical-behavior translation units that had previously been present but not compiled.
- `FTacticalBehaviorScore.h` now includes the missing SC2 type definitions required for `Tag` support.
- `TerranAgent` now routes the live scheduler path through:
  - command-authority processing
  - task-priority refresh
  - economy expansion
  - army mission expansion
  - squad expansion
  - unit-execution planning
  - ready-intent draining
- `FTerranArmyPlanner` now derives primary army posture from the current army mission descriptor instead of only the legacy game-plan switch.
- Live debug output now prints:
  - production focus
  - active goals by horizon
  - primary army mission descriptor
  - derived queue occupancy by priority tier
  - current army execution order count per step

### Current In-Progress Implementation State
- `cmd /c Build.bat --target tutorial` passes with the concrete army pipeline wired in.
- `cmd /c BuildAllTests.bat` passes after the goal-driven test updates and the new army-pipeline coverage.
- The legacy direct `ProduceArmyIntents(...)` path is no longer called from `TerranAgent::OnStep()`. The live army-control path is now scheduler-owned.
- Focused validation that currently passes for this slice:
  - `& 'L:\\Sc2_Bot\\RunTests.bat' --filter 'sc2::TestTerranArmyOrderPipeline' --timeout 180`
  - `& 'L:\\Sc2_Bot\\RunTests.bat' --filter 'sc2::TestTerranPlanners' --timeout 180`
  - `& 'L:\\Sc2_Bot\\RunTests.bat' --filter 'sc2::TestCommandAuthorityScheduling' --timeout 180`
  - `& 'L:\\Sc2_Bot\\RunTests.bat' --filter 'sc2::TestTerranDescriptorPipeline' --timeout 180`
  - `& 'L:\\Sc2_Bot\\RunTests.bat' --filter 'sc2::TestTerranEconomyProductionOrderExpander' --timeout 180`
  - `& 'L:\\Sc2_Bot\\RunTests.bat' --filter 'sc2::TestTerranOpeningPlanScheduler' --timeout 180`
- Remaining work in this slice:
  - remove or quarantine the now-dormant direct Terran combat helpers so the old marine-only path cannot accidentally re-enter live control
  - extend goal-driven runtime package seeding so the opener is only initial scheduler input and the follow-up macro packages remain on the same descriptor surface
  - live-validate that the new sweep and cleanup logic actually finishes games against hidden enemy expansions

### Commit Scope For This Checkpoint
- Commit the current scheduler-goal refactor files plus `RESUME.md`.
- Do not include the unrelated `Documentation/*` modifications or untracked documentation artifacts in this checkpoint commit.

### Immediate Next Steps After This Commit
1. Remove or quarantine the dormant direct Terran combat helpers and any remaining legacy combat-only target selection that is no longer authoritative.
2. Continue the goal-driven scheduler work by expanding runtime-triggered strategic task packages on top of the same descriptor surface.
3. Live-validate the scheduler-owned army path through the stock visible launch script once the legacy combat helpers are fully isolated.
4. Keep unrelated documentation artifacts out of the next gameplay commit unless the user explicitly asks for them.

## Current Worktree State Before The Next Commit
Modified:
- `tests/CMakeLists.txt`
- `tests/all_tests.cc`
- `tests/test_terran_descriptor_pipeline.cc`
- `tests/test_terran_opening_plan_scheduler.cc`
- `tests/test_terran_planners.cc`
- `examples/common/CMakeLists.txt`
- `examples/common/planning/FTerranArmyPlanner.cc`
- `examples/terran/terran.cc`
- `examples/terran/terran.h`
- `RESUME.md`

Untracked:
- `.codex/` (do not commit)
- `Documentation/Automation/`
- `Documentation/Ecosystem/`
- `Documentation/Sc2Api/`
- `Documentation/TerranBot/TerranAgentCoordinatorPath.md`
- `tests/test_terran_army_order_pipeline.cc`
- `tests/test_terran_army_order_pipeline.h`
- `examples/common/planning/FTerranArmyOrderExpander.cc`
- `examples/common/planning/FTerranArmyUnitExecutionPlanner.cc`
- `examples/common/planning/FTerranArmyUnitExecutionPlanner.h`
- `examples/common/planning/FTerranSquadOrderExpander.cc`
- `examples/common/planning/FTerranSquadOrderExpander.h`

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
- rail ordinal `0` is authored from the wall barracks with offset `{8, 10}`
- rail ordinal `1` is authored from that barracks rail pad with step offset `{4, 8}`
- rail ordinal `2` is authored from the resolved factory rail pad with the same `{4, 8}` step

Important detail:
- the registry now authors those early pads as `MainProductionWithAddon` slots instead of separate first barracks, factory, and starport families
- `FTerranBuildPlacementService` exposes the shared rail before the later family-specific fallback slots
- later `MainBarracksWithAddon`, `MainFactoryWithAddon`, and `MainStarportWithAddon` slots still exist for capacity after the opening rail
- the shared rail resolver is now chained instead of rigid:
  - resolve the factory pivot first
  - resolve the rail barracks backward from the resolved factory
  - resolve the rail starport forward from the resolved factory
  - reject the authored rail entirely if all opening ordinals do not resolve together
- there is now a second fallback rail path for non-authored cases:
  - use the first valid main barracks slot as the seed
  - derive the factory from that barracks using the wall-barracks-to-main-barracks delta
  - derive the starport from the resolved factory using that same step
  - remove the source barracks fallback slot once it becomes shared rail ordinal `0`

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

### 4A. Opening Plan Queue Depth
The opening-plan and scheduler pipeline now carries authored queue depth through:
- `FOpeningPlanStep::RequestedQueueCount`
- `FCommandOrderRecord::RequestedQueueCount`
- `FCommandAuthoritySchedulingState::RequestedQueueCounts`

Current authored queue-depth behavior:
- default requested queue count is `1`
- reactor marine steps now preserve the authored `2`-deep queue intent from the JSON asset
- `FTerranEconomyProductionOrderExpander` now allows a unit-production economy order to keep spawning unit-execution children until the active child count for that same parent reaches the authored requested queue count
- this is limited to unit-production orders and does not change the single-child structure-build behavior

### 4B. Standardized Scheduler Task Descriptor
New type:
- `FCommandTaskDescriptor`

Current descriptor coverage:
- package kind
- need kind
- action kind
- completion kind
- trigger metadata
  - minimum game loop
  - required completed task ids
  - reference clock time
- action metadata
  - ability id
  - producer type
  - result type
  - upgrade id
  - target count
  - requested queue count
  - parallel group id
  - preferred placement slot type and exact slot id
- completion metadata
  - observed count at least
  - expected completion by game loop

Current integration:
- `FOpeningPlanStep` now stores one `FCommandTaskDescriptor` instead of a bespoke field set
- `FOpeningPlanRegistry` now populates standardized task metadata for every compiled opening step
- `FCommandAuthorityProcessor` now seeds and completes opening work from `FCommandTaskDescriptor`
- current compiled opener assigns:
  - `PackageKind = Opening`
  - `NeedKind` and `ActionKind` derived from ability
  - `CompletionKind = CountAtLeast`
  - `TriggerReferenceClockTime` derived from the authored minimum game loop

Intent of this slice:
- opening steps are now the first concrete producer of the shared task shape
- future runtime triggers can emit the same descriptor without going through planner-specific desired-count fields

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
- The latest confirmed live run also placed the first factory, first starport, and second barracks in the correct main-base locations.
- The natural expansion command center now builds again after removing the extra worker-specific `BUILD_COMMANDCENTER` placement gate.
- The latest live adjustment moved the opening factory and starport one cell closer to the starting command center, and the user confirmed that looked good.
- The compiled `TerranTwoBaseMMMFrameOpening` registry now runs the converted `Terran.TwoBaseMMM.EightMinutePressure` package through `08:17` with 102 authored steps.
- The opening-plan scheduler test now validates the current 102-step registry surface through the standardized task descriptor instead of the old bespoke step fields.
- The scheduler stale wall-child recovery fix is in place and covered by unit tests.
- The shared `MainProductionWithAddon` rail is now threaded through the main-base layout descriptor, opening-plan metadata, scheduler persistence, economy expansion, and live debug output.
- The curated Bel'Shir main production rail now builds from the live ramp wall frame instead of the earlier bad absolute coordinates.
- The production rail now resolves as a chained exact descriptor with the factory as the pivot:
  - the factory slot resolves first from the authored ramp-back candidate
  - the barracks rail pad resolves backward from that factory point
  - the starport rail pad resolves forward from that factory point
  - the whole rail still publishes only when all required ordinals resolve together
- There is now a synthesized fallback shared rail for cases where the authored rail still does not resolve:
  - choose a resolved main barracks fallback slot as the barracks seed
  - derive factory ordinal `1` from that barracks seed
  - derive starport ordinal `2` from the resolved factory
  - keep scheduler behavior exact-slot and non-drifting
- Focused placement, scheduler, and economy tests pass after the factory-pivot chain change.
- The stock visible launch path is now frozen again to `cmd /c LaunchTerranEasyComputerMatch.bat`, and a normal visible launch succeeded without any detached or bounded-run workaround.
- `LaunchTerranEasyComputerMatch.bat` now cleans stale `SC2_x64.exe` before launch and again after exit to avoid leftover websocket sessions after interrupted runs.
- The converted `Terran.TwoBaseMMM.EightMinutePressure` build order is now wired into the compiled opening registry through `08:17`.
- Scheduler execution now supports the additional opener abilities needed by that deeper package:
  - marauders
  - widow mines
  - vikings
  - barracks tech labs
  - factory reactors
  - starport reactors
  - stimpack
  - combat shield
  - concussive shells
  - infantry weapons level 1
- Completed Terran upgrades are now observed in `FAgentState`, threaded into `FBuildPlanningState`, and used by opening-plan completion checks so research steps do not remain permanently active.
- The live test opponent in `examples/tutorial.cc` is now `sc2::Difficulty::Medium`, while the canonical launch path remains `cmd /c LaunchTerranEasyComputerMatch.bat`.
- The opener now seeds through `FCommandTaskDescriptor`, which is the first shared authored task surface for future runtime package triggers.

### What Is Still Broken
- The opening is still duplicated as a time-gated special case in `FTerranTimingAttackBuildPlanner`; live debug output is still showing that planner’s `Desired*Count` progression instead of treating the authored opener purely as scheduler seeding.
- Army behavior still does not close games reliably, so even with Medium difficulty the bot can stall into long cleanup matches if the opponent cannot break the bot economically.
- The next requested architecture slice is package seeding into the command scheduler:
  - expansion packages
  - upgrade-building packages
  - mineral-line safety turret packages
  - supply-cap response packages
  - high-income production scaling packages
- The user previously observed gas oversaturation in the main. Gas assignment logic has been updated, but it still needs live validation in a longer run after the build-order extension work.

### Most Important Current Diagnosis
The natural-expansion regression was caused by an unnecessary second placement gate:
- the command-center path already selected the natural expansion location with a global `BUILD_COMMANDCENTER` placement query
- it then performed a second worker-specific `BUILD_COMMANDCENTER` placement query before creating the SCV child order
- that extra gate was stricter than the stock Terran example path and could reject an otherwise valid expansion point

The current fix is:
- keep the global expansion-location validation in `GetNextExpansionLocation(...)`
- remove the extra worker-specific `BUILD_COMMANDCENTER` placement confirmation in `FTerranEconomyProductionOrderExpander`
- cover that behavior with a unit test that fails if a worker-specific rejection blocks a globally valid natural expansion

The current placement tuning is:
- opening factory and starport are both pulled one cell toward the start location while preserving their shared addon lane
- this is implemented in the authored Bel'Shir main-base layout descriptor by applying the same one-cell start-location pull step to the opening factory and starport chain

The current build-order diagnosis is:
- `Terran.TwoBaseMMM.FrameOpening.json` was only the earlier 32-step source package
- the compiled registry now uses the converted `Terran.TwoBaseMMM.EightMinutePressure` package with 102 steps through `08:17`
- the latest code change for build-order work was therefore a parity safeguard, not a behavioral extension:
  - explicit zero-valued goal fields now mirror the JSON production goals
  - the scheduler test now checks all 32 steps, their frame triggers, priorities, abilities, targets, parallel groups, and prerequisites
- the current follow-up change now carries the JSON `QueueCount` semantics into the scheduler model:
  - the compiled registry summary string now exactly matches the JSON asset summary
  - steps `11`, `12`, and `24` now preserve their authored requested queue count of `2`
  - scheduler persistence now preserves requested queue count on strategic, economy, and unit-execution orders
  - the economy expander can keep filling a reactor-backed queue for the same parent order instead of stopping at one active child

## Latest Confirmed Live Behavior
- Live launch used only `cmd /c LaunchTerranEasyComputerMatch.bat`.
- The user confirmed:
  - the ramp wall placement was correct
  - the factory position was correct
  - the starport position was correct
  - the second barracks position was correct
- A later live run after the command-center fix confirmed the natural expansion built again.
- The latest live run after the one-cell pull adjustment was accepted by the user as looking good.
- A later visible run with the deeper `08:17` opening wired in completed successfully through the stock script.
- That run confirmed the next architectural problem:
  - the bot can survive without actually finishing the opponent
  - the live test should therefore use Medium instead of Easy
  - the opening still needs to stop being mirrored by the special-case timing-attack build planner
- A later visible run after the Medium difficulty bump also completed successfully through the same stock script.
- That Medium run confirmed:
  - the launcher path is still stable when invoked only as `cmd /c LaunchTerranEasyComputerMatch.bat`
  - the difficulty bump itself is working
  - the bot is still driven mostly by the seeded opener plus planner desired-counts rather than standardized runtime task packages

## Working Main-Base Placement Strategy
- The ramp wall remains a discovered exact descriptor:
  - left depot exact slot
  - wall barracks exact slot
  - right depot exact slot
- The main-base opening production line now uses a reference chain anchored from the resolved wall barracks:
  - first factory = wall barracks build point plus the spawn-specific vertical step
  - first starport = first factory build point plus the same spawn-specific vertical step
  - second barracks = wall barracks build point plus the spawn-specific horizontal step
- Later shared rail pads extend from the second barracks side pad along the same horizontal step.
- For the currently supported Bel'Shir upper-left and lower-right starts:
  - vertical step is `{0, 4}` for upper-left and `{0, -4}` for lower-right
  - horizontal step is `{-6, 0}` for upper-left and `{6, 0}` for lower-right
- This layout keeps the opening factory and starport on the same north-south addon lane and the second barracks on the adjacent horizontal rail closer to the command center.

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

The following focused validations succeeded after the factory-pivot chain change:
- `cmd /c BuildAllTests.bat`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestTerranBuildPlacementService' --timeout 120`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestTerranEconomyProductionOrderExpander' --timeout 120`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestTerranOpeningPlanScheduler' --timeout 120`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestCommandAuthorityScheduling' --timeout 120`
- `cmd /c Build.bat --target tutorial`

The following validations also succeeded after the synthesized shared-rail fallback was added:
- `cmd /c BuildAllTests.bat`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestTerranBuildPlacementService' --timeout 120`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestTerranEconomyProductionOrderExpander' --timeout 120`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestTerranOpeningPlanScheduler' --timeout 120`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestCommandAuthorityScheduling' --timeout 120`
- `cmd /c Build.bat --target tutorial`

The following validations succeeded after the requested-queue-count opening-plan change:
- `cmd /c BuildAllTests.bat`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestTerranOpeningPlanScheduler' --timeout 120`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestTerranEconomyProductionOrderExpander' --timeout 120`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestCommandAuthorityScheduling' --timeout 120`
- `cmd /c Build.bat --target tutorial`

The following validations succeeded after the standardized task-descriptor slice:
- `cmd /c Build.bat --target tutorial`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestTerranOpeningPlanScheduler' --timeout 180`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestCommandAuthorityScheduling' --timeout 180`
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestTerranEconomyProductionOrderExpander' --timeout 180`

Current unrelated regression still present on this branch:
- `& 'L:\Sc2_Bot\RunTests.bat' --filter 'sc2::TestTerranBuildPlacementService' --timeout 120`
  - failing authored-rail placement expectations in `tests/test_terran_build_placement_service.cc`
  - this slice did not modify `FTerranBuildPlacementService` or that test file
  - treat it as an existing placement-test mismatch to address separately from the queue-depth work

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
- the last confirmed live run before the newest fallback code showed `ProductionRail None`
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
- preserve the user's preferred reference chain for the opening line:
  - first factory uses the resolved first barracks as its reference
  - first starport uses the resolved first factory as its reference
  - hard blocks choose another candidate chain
  - soft blocks keep the same exact slot id and retry

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
- Resolve the opening production rail as a chained exact group instead of a rigid snapped group:
  - use the authored factory slot as the candidate pivot
  - resolve the barracks rail pad backward from the resolved factory point
  - resolve the starport pad forward from the resolved factory point
  - keep alternate candidate translations and mirrored orientation for hard-block fallback
  - keep exact-slot scheduler retries for soft blocks

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
1. Commit the new standardized scheduler task descriptor once the user is ready to save this slice.
2. Use `FCommandTaskDescriptor` for the first rudimentary runtime package trigger, starting with a simple supply or expansion package.
3. Thread runtime-triggered task descriptors into the same strategic-order seeding path already used by the opener.
4. After one or two runtime package triggers are working, remove the special-case time-gated opener mirror from `FTerranTimingAttackBuildPlanner`.
5. Keep live validation on `cmd /c LaunchTerranEasyComputerMatch.bat` against the Medium opponent.

## Notes
- `notes/MainBaseLayoutNotes.md` exists and should be updated together with any new placement report.
- A previous live run emitted civetweb port bind noise on `8080`. That looked environmental rather than placement-specific.
- `examples/tutorial.cc` now supports a bounded live-run override through the `SC2_TUTORIAL_MAX_GAME_LOOP` environment variable.
- Do not forget to commit the restored `RESUME.md` itself.
