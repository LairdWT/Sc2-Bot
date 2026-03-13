#include <iostream>

#include "sc2api/sc2_api.h"
#include "sc2renderer/sc2_renderer.h"
#include "sc2utils/sc2_manage_process.h"

#include "terran/terran.h"

#include "common/render_settings.h"

int main(int argc, char* argv[])
{
    sc2::TerranAgent agent;

    sc2::Coordinator coordinator;
    if (!coordinator.LoadSettings(argc, argv))
    {
        return 1;
    }

    static const sc2::FeatureLayerSettings settings(CAMERA_WIDTH, FEATURE_LAYER_SIZE, FEATURE_LAYER_SIZE,
                                                    FEATURE_LAYER_SIZE, FEATURE_LAYER_SIZE);
    coordinator.SetFeatureLayers(settings);
    coordinator.SetPortStart(16680);
    coordinator.SetTimeoutMS(300000U);
    coordinator.SetParticipants(
        {CreateParticipant(sc2::Race::Terran, &agent), CreateComputer(sc2::Race::Random, sc2::Difficulty::Easy)});
    coordinator.LaunchStarcraft();
    coordinator.StartGame(sc2::kMapBelShirVestigeLE);
    while (coordinator.Update())
    {
        if (sc2::PollKeyPress())
        {
            break;
        }
    }

    std::cout << "Game ended, Press any key to continue..." << std::endl;
    return 0;
}
