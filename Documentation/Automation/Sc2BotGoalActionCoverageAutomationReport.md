# Sc2 Bot Goal Action Coverage Automation Report

## Run Summary

- RunTimeUtc: 2026-03-15T19:38:01Z
- MatrixPath: L:\Sc2_Bot\Documentation\Automation\Sc2BotGoalActionCoverageMatrix.md
- OpenGoalRows: 22
- OpenActionRows: 35
- UnavailableSuites: 0
- FailedSuites: 1

## Current Unresolved Goal Coverage

- GoalId 100 `HoldOwnedBase` -> PartiallyCovered. Missing: docs. Consumer: `FCommandAuthorityProcessor`.
- GoalId 110 `SaturateWorkers` -> PartiallyCovered. Missing: docs. Consumer: `FTerranTimingAttackBuildPlanner`.
- GoalId 210 `BuildRefineryCapacity` -> PartiallyCovered. Missing: docs. Consumer: `FTerranTimingAttackBuildPlanner`.
- GoalId 220 `BuildBarracksCapacity` -> PartiallyCovered. Missing: docs. Consumer: `FTerranTimingAttackBuildPlanner`.
- GoalId 221 `BuildFactoryCapacity` -> PartiallyCovered. Missing: docs. Consumer: `FTerranTimingAttackBuildPlanner`.
- GoalId 222 `BuildStarportCapacity` -> PartiallyCovered. Missing: docs. Consumer: `FTerranTimingAttackBuildPlanner`.
- GoalId 230 `UnlockBarracksReactor` -> PartiallyCovered. Missing: docs. Consumer: `FTerranTimingAttackBuildPlanner`.
- GoalId 231 `UnlockFactoryTechLab` -> PartiallyCovered. Missing: docs. Consumer: `FTerranTimingAttackBuildPlanner`.
- GoalId 232 `UnlockEngineeringBay` -> PartiallyCovered. Missing: docs. Consumer: `FTerranTimingAttackBuildPlanner`.
- GoalId 233 `UnlockStarportReactor` -> PartiallyCovered. Missing: docs. Consumer: `FTerranTimingAttackBuildPlanner`.
- GoalId 240 `ResearchStimpack` -> PartiallyCovered. Missing: docs. Consumer: `FTerranTimingAttackBuildPlanner`.
- GoalId 241 `ResearchCombatShield` -> PartiallyCovered. Missing: docs. Consumer: `FTerranTimingAttackBuildPlanner`.
- GoalId 242 `ResearchInfantryWeaponsLevel1` -> PartiallyCovered. Missing: docs. Consumer: `FTerranTimingAttackBuildPlanner`.
- GoalId 243 `ResearchConcussiveShells` -> PartiallyCovered. Missing: docs. Consumer: `FTerranTimingAttackBuildPlanner`.
- GoalId 300 `ProduceMarines` -> PartiallyCovered. Missing: docs. Consumer: `FTerranTimingAttackBuildPlanner`.
- GoalId 301 `ProduceMarauders` -> PartiallyCovered. Missing: docs. Consumer: `FTerranTimingAttackBuildPlanner`.
- GoalId 302 `ProduceCyclones` -> PartiallyCovered. Missing: docs. Consumer: `FTerranTimingAttackBuildPlanner`.
- GoalId 303 `ProduceSiegeTanks` -> PartiallyCovered. Missing: docs. Consumer: `FTerranTimingAttackBuildPlanner`.
- GoalId 304 `ProduceMedivacs` -> PartiallyCovered. Missing: docs. Consumer: `FTerranTimingAttackBuildPlanner`.
- GoalId 305 `ProduceLiberators` -> PartiallyCovered. Missing: docs. Consumer: `FTerranTimingAttackBuildPlanner`.
- GoalId 410 `ClearEnemyPresence` -> PartiallyCovered. Missing: docs. Consumer: `FCommandAuthorityProcessor`.
- GoalId 420 `ScoutExpansionLocations` -> PartiallyCovered. Missing: docs. Consumer: `FCommandAuthorityProcessor`.

## Current Unresolved Action Coverage

