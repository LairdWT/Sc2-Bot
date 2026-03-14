# Deterministic Placement Patterns

## Scope

- TopicId: `SC2-ECOSYSTEM-DETERMINISTIC-PLACEMENT-PATTERNS`
- Domain: `EcosystemPlacement`
- Target seam: `TerranAgent` placement flow and `FTerranBuildPlacementService`
- Source authority: local code under `L:\Sc2_Bot\examples` first; no external references used in this pass

## Local Placement Pipeline

- `TerranAgent::OnStep` invokes `InitializeMainBaseLayoutDescriptor` before planner expansion (`L:\Sc2_Bot\examples\terran\terran.cc:610`).
- `TerranAgent::InitializeMainBaseLayoutDescriptor` builds `FBuildPlacementContext` then calls `IBuildPlacementService::GetMainBaseLayoutDescriptor` (`L:\Sc2_Bot\examples\terran\terran.cc:713`, `L:\Sc2_Bot\examples\terran\terran.cc:721`, `L:\Sc2_Bot\examples\terran\terran.cc:724`).
- `TerranAgent::InitializeRampWallDescriptor` follows the same context-first pattern for `GetRampWallDescriptor` (`L:\Sc2_Bot\examples\terran\terran.cc:702`, `L:\Sc2_Bot\examples\terran\terran.cc:705`).
- `TerranAgent::CreateBuildPlacementContext` carries map and geometry inputs used for deterministic slot resolution: `MapName`, `BaseLocation`, nearest expansion as `NaturalLocation`, `PlayableMin`, `PlayableMax`, plus cached `RampWallDescriptor` and `MainBaseLayoutDescriptor` (`L:\Sc2_Bot\examples\terran\terran.cc:772`).
- `FTerranBuildPlacementService::GetStructurePlacementSlots` emits ability-specific slot lists from resolved descriptors and falls back to one `MainSupportStructure` anchor slot for unknown abilities (`L:\Sc2_Bot\examples\common\services\FTerranBuildPlacementService.cc:2652`).

## Deterministic Mechanics In Local Service

- Authored-first resolution: `GetMainBaseLayoutDescriptor` queries `FTerranMainBaseLayoutRegistry::TryGetAuthoredMainBaseLayout` and scores mirror candidates before selecting the preferred resolved layout (`L:\Sc2_Bot\examples\common\services\FTerranBuildPlacementService.cc:2387`, `L:\Sc2_Bot\examples\common\services\FTerranBuildPlacementService.cc:2454`, `L:\Sc2_Bot\examples\common\services\FTerranMainBaseLayoutRegistry.cc:188`).
- Runtime anchor recovery: when authored production anchors are unavailable, service attempts `TryResolveProductionLayoutAnchorPoint` with primary and mirrored lateral direction fallback (`L:\Sc2_Bot\examples\common\services\FTerranBuildPlacementService.cc:2512`, `L:\Sc2_Bot\examples\common\services\FTerranBuildPlacementService.cc:2519`).
- Slot family materialization is explicit and ordered through `AppendTemplateSlotsToLayoutDescriptor` for depot, barracks, factory, and starport groups (`L:\Sc2_Bot\examples\common\services\FTerranBuildPlacementService.cc:2539`, `L:\Sc2_Bot\examples\common\services\FTerranBuildPlacementService.cc:2550`, `L:\Sc2_Bot\examples\common\services\FTerranBuildPlacementService.cc:2561`, `L:\Sc2_Bot\examples\common\services\FTerranBuildPlacementService.cc:2592`, `L:\Sc2_Bot\examples\common\services\FTerranBuildPlacementService.cc:2603`).
- Production rail fallback can be derived from resolved barracks seeds via `TryResolveDerivedProductionRailSlotGroup` when authored production rail slots are absent (`L:\Sc2_Bot\examples\common\services\FTerranBuildPlacementService.cc:2579`).
- Assembly point is deterministic from wall state: wall outside staging when valid, otherwise forward offset from primary anchor (`L:\Sc2_Bot\examples\common\services\FTerranBuildPlacementService.cc:2635`).

## Integration Implications For Future Ecosystem Comparison

- Candidate external patterns are only useful if they preserve the local authored-first then runtime-recovery ordering.
- Any imported pattern must keep ability-routed slot families compatible with `GetStructurePlacementSlots` switch behavior.
- Any imported pattern must preserve context inputs already carried by `FBuildPlacementContext` instead of bypassing this seam.
- No unresolved ambiguity was opened by this topic pass.
