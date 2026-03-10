# Singularity Overview

## Scope

This review assumes `Singularity` is the Terran bot wired through `examples/tutorial.cc` and implemented in `examples/terran/terran.h` and `examples/terran/terran.cc`. It was refreshed after rebasing local `master` onto `origin/master`, and it reflects the current code at the time of review. This is still a static review; I did not rely on a full local build to reach these findings.

## Executive Summary

The bot is noticeably closer to its stated goals than it was before the rebase. Upstream changes introduced a real structure-of-arrays `FTerranUnitContainer`, static Terran economic tables, and derived counts for units and buildings that are already in construction. Those changes move the project toward the intended data-oriented design.

The main remaining problems are now correctness issues in that new data pipeline and the fact that feature layers still do not influence decisions. The bot still behaves like a rules bot with random targeting rather than a texture-informed state machine, and there are a few concrete bugs that can break production accounting or issue invalid commands.

## Findings

### 1. High: idle barracks fall through into the marine attack case

`OnUnitIdle()` handles `TERRAN_BARRACKS` by calling `TryBuildMarine()`, but there is no `break` before the next `TERRAN_MARINE` case (`examples/terran/terran.cc:68-78`). That means an idle barracks falls through and receives the marine behavior too, which issues `ATTACK_ATTACK` toward `BarracksRally`.

At best this produces a steady stream of invalid commands against an immobile production structure. At worst it obscures whether the barracks training logic is behaving correctly, because every idle event mixes a valid production action with an impossible combat order.

Recommended fix: add an explicit `break` after `TryBuildMarine()` in the barracks case.

### 2. High: build actions still compete for the same worker within one frame

The new state correctly tracks buildings already under construction, but `OnStepBuildUpdate()` still runs `TryBuildSupplyDepot()` and `TryBuildBarracks()` back-to-back in the same frame (`examples/terran/terran.cc:136-140`). `TryBuildStructure()` uses `AgentState.UnitContainer.GetIdleWorker()` or the same worker snapshot for both decisions, and nothing reserves that worker after the first build command is issued (`examples/terran/terran.cc:142-203`, `examples/common/terran_unit_container.h:648-659`).

So the same SCV can still be selected for a depot and then immediately reused for a barracks before the next observation arrives. The new in-construction counters fix inter-frame accounting, but they do not solve this same-frame command conflict.

Recommended fix: add a per-step worker reservation or command-intent buffer so once a worker is chosen for a structure, later build passes cannot reuse it until the next observation.

### 3. Medium: `FTerranUnitContainer::ResetAll()` leaves `UnitBuffs` out of sync

`AddUnit()` appends a new entry to `UnitBuffs` for every unit (`examples/common/terran_unit_container.h:161`), but `ResetAll()` does not clear `UnitBuffs` when the container is rebuilt from a fresh observation (`examples/common/terran_unit_container.h:204-232`).

That breaks the SoA invariant after the first update: most arrays represent the current frame, while `UnitBuffs` still starts with stale entries from previous frames. Any filter that reads `UnitBuffs[Index]`, such as `FilterByUnitBuffActive()`, will then be working against mismatched data (`examples/common/terran_unit_container.h:470-476`).

Recommended fix: clear `UnitBuffs` inside `ResetAll()` alongside the other per-unit arrays.

### 4. Medium: building counts double-count add-ons as barracks, factories, and starports

The Terran building table includes add-on unit types like `TERRAN_BARRACKSREACTOR`, `TERRAN_BARRACKSTECHLAB`, `TERRAN_FACTORYREACTOR`, and `TERRAN_STARPORTTECHLAB` (`examples/common/terran_models.h:189-220`). `FAgentBuildings::GetBarracksCount()`, `GetFactoryCount()`, and `GetStarportCount()` then sum those add-on entries together with the base production structure counts (`examples/common/bot_status_models.h:381-399`).

That inflates production structure totals as soon as add-ons exist. A single barracks with a tech lab can be represented as both a barracks-related structure and a tech-lab-related structure, which makes the aggregate count look larger than the actual number of production buildings available.

Recommended fix: keep add-ons tracked separately from the parent production structures, or compute production-building totals from the parent structure types only.

### 5. Medium: the feature-layer pipeline is still not operational

The entry point still enables feature layers (`examples/tutorial.cc:21-23`), and `TerranAgent` still caches render and minimap feature-layer pointers (`examples/terran/terran.cc:10-13`, `examples/terran/terran.cc:30-37`). But those textures are not consumed anywhere in the decision loop. The only related logic in `OnStep()` remains commented-out debug rendering.

The new state system is a better data-oriented foundation, but it still derives decisions from raw counts and hard-coded thresholds rather than from map textures, influence data, or real state transitions. That means the core design goal you described is still not implemented, even though the scaffolding is getting stronger.

Recommended fix: add a derived-state pass that turns feature-layer data into compact metrics, then drive strategy and action selection from those metrics instead of bypassing them.

### 6. Medium: build and combat targeting still ignore placement, pathing, and map bounds

The bot still generates build and attack targets by adding only positive random offsets to worker positions, the start location, or the enemy start location (`examples/terran/terran.cc:17-18`, `examples/terran/terran.cc:75-77`, `examples/terran/terran.cc:149-152`, `examples/terran/terran.cc:198-201`). There is still no `Query()->Placement()` validation for structures and no pathing or playable-area clamp for combat targets.

So even after the rebase, the bot can still request off-map targets, blocked build cells, or otherwise invalid positions. Because the logic retries these actions every step, those bad targets can turn into persistent command spam instead of a single failed attempt.

Recommended fix: derive legal target regions from `GameInfo`, clamp randomized positions to the playable rectangle, and validate structure placement before sending the command.

### 7. Medium: worker mining still only recognizes one mineral field type

`FindNearestMineralPatch()` still only accepts `UNIT_TYPEID::NEUTRAL_MINERALFIELD` (`examples/terran/terran.cc:299-313`). The API already provides a broader mineral predicate in `src/sc2api/sc2_unit_filters.cc:35-48` that covers the normal ladder mineral variants.

On maps that expose a different mineral field type, idle SCVs can still miss the nearest patch and fall back to `ATTACK_ATTACK` toward the enemy start location (`examples/terran/terran.cc:57-65`).

Recommended fix: replace the manual equality check with `sc2::IsMineralPatch()`.

### 8. Low: the example target is still packaged by including a `.cc` file directly

`examples/tutorial.cc` still includes both `terran/terran.h` and `terran/terran.cc` (`examples/tutorial.cc:6-7`). This works for the current example target, but it still hides source ownership from the build graph and makes future refactors easy to break through duplicate compilation.

Recommended fix: add `terran/terran.cc` to the example target in CMake and include only the header from `tutorial.cc`.

## What Improved Upstream

- `FAgentState` now carries economy, unit, and building state through a real update path instead of only ad hoc counters.
- `FTerranUnitContainer` is a meaningful move toward the project’s data-oriented goal.
- Construction-in-progress tracking is now explicit for both units and buildings.
- Marine production is no longer solely dependent on the `OnUnitIdle` callback.

## Suggested Next Steps

1. Fix the barracks fallthrough and the missing `UnitBuffs.clear()` first, because both are correctness bugs in the current implementation.
2. Add a per-frame command reservation layer so structure-building systems cannot fight over the same SCV.
3. Separate parent production structures from add-ons in the building aggregates.
4. Replace random target generation with legal target selection based on map bounds, placement checks, and pathing validation.
5. Start feeding feature-layer-derived metrics into `FAgentState` so the data-oriented architecture becomes the actual decision engine instead of just a reporting layer.
