# Action Dispatch And Event Ordering

## Scope

- TopicId: `SC2-API-ACTION-DISPATCH-EVENT-ORDERING`
- Domain: `ApiActionDispatch`
- Focus: exact ordering and data handoff between `ActionInterface::UnitCommand(...)`, `ActionInterface::Commands()`, `ControlImp::IssueEvents(...)`, and `TerranAgent::ExecuteResolvedIntents(...)`.

## Source Anchors

- `L:\Sc2_Bot\examples\terran\terran.cc`
  - `TerranAgent::ExecuteResolvedIntents(...)`
  - `TerranAgent::OnStep()`
- `L:\Sc2_Bot\src\sc2api\sc2_interfaces.h`
  - `class ActionInterface`
  - `ActionInterface::Commands() const`
  - `ActionInterface::SendActions()`
- `L:\Sc2_Bot\src\sc2api\sc2_agent.cc`
  - `class ActionImp`
  - `ActionImp::UnitCommand(...)`
  - `ActionImp::SendActions()`
- `L:\Sc2_Bot\src\sc2api\sc2_client.cc`
  - `ControlImp::IssueEvents(const Tags& commands)`
- `L:\Sc2_Bot\src\sc2api\sc2_coordinator.cc`
  - `CallOnStep(Agent* a)`
  - `CoordinatorImp::StepAgents()`
  - `CoordinatorImp::StepAgentsRealtime()`

## Verified Ordering

- `CallOnStep(Agent* a)` executes:
  - `control->IssueEvents(action->Commands())`
  - then `action->SendActions()`
- `ControlImp::IssueEvents(...)` executes:
  - unit and upgrade event emitters
  - then `client_.OnStep()`
- Result:
  - `TerranAgent::OnStep()` runs before current-frame command batch dispatch.
  - commands queued by `TerranAgent::ExecuteResolvedIntents(...)` are sent by `ActionImp::SendActions()` after `OnStep()` returns.

## Command Buffer Semantics

- `ActionImp::UnitCommand(...)` appends `SC2APIProtocol::ActionRawUnitCommand` items to one batched request.
- `ActionImp::SendActions()`:
  - clears `commands_`
  - sends the batched request
  - repopulates `commands_` from each dispatched action `unit_tags`
  - waits for response via `control_.WaitForResponse()`
- `ActionInterface::Commands()` therefore exposes the tags from the most recently dispatched batch, consumed on the next `IssueEvents(...)` call.

## Terran Integration Implications

- `TerranAgent::ExecuteResolvedIntents(...)` maps `EIntentTargetKind::{None, Point, Unit}` to the tag-based `Actions()->UnitCommand(...)` overloads.
- `TerranAgent::UpdateDispatchedSchedulerOrders(...)` cannot assume immediate same-frame acknowledgment from action dispatch; it must use observation and lifecycle checks, which matches current implementation.
- If deterministic immediate confirmation is required, integration must add explicit local bookkeeping beyond `ActionInterface::Commands()` and idle-event inference.

## Ambiguities

- None opened in this topic pass.