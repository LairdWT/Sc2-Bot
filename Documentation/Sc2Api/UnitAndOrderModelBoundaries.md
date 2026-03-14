# Unit And Order Model Boundaries

## Purpose

This page documents the exact `Unit`, `UnitOrder`, `Tag`, `ABILITY_ID`, and `UNIT_TYPEID` surfaces that the owned Terran seam uses for scheduler bookkeeping, recovery behavior, and resolved execution.

Primary local sources:

- `L:\Sc2_Bot\examples\terran\terran.h`
- `L:\Sc2_Bot\examples\terran\terran.cc`
- `L:\Sc2_Bot\examples\common\agent_framework.h`
- `L:\Sc2_Bot\examples\common\planning\FCommandOrderRecord.h`
- `L:\Sc2_Bot\examples\common\planning\FCommandAuthoritySchedulingState.h`
- `L:\Sc2_Bot\examples\common\planning\FCommandAuthoritySchedulingState.cc`
- `L:\Sc2_Bot\examples\common\planning\FIntentSchedulingService.cc`
- `L:\Sc2_Bot\src\sc2api\sc2_unit.h`
- `L:\Sc2_Bot\src\sc2api\sc2_gametypes.h`
- `L:\Sc2_Bot\src\sc2api\sc2_typeenums.h`
- `L:\Sc2_Bot\src\sc2api\sc2_interfaces.h`

## Canonical API Types At The Seam

- `Tag`
  - Defined as `typedef uint64_t Tag;` with `NullTag = 0LL` in `sc2_gametypes.h`.
  - Used as the stable identity key for actor resolution and unit-target resolution.
- `AbilityID` and `ABILITY_ID`
  - `AbilityID` is `SC2Type<ABILITY_ID>` in `sc2_typeenums.h`.
  - Owned code compares and dispatches by `ABILITY_ID` values such as `HARVEST_GATHER`, `SMART`, `RALLY_UNITS`, `TRAIN_MARINE`, and build or morph abilities.
- `UnitTypeID` and `UNIT_TYPEID`
  - `UnitTypeID` is `SC2Type<UNIT_TYPEID>` in `sc2_typeenums.h`.
  - Owned code projects observed and planned state by exact `UNIT_TYPEID` keys, including flying or morphed equivalents when counting progress.
- `UnitOrder`
  - Carries `ability_id`, `target_unit_tag`, `target_pos`, and `progress` in `sc2_unit.h`.
  - Owned scheduler and recovery logic reads `ability_id` and `target_unit_tag` as command-intent evidence.
- `Unit`
  - Provides identity (`tag`), type (`unit_type`), dynamic orders (`orders`), producer/attachment state (`add_on_tag`), and economy state (`assigned_harvesters`, `ideal_harvesters`, `vespene_contents`).
  - `ObservationInterface` exposes `const Unit*` snapshots; command authority does not mutate `Unit` directly.

## Observation And Query Boundaries

- `ObservationInterface`
  - `GetUnits(...)` and `GetUnit(Tag)` are authoritative read paths for current unit snapshots.
  - `GetRawActions()` is available but Terran scheduling in `terran.cc` confirms execution from observed unit orders instead of consuming raw-action mirrors.
- `QueryInterface`
  - Used in intent normalization for placement and pathing checks before dispatch (`FIntentArbiter::ValidateAndNormalize(...)`).
- `ActionInterface`
  - `TerranAgent::ExecuteResolvedIntents(...)` issues `Actions()->UnitCommand(...)` by actor `Tag`, `AbilityID`, and target kind.

## Owned Scheduler Data Contract

`FCommandOrderRecord` and `FCommandAuthoritySchedulingState` preserve command intent in an SoA container where the core identity and command columns are:

- `ActorTag` / `ActorTags`
- `AbilityId` / `AbilityIds`
- `TargetKind` / `TargetKinds`
- `TargetPoint` / `TargetPoints`
- `TargetUnitTag` / `TargetUnitTags`
- `ProducerUnitTypeId` / `ProducerUnitTypeIds`
- `ResultUnitTypeId` / `ResultUnitTypeIds`

Lifecycle and bookkeeping columns that bind order state back to observation include:

- `LifecycleState`, `DispatchStep`, `DispatchGameLoop`, `DispatchAttemptCount`
- `ObservedCountAtDispatch`, `ObservedInConstructionCountAtDispatch`
- deferral fields (`LastDeferralReason`, `LastDeferralStep`, `LastDeferralGameLoop`)

The SoA structure is materialized to `FCommandOrderRecord` only when queried through `GetOrderRecord(...)`.

## Unit And Order Usage In Terran Flow

- Recovery and worker assignment:
  - Reads `UnitOrder` front-order `ability_id` and `target_unit_tag` to determine mineral vs refinery commitment.
  - Resolves target tags through `ObservationInterface::GetUnit(Tag)` and validates refinery type by `UNIT_TYPEID`.
- Planned production accounting:
  - `CountOrdersAndIntentsForAbility(...)` combines observed `UnitOrder.ability_id` with buffered `FUnitIntent.Ability`.
- Dispatch confirmation:
  - `UpdateDispatchedSchedulerOrders(...)` compares current observed counts and in-construction counts against dispatch-time baselines keyed by `ResultUnitTypeId`.
  - `HasProducerConfirmedDispatchedOrder(...)` checks producer `Unit` state (`unit_type`, `add_on_tag`, and current `orders`) against the dispatched `AbilityId`.
- Final command emission:
  - `ExecuteResolvedIntents(...)` maps `EIntentTargetKind::{None,Point,Unit}` to the corresponding `ActionInterface::UnitCommand(...)` overloads.

## Boundary Rules Captured For Future Work

- Use `Tag` for identity joins; never infer identity from position or iteration order.
- Treat `const Unit*` as frame-scoped observation data; persist only scalar IDs and enums in scheduler state.
- Keep command identity (`AbilityID`/`ABILITY_ID`) and result identity (`UNIT_TYPEID`) as separate fields; they encode different confirmation semantics.
- For build and morph confirmation, compare against both canonical and equivalent observed type families where local counting logic already does so (for example flying or lowered variants).
- Keep target-kind dispatch explicit (`None`, `Point`, `Unit`) to avoid ambiguous API calls.