- `ABILITY_ID::ATTACK_ATTACK` -> PartiallyCovered. Missing: docs. Sources: FTerranArmyUnitExecutionPlanner, TerranAgentDirectIntent.
- `ABILITY_ID::BUILD_BARRACKS` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:BuildBarracks.
- `ABILITY_ID::BUILD_BUNKER` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:BuildBunker.
- `ABILITY_ID::BUILD_COMMANDCENTER` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:BuildCommandCenter.
- `ABILITY_ID::BUILD_ENGINEERINGBAY` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:BuildEngineeringBay.
- `ABILITY_ID::BUILD_FACTORY` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:BuildFactory.
- `ABILITY_ID::BUILD_REACTOR_BARRACKS` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:BuildBarracksReactor.
- `ABILITY_ID::BUILD_REACTOR_FACTORY` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:BuildFactoryReactor.
- `ABILITY_ID::BUILD_REACTOR_STARPORT` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:BuildStarportReactor.
- `ABILITY_ID::BUILD_REFINERY` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:BuildRefinery.
- `ABILITY_ID::BUILD_STARPORT` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:BuildStarport.
- `ABILITY_ID::BUILD_SUPPLYDEPOT` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:BuildSupplyDepot.
- `ABILITY_ID::BUILD_TECHLAB_BARRACKS` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:BuildBarracksTechLab.
- `ABILITY_ID::BUILD_TECHLAB_FACTORY` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:BuildFactoryTechLab.
- `ABILITY_ID::BURROWDOWN` -> PartiallyCovered. Missing: docs. Sources: FTerranArmyUnitExecutionPlanner.
- `ABILITY_ID::BURROWUP` -> PartiallyCovered. Missing: docs. Sources: FTerranArmyUnitExecutionPlanner.
- `ABILITY_ID::EFFECT_HEAL` -> PartiallyCovered. Missing: docs. Sources: FTerranArmyUnitExecutionPlanner.
- `ABILITY_ID::GENERAL_HOLDPOSITION` -> PartiallyCovered. Missing: docs. Sources: FTerranArmyUnitExecutionPlanner.
- `ABILITY_ID::GENERAL_MOVE` -> PartiallyCovered. Missing: docs. Sources: FTerranEconomyProductionOrderExpander.
- `ABILITY_ID::MORPH_SIEGEMODE` -> PartiallyCovered. Missing: docs. Sources: FTerranArmyUnitExecutionPlanner.
- `ABILITY_ID::MORPH_UNSIEGE` -> PartiallyCovered. Missing: docs. Sources: FTerranArmyUnitExecutionPlanner.
- `ABILITY_ID::MOVE_MOVE` -> PartiallyCovered. Missing: docs. Sources: FTerranArmyUnitExecutionPlanner.
- `ABILITY_ID::RESEARCH_COMBATSHIELD` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:ResearchCombatShield.
- `ABILITY_ID::RESEARCH_CONCUSSIVESHELLS` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:ResearchConcussiveShells.
- `ABILITY_ID::RESEARCH_STIMPACK` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:ResearchStimpack.
- `ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONSLEVEL1` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:ResearchTerranInfantryWeaponsLevel1.
- `ABILITY_ID::TRAIN_CYCLONE` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:TrainCyclone.
- `ABILITY_ID::TRAIN_HELLION` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:TrainHellion.
- `ABILITY_ID::TRAIN_LIBERATOR` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:TrainLiberator.
- `ABILITY_ID::TRAIN_MARAUDER` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:TrainMarauder.
- `ABILITY_ID::TRAIN_MEDIVAC` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:TrainMedivac.
- `ABILITY_ID::TRAIN_SCV` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:TrainScv.
- `ABILITY_ID::TRAIN_SIEGETANK` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:TrainSiegeTank.
- `ABILITY_ID::TRAIN_VIKINGFIGHTER` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:TrainVikingFighter.
- `ABILITY_ID::TRAIN_WIDOWMINE` -> PartiallyCovered. Missing: docs. Sources: TaskTemplate:TrainWidowMine.

## Focused Suite Status

- `sc2::TestTerranGoalTaskDictionary` -> availability `Available`, execution `Passed`.
- `sc2::TestTerranPlanners` -> availability `Available`, execution `Passed`.
- `sc2::TestTerranEconomyProductionOrderExpander` -> availability `Available`, execution `Passed`.
- `sc2::TestTerranArmyOrderPipeline` -> availability `Available`, execution `Passed`.
- `sc2::TestCommandAuthorityScheduling` -> availability `Available`, execution `Passed`.
- `sc2::TestTerranDescriptorPipeline` -> availability `Available`, execution `Passed`.
- `sc2::TestTerranBotScaffolding` -> availability `Available`, execution `Passed`.
- `sc2::TestUnitCommand` -> availability `Available`, execution `Failed`.
- `sc2::TestObservationInterface` -> availability `Available`, execution `Passed`.

## Notes

- Documentation evidence excludes the generated goal-action matrix and generated unresolved report to avoid self-coverage.
- `OutOfScope` is reserved for valid SC2 API surfaces not currently consumed by the owned Terran bot; this run inventories only owned Terran goal and emitted-action surfaces.
