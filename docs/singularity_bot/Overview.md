# Singularity Overview

## Scope

This review assumes `Singularity` is the Terran bot wired through `examples/tutorial.cc` and implemented in `examples/terran/terran.h` and `examples/terran/terran.cc`. I did a static code review only; I did not rely on a full local build to reach these findings.

## Executive Summary

The current bot has the beginnings of the architecture you described: there is a central `FAgentState`, feature layers are enabled in the entry point, and there are renderer helpers that could visualize map textures. In practice, though, the current implementation is still a simple rule bot. The feature-layer data never feeds decisions, the state machine fields stay at their defaults, and several action paths can fight themselves hard enough to stall production or degrade combat behavior.

## Findings

### 1. High: the build planner can overwrite its own orders in the same frame

`OnStepBuildUpdate()` calls `TryBuildSupplyDepot()` and `TryBuildBarracks()` back-to-back, but both functions read from the same stale `ControlledUnits` snapshot (`examples/terran/terran.cc:168-170`). `TryBuildStructure()` then chooses a worker from that snapshot without reserving it for the rest of the frame (`examples/terran/terran.cc:173-225`).

That means the same SCV can be selected twice in one `OnStep()`: first for a depot and then immediately for a barracks, with the second command replacing the first. Because the in-progress counters are also derived from the stale snapshot, they do not protect against this intra-frame conflict (`examples/terran/terran.cc:228-300`).

Recommended fix: introduce a per-frame reservation set for workers and pending structure intents, or queue build decisions into a command buffer and submit them once after conflict resolution.

### 2. High: the marine attack routine reissues orders every frame and will thrash combat

Once the bot reaches 24 marines, `AllMarinesAttack()` runs on every step (`examples/terran/terran.cc:136-166`). It increments `m_CurrentStep` and then sends a fresh `MOVE_MOVE` or `ATTACK_ATTACK` command to every marine every frame, with a new random target each time.

In SC2 this kind of action spam usually cancels unit intent before it can complete, which leads to stutter-stepping without purpose, lost focus fire, and poor responsiveness to visible enemies. The comment says "divisible by 5" but the code actually alternates on `% 3`, which suggests the behavior is still exploratory rather than deliberate (`examples/terran/terran.cc:148-149`).

Recommended fix: only issue a new army command when the strategic target changes, or when a unit has completed or invalidated its current order.

### 3. Medium: worker mining logic only recognizes one mineral field type

`FindNearestMineralPatch()` only accepts `UNIT_TYPEID::NEUTRAL_MINERALFIELD` (`examples/terran/terran.cc:303-317`). The API already ships a broader mineral predicate in `src/sc2api/sc2_unit_filters.cc:35-48` that covers the common mineral variants, including rich, purifier, lab, and battle station nodes.

On maps that expose a different mineral field type, an idle SCV can fail to find a patch and fall into the `ATTACK_ATTACK` fallback in `OnUnitIdle()` (`examples/terran/terran.cc:55-63`). That turns a simple idle-worker recovery path into a possible economy failure.

Recommended fix: replace the manual equality check with `sc2::IsMineralPatch()`.

### 4. Medium: the state machine and feature-layer pipeline are mostly declarative, not operational

The entry point enables feature layers (`examples/tutorial.cc:21-23`), and `TerranAgent` stores pointers to both render and minimap feature data (`examples/terran/terran.cc:10-13`, `examples/terran/terran.cc:30-37`). But the bot never reads those textures to derive map control, vision, threat, scouting, or terrain-aware behavior. The only texture-related calls in `OnStep()` are commented-out debug draws.

The same pattern shows up in the "state machine". `FAgentState` defines progression, assessments, and strategy enums (`examples/common/bot_status_models.h:16-37`, `examples/common/bot_status_models.h:268-430`), but `UpdateAgentState()` only populates raw economy and army counts (`examples/terran/terran.cc:89-121`). The higher-level fields remain at constructor defaults, and the action logic bypasses them entirely in favor of direct thresholds and random offsets (`examples/terran/terran.cc:47-80`, `examples/terran/terran.cc:127-170`).

Recommended fix: add a derived-state pass that consumes observation plus feature-layer metrics, then drive build and combat policies exclusively from that derived state.

### 5. Medium: movement and build targets ignore bounds, pathing, and placement validation

Several commands generate positions by adding only positive random offsets to the start or enemy start location (`examples/terran/terran.cc:17-18`, `examples/terran/terran.cc:77-79`, `examples/terran/terran.cc:153-161`, `examples/terran/terran.cc:222-223`). There is no clamp to the playable area, no `Query()->PathingDistance()` check, and no `Query()->Placement()` check before issuing a build.

The result is that orders can be aimed off-map, into blocked terrain, or into invalid build cells, especially on spawns near the upper or right edges of the map. Because the bot retries these actions every step, invalid targets can turn into persistent command spam instead of one clean failure.

Recommended fix: derive legal target positions from `GameInfo`, clamp randomization to the playable rectangle, and validate structure placement before issuing a build command.

### 6. Low: the example packaging is brittle because `tutorial.cc` includes a `.cc` file directly

`examples/tutorial.cc` includes both `terran/terran.h` and `terran/terran.cc` (`examples/tutorial.cc:6-7`). This works for the current example target, but it hides the real source ownership from the build graph and creates an easy future ODR trap if `terran.cc` is ever added as a normal source file as well.

Recommended fix: add `terran/terran.cc` to the example target in CMake and include only the header from `tutorial.cc`.

## Architecture Notes

### What is already heading in the right direction

- `FAgentState` is a reasonable starting point for a data-oriented blackboard.
- `tutorial.cc` correctly enables feature layers, which is the right prerequisite for a texture-driven bot.
- The renderer helpers in `terran.h` are useful for debugging what the bot "sees".

### What is still missing for the stated design goal

- A pass that converts feature-layer textures into compact derived metrics.
- Real state transitions for progression, assessment, and strategy.
- A command-intent layer that prevents different systems from fighting over the same unit in the same frame.
- Bot-specific tests; the repository has framework and feature-layer tests, but nothing exercising `TerranAgent` behavior directly.

## Suggested Next Steps

1. Create a per-frame derived-state pipeline:
   `Observation -> texture metrics -> FAgentState -> action selection`.
2. Add worker and structure reservations so build decisions cannot overwrite each other within one frame.
3. Replace the continuous marine command spam with target persistence and event-driven retasking.
4. Use `IsMineralPatch`, `Query()->Placement()`, and map bounds clamping to harden basic economy and build behavior.
5. Add small deterministic tests around state derivation and action selection, even if full SC2 integration tests stay expensive.
