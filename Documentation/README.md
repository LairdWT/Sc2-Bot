# Documentation

This folder contains bot-owned architecture and planning references for the Terran bot work in this repository, plus source-backed API integration notes, ecosystem leverage notes, and automation artifacts that keep those documents current.

## Scope

- These documents apply to code we own under `examples/common` and `examples/terran`.
- The shipped StarCraft II API and upstream third-party code under `src` and `thirdparty` are not refactor targets for this plan.
- `Documentation/Sc2Api` documents how our owned Terran code uses the checked-in API.
- `Documentation/Ecosystem` records external leverage notes that may inform our design goals, but those notes do not override local source behavior.
- `Documentation/Automation` stores ledgers and generated reports for the documentation-deepening automations.
- The root [CodingStandards.md](../CodingStandards.md) remains the governing standard for all future implementation work.

## Guiding Principles

- Keep authoritative runtime state in data-oriented stores.
- Rebuild public descriptors deterministically from authoritative state each frame.
- Separate strategic planning, squad planning, spatial analysis, and intent execution into explicit domains.
- Use interfaces and composition between planning systems instead of concentrating behavior in `TerranAgent`.
- Stage command work through bounded queues and ready buffers when producer and consumer lifecycles differ.
- Keep new types one-per-file and keep naming aligned with the project coding standards.

## Reference Index

### Owned Terran Architecture
- [TerranBot/ArchitectureOverview.md](TerranBot/ArchitectureOverview.md)
- [TerranBot/GameStateDescriptors.md](TerranBot/GameStateDescriptors.md)
- [TerranBot/ArmyAndSquadDomain.md](TerranBot/ArmyAndSquadDomain.md)
- [TerranBot/BuildPlanningAndBudgeting.md](TerranBot/BuildPlanningAndBudgeting.md)
- [TerranBot/IntentSchedulingAndCommandAuthority.md](TerranBot/IntentSchedulingAndCommandAuthority.md)
- [TerranBot/SpatialFieldsAndHotspots.md](TerranBot/SpatialFieldsAndHotspots.md)
- [TerranBot/TelemetryAndGameRecordStore.md](TerranBot/TelemetryAndGameRecordStore.md)
- [TerranBot/ExecutionRoadmap.md](TerranBot/ExecutionRoadmap.md)
- [TerranBot/ImplementationBacklog.md](TerranBot/ImplementationBacklog.md)
- [TerranBot/TerranAgentCoordinatorPath.md](TerranBot/TerranAgentCoordinatorPath.md)

### Source-Backed API Integration
- [Sc2Api/README.md](Sc2Api/README.md)
- [Sc2Api/TerranAgentApiIntegration.md](Sc2Api/TerranAgentApiIntegration.md)

### External Leverage Notes
- [Ecosystem/README.md](Ecosystem/README.md)

### Automation Artifacts
- [Automation/README.md](Automation/README.md)
- [Automation/Sc2BotTerranDocumentationAutomationReport.md](Automation/Sc2BotTerranDocumentationAutomationReport.md)
- [Automation/Sc2BotApiRefinementLedger.md](Automation/Sc2BotApiRefinementLedger.md)
- [Automation/Sc2BotApiSkillAutomationReport.md](Automation/Sc2BotApiSkillAutomationReport.md)
- [Automation/Sc2BotGoalActionCoverageMatrix.md](Automation/Sc2BotGoalActionCoverageMatrix.md)
- [Automation/Sc2BotGoalActionCoverageAutomationReport.md](Automation/Sc2BotGoalActionCoverageAutomationReport.md)

## Current Intent

The near-term goal is architectural cleanliness and a strong foundation for state-driven decision making. The first major gameplay target is a two-base Marine, Marauder, Medivac timing with home-defense tanks, then a transition toward Marine, Marauder, Medivac, Liberator with Viking support when the matchup or enemy air presence demands it.
