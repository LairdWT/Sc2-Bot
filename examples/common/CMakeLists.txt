set(sc2_terran_bot_common_sources
    armies/EArmyGoal.cc
    armies/FArmyDomainState.cc
    build_planning/FBuildPlanningState.cc
    descriptors/EGamePlan.cc
    descriptors/EMacroPhase.cc
    descriptors/FGameStateDescriptor.cc
    descriptors/FMacroStateDescriptor.cc
    planning/ECommandAuthorityLayer.cc
    planning/EIntentPlaybackState.cc
    planning/EOrderLifecycleState.cc
    planning/EPlanningProcessorState.cc
    planning/FCommandAuthoritySchedulingState.cc
    planning/FCommandOrderRecord.cc
    planning/IArmyPlanner.cc
    planning/IArmyOrderExpander.cc
    planning/IBuildPlanner.cc
    planning/FIntentSchedulingService.cc
    planning/IIntentSchedulingService.cc
    planning/ISquadOrderExpander.cc
    planning/IStrategicDirector.cc
    planning/IUnitExecutionPlanner.cc
    services/FTerranBuildPlacementService.cc
    services/IBuildPlacementService.cc
    services/IExpansionSelectionService.cc
    services/ISpatialFieldBuilder.cc
    spatial/FSpatialFieldSet.cc)

add_library(sc2_terran_bot_common STATIC ${sc2_terran_bot_common_sources})

target_include_directories(sc2_terran_bot_common
    PUBLIC
        "${PROJECT_SOURCE_DIR}/examples")

target_link_libraries(sc2_terran_bot_common
    PUBLIC
        cpp_sc2)

set_target_properties(sc2_terran_bot_common PROPERTIES FOLDER examples)
