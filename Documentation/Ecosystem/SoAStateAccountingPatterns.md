# SoA State Accounting Patterns

## Scope

- TopicId: `SC2-ECOSYSTEM-SOA-STATE-ACCOUNTING`
- Domain: `EcosystemDataOrientedDesign`
- Target seam: `TerranAgent` scheduler, state rebuild, and telemetry accounting surfaces
- Source authority: local code under `L:\Sc2_Bot\examples` first; no external references used in this pass

## Local SoA Surfaces

- `FTerranUnitContainer` is explicit structure-of-arrays storage keyed by shared index across `ControlledUnits`, `Tags`, `UnitTypes`, positional data, health data, order data, and harvest data columns (`L:\Sc2_Bot\examples\common\terran_unit_container.h:50`).
- `FTerranUnitContainer::SetUnits` pre-reserves each column to `NewUnits.size()` before append, then `AddUnit` appends one row across every column (`L:\Sc2_Bot\examples\common\terran_unit_container.h:204`, `L:\Sc2_Bot\examples\common\terran_unit_container.h:208`, `L:\Sc2_Bot\examples\common\terran_unit_container.h:237`).
- Column alignment is guarded by `FTerranUnitContainer::HasSynchronizedSizes`, which validates every column count against `ControlledUnits.size()` (`L:\Sc2_Bot\examples\common\terran_unit_container.h:240`).
- `FCommandAuthoritySchedulingState` stores one order row as many parallel vectors (`OrderIds`, `LifecycleStates`, `ActorTags`, `AbilityIds`, targets, deferral, dispatch metadata) and keeps `OrderIdToIndex` as lookup indirection (`L:\Sc2_Bot\examples\common\planning\FCommandAuthoritySchedulingState.h:24`, `L:\Sc2_Bot\examples\common\planning\FCommandAuthoritySchedulingState.h:59`, `L:\Sc2_Bot\examples\common\planning\FCommandAuthoritySchedulingState.h:103`).
- `FCommandAuthoritySchedulingState::Reserve` and `EnqueueOrder` enforce column-wise preallocation and lockstep writes for scheduler rows (`L:\Sc2_Bot\examples\common\planning\FCommandAuthoritySchedulingState.cc:99`, `L:\Sc2_Bot\examples\common\planning\FCommandAuthoritySchedulingState.cc:146`).
- `FBuildPlanningState` uses fixed-size type-indexed arrays for observed units, units-in-construction, observed buildings, and buildings-in-construction (`L:\Sc2_Bot\examples\common\build_planning\FBuildPlanningState.h:11`, `L:\Sc2_Bot\examples\common\build_planning\FBuildPlanningState.h:38`).
- `FTerranGameStateDescriptorBuilder::RebuildBuildPlanningState` copies SoA counters from `FAgentState` into `FBuildPlanningState` once per rebuild, keeping planner reads on indexed arrays instead of ad hoc scans (`L:\Sc2_Bot\examples\common\descriptors\FTerranGameStateDescriptorBuilder.cc:106`).

## Integration Seam Implications

- `TerranAgent::RebuildGameStateDescriptor` is the state-accounting boundary where `ObservationInterface`-derived unit snapshots become planner-facing arrays (`L:\Sc2_Bot\examples\terran\terran.cc:727`).
- `TerranAgent::GetObservedCountForOrder` and `TerranAgent::GetObservedInConstructionCountForOrder` consume `FBuildPlanningState` arrays directly when confirming scheduler order outcomes (`L:\Sc2_Bot\examples\terran\terran.cc:1498`, `L:\Sc2_Bot\examples\terran\terran.cc:1577`).
- `TerranAgent::UpdateDispatchedSchedulerOrders` evaluates lifecycle transitions by comparing dispatch snapshots against current array-backed observed counts (`L:\Sc2_Bot\examples\terran\terran.cc:1401`).

## Local Gaps To Watch

- `FAgentExecutionTelemetry::RecentEvents` is an append-plus-erase window (`push_back`, then `erase(begin)`), so telemetry history is not SoA and incurs front-erase movement cost by design (`L:\Sc2_Bot\examples\common\telemetry\FAgentExecutionTelemetry.cc:262`).
- No new ambiguity was opened for the scheduler or build-planning SoA model in this pass.
