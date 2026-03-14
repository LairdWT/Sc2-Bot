# Agent Event Dispatch And Action Flush Order

## Purpose

This page maps the exact callback and action-dispatch order across `sc2::Coordinator`, `ControlImp`, `Agent`, and owned `TerranAgent` integration.

Primary local sources:

- `L:\Sc2_Bot\src\sc2api\sc2_coordinator.cc`
- `L:\Sc2_Bot\src\sc2api\sc2_client.cc`
- `L:\Sc2_Bot\src\sc2api\sc2_client.h`
- `L:\Sc2_Bot\src\sc2api\sc2_interfaces.h`
- `L:\Sc2_Bot\src\sc2api\sc2_agent.cc`
- `L:\Sc2_Bot\examples\terran\terran.cc`

## Startup Order

`CoordinatorImp::JoinGame()` runs startup in this order for each `Agent`:

1. `Control()->GetObservation()`
2. `OnGameFullStart()`
3. `Control()->OnGameStart()`
4. `OnGameStart()`
5. `Control()->IssueEvents(Actions()->Commands())`

Implications for `TerranAgent::OnGameStart()`:

- `Observation()` is expected to be populated before `OnGameStart()`.
- `ControlImp::OnGameStart()` updates start location before `TerranAgent::OnGameStart()`.
- Initial event issuance after startup can trigger `OnUnitCreated` and `OnUnitIdle` before first scheduled coordinator step.

## Per-Step Dispatch Order

`CallOnStep(Agent* AgentValue)` in `sc2_coordinator.cc` runs:

1. `control->IssueEvents(action->Commands())`
2. `action->SendActions()`
3. `action_feature_layer->SendActions()`

`ControlImp::IssueEvents(const Tags& commands)` in `sc2_client.cc` runs:

1. `IssueUnitDestroyedEvents()`
2. `IssueUnitAddedEvents()`
3. `IssueBuildingCompletedEvents()`
4. `IssueIdleEvents(commands)`
5. `IssueUpgradeEvents()`
6. `IssueAlertEvents()`
7. `IssueUnitDamagedEvents()`
8. `client_.OnStep()`

Owned seam impact:

- `TerranAgent::OnUnitCreated(...)` and `TerranAgent::OnUnitIdle(...)` are delivered before `TerranAgent::OnStep()` each frame where events are emitted.
- `TerranAgent::OnStep()` batches intents into `Actions()->UnitCommand(...)`.
- Commands batched in `TerranAgent::OnStep()` are transmitted after `OnStep()` returns, in the same coordinator frame.

## Command Echo Semantics For Idle Detection

`ActionImp::SendActions()` clears and then repopulates `commands_` with dispatched `unit_tags`.

`ControlImp::IssueIdleEvents(const Tags& commands)` uses those tags to detect units that had commands in the prior send but now have no orders.

Boundary condition:

- Idle events are based on previously dispatched command tags plus unit-pool new-idle tracking, not on intents queued but never sent.

## TerranAgent Integration Points

`TerranAgent` callback handling tied to the SC2 event order:

- `OnUnitIdle(const Unit* UnitPtr)`
  - Inserts worker tags into `PendingRecoveryWorkers` for later recovery intents.
- `OnUnitCreated(const Unit* UnitPtr)`
  - Inserts production structure tags into `PendingProductionRallyStructureTags`.

`TerranAgent::OnStep()` consumers:

- `ProduceProductionRallyIntents()` drains `PendingProductionRallyStructureTags` when structures are complete and no higher-priority actor intent already exists.
- `ProduceRecoveryIntents(...)` consumes `PendingRecoveryWorkers`.

This preserves a deterministic event-to-intent handoff where callback-generated tags are accumulated before scheduler, production, wall, harvest, and recovery intent production in `OnStep()`.

## API Guarantees Used

- `ObservationInterface` validity guarantee during `OnGameStart` and `OnStep`.
- `ActionInterface` validity guarantee during `OnStep`.
- `ClientEvents` contract that a newly created unit can trigger both `OnUnitCreated` and `OnUnitIdle` when it has no rally/order.

## Remaining Ambiguities After This Pass

- None.
