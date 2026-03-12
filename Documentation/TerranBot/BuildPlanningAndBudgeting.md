# Build Planning And Budgeting

## Purpose

The bot needs a hierarchical planning model for spending minerals, vespene, and supply accurately. A simple linear build order is not enough for the intended architecture because the bot must react to map state, army composition, and threat response while still preserving timing windows.

## Hierarchical Planning Model

The planned structure is:

- `FBuildPlanningState`
- `FBuildPackageDescriptor`
- `FBuildNeedDescriptor`
- `FResourceBudgetLedger`

The intent is:

- a package represents a broader goal
- a need represents one concrete requirement inside that package
- the budget ledger ensures that accepted needs reserve resources explicitly

## Package Types

Proposed package categories:

- `EBuildPackageKind::Opening`
- `EBuildPackageKind::TimingAttack`
- `EBuildPackageKind::HomeDefense`
- `EBuildPackageKind::Expansion`
- `EBuildPackageKind::TechTransition`
- `EBuildPackageKind::AirResponse`
- `EBuildPackageKind::UpgradeWindow`
- `EBuildPackageKind::SquadComposition`

Examples:

- the two-base Marine, Marauder, Medivac timing should be a timing attack package
- tanks for base defense should be a home defense package
- third base and fourth base moves should be expansion packages
- Liberator and Viking transition should be a tech transition or air response package depending on trigger

## Need Types

Each package should contain concrete needs with explicit completion and failure rules.

Proposed need categories:

- `EBuildNeedKind::Structure`
- `EBuildNeedKind::AddOn`
- `EBuildNeedKind::Unit`
- `EBuildNeedKind::Upgrade`
- `EBuildNeedKind::Expansion`
- `EBuildNeedKind::WorkerSaturation`
- `EBuildNeedKind::StaticDefense`
- `EBuildNeedKind::SquadRequirement`

Each need should describe:

- resource cost
- supply impact
- producer requirement
- prerequisite requirements
- desired timing window
- target army or squad if applicable
- current lifecycle state

Proposed lifecycle enum:

- `EBuildNeedState::Proposed`
- `EBuildNeedState::Reserved`
- `EBuildNeedState::InProgress`
- `EBuildNeedState::Completed`
- `EBuildNeedState::Blocked`
- `EBuildNeedState::Abandoned`

## Resource Budget Ledger

`FResourceBudgetLedger` should be the single authoritative budget view.

Planned tracked values:

- current minerals
- current vespene
- current supply used, cap, and available
- reserved minerals
- reserved vespene
- reserved supply
- committed minerals
- committed vespene
- committed supply
- predicted minerals at planning horizon
- predicted vespene at planning horizon

Budgeting rules:

- reserve resources when a need is accepted into active planning
- commit resources when a command is issued for the need
- release resources when a need fails, is canceled, or is superseded
- prevent double-booking across planners
- track supply as a first-class budget dimension, not only minerals and vespene

## Priority And Arbitration

Packages and needs need a stable priority model so the bot can react without losing its larger plan.

Suggested priority order:

1. survival and emergency defense
2. supply continuity
3. core production continuity
4. active timing package commitments
5. expansion packages
6. tech transition and non-critical upgrades

Strategic direction should set broad priorities, but the ledger should enforce the final resource reality.

## Interaction With Armies And Squads

The build planning domain should be able to answer questions such as:

- which army is currently under-supplied
- whether a squad composition requirement is blocked by missing production capacity
- whether the main timing attack can launch on time
- whether a third base can be taken without breaking the current army timing

The army and squad domains should request composition and reinforcement needs through build packages rather than bypassing the budget ledger.

## Initial Package Set For The First Major Milestone

The first gameplay milestone should likely include these packages:

- opening package
- main timing attack package
- home defense package
- natural expansion package
- third base package
- infantry upgrade package
- starport transition package

## Validation Rules

- No active need may reserve resources without a ledger entry.
- No resource reservation may exist without an owning package and need.
- A need blocked on prerequisites must not appear as ready for execution.
- A command cannot be issued if it would violate the ledger after reservation and commitment checks.
- Supply budgeting must account for planned unit production and near-term depot completion or risk explicitly.
