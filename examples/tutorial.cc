#include <iostream>
#include "sc2api/sc2_api.h"
#include "sc2renderer/sc2_renderer.h"
#include "sc2utils/sc2_manage_process.h"

#include "terran/terran.h"

#include "common/render_settings.h"

int main(int argc, char* argv[])
{
    TerranAgent Agent;

    sc2::Coordinator Coordinator;
    if (!Coordinator.LoadSettings(argc, argv))
    {
        return 1;
    }

    static const sc2::FeatureLayerSettings Settings(CAMERA_WIDTH, FEATURE_LAYER_SIZE, FEATURE_LAYER_SIZE, FEATURE_LAYER_SIZE, FEATURE_LAYER_SIZE);
    Coordinator.SetFeatureLayers(Settings);
    Coordinator.SetParticipants({CreateParticipant(sc2::Race::Terran, &Agent), CreateComputer(sc2::Race::Zerg, sc2::Difficulty::Medium)});
    Coordinator.LaunchStarcraft();
    Coordinator.StartGame(sc2::kMapBelShirVestigeLE);
    while (Coordinator.Update())
    {
        if (sc2::PollKeyPress())
        {
            break;
        }
    }

    std::cout << "Game ended, Press any key to continue..." << std::endl;
    return 0;
}
