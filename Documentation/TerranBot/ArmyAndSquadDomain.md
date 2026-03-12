# Army And Squad Domain

## Purpose

The bot needs an explicit hierarchy for coordinated combat behavior. An army is a theater-level formation with a broader goal. A squad is a subordinate package inside an army with a concrete role and local behavior.

This hierarchy should support:

- a persistent main army
- home-defense detachments
- reaper scouting
- hellion harassment
- medivac drop packages
- future multi-front pressure without breaking the main plan

## Core Rules

- The bot must always have at least one army, even if that army has no units or squads.
- `ArmyIndex 0` should always exist and serve as the default primary army anchor.
- Every squad belongs to exactly one army.
- Every combat-capable unit is either assigned to exactly one squad or remains in an explicit reserve pool.
- New armies should be created only for meaningfully separate objectives or theaters, not simply because an existing army is not yet full.
- Existing armies should be refilled and expanded when the objective, location, and posture still match.

## Army-Level Responsibilities

An army owns the theater-level intent.

Planned responsibilities:

- overall goal
- target theater or defended region
- posture
- desired composition summary
- staging point
- fallback point
- reinforcement policy
- engagement permissions

Proposed goal enum:

- `EArmyGoal::MapControl`
- `EArmyGoal::HoldBase`
- `EArmyGoal::FrontalAssault`
- `EArmyGoal::TimingAttack`
- `EArmyGoal::Feint`
- `EArmyGoal::Backstab`
- `EArmyGoal::Surround`

Proposed posture enum:

- `EArmyPosture::Assemble`
- `EArmyPosture::Advance`
- `EArmyPosture::Contain`
- `EArmyPosture::Engage`
- `EArmyPosture::Disengage`
- `EArmyPosture::Retreat`
- `EArmyPosture::Regroup`

## Squad-Level Responsibilities

A squad owns a local role inside the army.

Planned responsibilities:

- role assignment
- local target or area
- required composition
- local micro posture
- reinforcement threshold
- merge and split eligibility

Proposed role enum:

- `ESquadRole::Frontline`
- `ESquadRole::Support`
- `ESquadRole::Harass`
- `ESquadRole::Drop`
- `ESquadRole::Scout`
- `ESquadRole::SiegeDefense`
- `ESquadRole::Escort`

## Authoritative Storage Direction

The authoritative domain should be stored in data-oriented arrays rather than nested heap-owned objects.

Planned authoritative stores:

- `FArmyDomainState`
- `FSquadDomainState`
- `FArmyUnitAssignmentState`

Example authoritative army columns:

- army identifiers
- army goals
- army postures
- home region identifiers
- objective region identifiers
- staging points
- fallback points
- desired supply values
- current supply values
- active squad index lists

Example authoritative squad columns:

- squad identifiers
- owner army indices
- squad roles
- squad posture values
- anchor points
- desired composition values
- current composition values
- assigned unit index lists

Descriptors should reconstruct tree-style views for planners and logging from these backing arrays.

## New Army Creation Rules

A new army should be considered only when one of these conditions is true:

- a base must be defended by a persistent detached force
- a transport or harassment package needs a separate theater and posture
- two objectives require incompatible movement or engagement behavior
- a scouting package must persist independently from the main force
- a flank or surround package needs separate pathing and timing

A new army should not be created only because:

- the current main army is below desired supply
- a squad needs reinforcements
- the bot has produced new units with no assignment yet

## Initial Army Layout For The First Major Plan

The first target composition suggests these standing army and squad patterns:

- Main Army:
  two-base Marine, Marauder, Medivac timing force
- Home Defense Army:
  base-defense tanks and supporting units assigned to hold vulnerable bases
- Scout Squad:
  single reaper squad with independent scouting role
- Harass Squad:
  two-hellion harassment package against Zerg when the strategic plan allows it

This does not require all armies to exist from the opening frame. It requires the domain to support them cleanly when conditions are met.

## Coordination Flow

The intended flow is:

1. Strategic direction sets the active army goals.
2. Army planning determines which armies should exist and what they should achieve.
3. Squad coordination assigns roles and required composition inside each army.
4. Micro logic consumes local spatial fields and emits intents for each squad.

## Validation Rules

- An army cannot reference a missing squad.
- A squad cannot reference a missing army.
- A unit cannot belong to more than one squad at once.
- Army supply totals must equal the sum of assigned squad and reserve composition for that army.
- Deleting or merging a squad must release all owned unit assignments deterministically.
