# Terran Bot Execution Roadmap

## Purpose

This roadmap turns the current architecture discussion into an execution order. The intent is to build the foundation first, then layer behavior on top of it without collapsing back into a monolithic bot file.

## Phase 0: Foundation Corrections

Correct the known unsafe and unstable areas in the current bot-owned code before expanding behavior.

Primary targets:

- economic table sizing and indexing safety
- invalid Terran type fallback handling
- intent pathing validation
- repeated queued attack command behavior
- deterministic structure placement anchors

Exit criteria:

- no known out-of-bounds economic lookups remain
- invalid type lookups fail safely
- structure placement no longer depends on worker-local random scatter
- the army intent loop does not accumulate stale queued movement or attack commands

## Phase 1: Domain Scaffolding

Create the initial domain and interface surfaces without implementing the full behavior set.

Primary targets:

- descriptor type definitions
- authoritative domain store definitions
- planner interface definitions
- service interface definitions
- file layout aligned with `CodingStandards.md`

Exit criteria:

- the target domains exist in code with clear ownership boundaries
- `TerranAgent` can depend on interfaces instead of future concrete planner logic

## Phase 2: Descriptor Rebuild Pipeline

Implement deterministic rebuilds of authoritative state and high-level descriptors.

Primary targets:

- economy descriptor rebuild
- map descriptor rebuild
- army and squad descriptor rebuild
- build planning descriptor rebuild
- spatial summary descriptor rebuild

Exit criteria:

- a root `FGameStateDescriptor` exists each frame
- higher-level planners can consume descriptors without querying raw observations directly

## Phase 3: Build Placement And Expansion Services

Replace random placement with map-aware services.

Primary targets:

- wall-off candidate evaluation
- natural expansion identification
- third and fourth expansion scoring
- safe rally and staging point selection
- base defense anchor point selection

Exit criteria:

- depots and barracks can be placed in sensible early-game positions
- expansions can be selected from explicit candidate scoring

## Phase 4: Army And Squad Domain

Implement persistent armies, squads, reserve pools, and assignment rules.

Primary targets:

- army creation rules
- squad creation and merge rules
- unit assignment bookkeeping
- standing scout and harassment package support
- defense detachment support

Exit criteria:

- the bot can maintain at least one always-valid army
- squads can exist as first-class planning objects
- units can move between reserve and squads deterministically

## Phase 5: Build Planning And Budget Ledger

Implement goal-driven spending control.

Primary targets:

- package and need model
- budget ledger
- package priority model
- composition-driven production requests
- expansion and upgrade reservations

Exit criteria:

- the bot can explain why resources are reserved and for which active package
- army composition needs and macro needs do not double-book resources

## Phase 6: Spatial Fields And Hotspots

Implement coarse field generation and hotspot extraction.

Primary targets:

- enemy threat fields
- friendly influence fields
- retreat safety fields
- staging and defense hotspot extraction

Exit criteria:

- planners can consume stable spatial summaries
- micro-facing systems have a clean future path to higher-resolution field use

## Phase 7: First Strategic Milestone

Implement the first end-to-end game plan.

Primary gameplay target:

- two-base Marine, Marauder, Medivac timing
- home-defense tanks
- one reaper scout
- two-hellion Zerg harassment follow-up when enabled by the strategic plan

Exit criteria:

- the bot can execute the plan through goals and packages instead of hard-coded one-off logic
- the army and squad domain is actively used
- build placement and expansion logic are service-driven

## Phase 8: Transition And Adaptation

Extend the first milestone into the intended stable mid-game composition.

Primary gameplay target:

- transition into Marine, Marauder, Medivac, Liberator
- add Vikings in TvT or heavy enemy air situations
- add third and fourth base logic
- upgrade timing aligned to the active plan and threat context

Exit criteria:

- the bot can transition plans without resetting its architecture
- map, army, and build descriptors all inform the transition choice

## Phase 9: Telemetry And Gameplay Records

Implement a versioned data trail for matches, decisions, and spatial summaries.

Primary targets:

- match manifest schema
- decision and state event store
- opponent-type labeling
- heatmap and field export format
- versioned replay and telemetry correlation

Exit criteria:

- each recorded match can be traced to a bot version and schema version
- decision and outcome data can be queried without scraping logs
- spatial exports can be correlated with matchup and result data

## Immediate Backlog Seed

The next implementation tasks should likely be:

1. correct safety issues in the current bot-owned foundational headers and source files
2. create the initial domain and descriptor files
3. extract a thin coordinator path out of `TerranAgent`
4. introduce deterministic placement and expansion services

## Definition Of Done For The Architecture Pass

The architecture pass should be considered successful when:

- the bot-owned code is split into explicit domains with one-type-per-file organization
- the current Terran bot behavior no longer depends on one large coordinator file for all decision logic
- resource budgeting, army organization, and spatial interpretation all have authoritative owners
- new gameplay behaviors can be added by extending planners and descriptors instead of rewriting the control loop
