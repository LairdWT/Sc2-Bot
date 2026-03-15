#include <cstdlib>
#include <iostream>
#include <string>

#if defined(_WIN32)
#include <crtdbg.h>
#include <stdlib.h>
#include <windows.h>
#endif

#include "sc2utils/sc2_manage_process.h"
#include "test_agent_execution_telemetry.h"
#include "test_actions.h"
#include "test_app.h"
#include "test_command_authority_scheduling.h"
#include "test_feature_layer.h"
#include "test_feature_layer_mp.h"
#include "test_movement_combat.h"
#include "test_map_paths.h"
#include "test_multiplayer.h"
#include "test_observation_interface.h"
#include "test_performance.h"
#include "test_rendered.h"
#include "test_restart.h"
#include "test_scheduler_hot_path_profiles.h"
#include "test_singularity_framework.h"
#include "test_snapshots.h"
#include "test_terran_descriptor_pipeline.h"
#include "test_terran_build_placement_service.h"
#include "test_terran_economic_models.h"
#include "test_terran_goal_task_dictionary.h"
#include "test_terran_army_order_pipeline.h"
#include "test_terran_bot_scaffolding.h"
#include "test_terran_opening_plan_scheduler.h"
#include "test_terran_economy_production_order_expander.h"
#include "test_terran_ramp_wall_controller.h"
#include "test_terran_planners.h"
#include "test_unit_command.h"

namespace sc2
{
bool TestAbilityRemap(int argc, char** argv);
}

static bool ShouldWaitOnExit() {
    const char* wait_on_exit_value = std::getenv("SC2_WAIT_ON_EXIT");
    if (!wait_on_exit_value) {
        return false;
    }

    const std::string wait_on_exit_string = wait_on_exit_value;
    return wait_on_exit_string == "1" || wait_on_exit_string == "true" || wait_on_exit_string == "TRUE";
}

static bool ShouldRunTest(const std::string& TestName) {
    const char* test_filter_value = std::getenv("SC2_TEST_FILTER");
    if (!test_filter_value) {
        return true;
    }

    return TestName == test_filter_value;
}

#if defined(_WIN32)
static void ConfigureWindowsErrorHandling() {
    ::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);

#if defined(_DEBUG)
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDERR);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif
}
#endif

#define TEST(X)                                                    \
    if (ShouldRunTest(#X))                                         \
    {                                                              \
        std::cout << "Running test: " << #X << std::endl;          \
        if (X(argc, argv))                                         \
        {                                                          \
            std::cout << "Test: " << #X << " succeeded." << std::endl; \
        }                                                          \
        else                                                       \
        {                                                          \
            success = false;                                       \
            std::cerr << "Test: " << #X << " failed!" << std::endl; \
        }                                                          \
    }

int main(int argc, char* argv[])
{
#if defined(_WIN32)
    ConfigureWindowsErrorHandling();
#endif

    bool success = true;

    TEST(sc2::TestMapPaths);
    TEST(sc2::TestAbilityRemap);
    TEST(sc2::TestSnapshots);
    TEST(sc2::TestMultiplayer);
    TEST(sc2::TestMovementCombat);
    TEST(sc2::TestFastRestartSinglePlayer);
    TEST(sc2::TestUnitCommand);
    TEST(sc2::TestSchedulerHotPathProfiles);
    TEST(sc2::TestPerformance);
    TEST(sc2::TestObservationInterface);
    TEST(sc2::TestSingularityFramework);
    TEST(sc2::TestTerranEconomicModels);
    TEST(sc2::TestTerranGoalTaskDictionary);
    TEST(sc2::TestTerranArmyOrderPipeline);
    TEST(sc2::TestAgentExecutionTelemetry);
    TEST(sc2::TestCommandAuthorityScheduling);
    TEST(sc2::TestTerranBuildPlacementService);
    TEST(sc2::TestTerranBotScaffolding);
    TEST(sc2::TestTerranDescriptorPipeline);
    TEST(sc2::TestTerranEconomyProductionOrderExpander);
    TEST(sc2::TestTerranPlanners);
    TEST(sc2::TestTerranOpeningPlanScheduler);
    TEST(sc2::TestTerranRampWallController);

#ifdef BUILD_SC2_RENDERER
    TEST(sc2::TestRendered);
#endif

    if (success)
    {
        std::cout << "All tests succeeded!" << std::endl;
    }
    else
    {
        std::cerr << "Some tests failed!" << std::endl;
    }

    if (ShouldWaitOnExit())
    {
        std::cout << "SC2_WAIT_ON_EXIT is enabled. Hit any key to exit..." << std::endl;
        while (!sc2::PollKeyPress())
        {
        }
    }

    return success ? 0 : -1;
}
