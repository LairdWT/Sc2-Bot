#include <cstdlib>
#include <iostream>
#include <limits>

#include "sc2api/sc2_api.h"
#include "sc2renderer/sc2_renderer.h"
#include "sc2utils/sc2_manage_process.h"

#include "terran/terran.h"

#include "common/render_settings.h"

namespace
{

uint32_t GetUnsignedIntegerEnvironmentOverride(const char* VariableNamePtrValue)
{
    const char* VariableTextPtrValue = std::getenv(VariableNamePtrValue);
    if (VariableTextPtrValue == nullptr || VariableTextPtrValue[0] == '\0')
    {
        return 0U;
    }

    char* ParseEndPtrValue = nullptr;
    const unsigned long ParsedValue = std::strtoul(VariableTextPtrValue, &ParseEndPtrValue, 10);
    if (ParseEndPtrValue == VariableTextPtrValue || (ParseEndPtrValue != nullptr && ParseEndPtrValue[0] != '\0'))
    {
        std::cerr << "Ignoring invalid " << VariableNamePtrValue << " value: " << VariableTextPtrValue << std::endl;
        return 0U;
    }

    if (ParsedValue > std::numeric_limits<uint32_t>::max())
    {
        return std::numeric_limits<uint32_t>::max();
    }

    return static_cast<uint32_t>(ParsedValue);
}

}  // namespace

int main(int argc, char* argv[])
{
    sc2::TerranAgent agent;
    const uint32_t MaxGameLoopOverrideValue =
        GetUnsignedIntegerEnvironmentOverride("SC2_TUTORIAL_MAX_GAME_LOOP");
    const uint32_t TimeoutOverrideValue =
        GetUnsignedIntegerEnvironmentOverride("SC2_TUTORIAL_TIMEOUT_MS");
    const uint32_t PortStartOverrideValue =
        GetUnsignedIntegerEnvironmentOverride("SC2_TUTORIAL_PORT_START");

    sc2::Coordinator coordinator;
    if (!coordinator.LoadSettings(argc, argv))
    {
        return 1;
    }

    static const sc2::FeatureLayerSettings settings(CAMERA_WIDTH, FEATURE_LAYER_SIZE, FEATURE_LAYER_SIZE,
                                                    FEATURE_LAYER_SIZE, FEATURE_LAYER_SIZE);
    coordinator.SetFeatureLayers(settings);
    coordinator.SetPortStart(PortStartOverrideValue > 0U ? static_cast<int>(PortStartOverrideValue) : 16680);
    coordinator.SetTimeoutMS(TimeoutOverrideValue > 0U ? TimeoutOverrideValue : 300000U);
    coordinator.SetParticipants(
        {CreateParticipant(sc2::Race::Terran, &agent), CreateComputer(sc2::Race::Random, sc2::Difficulty::Easy)});
    coordinator.LaunchStarcraft();
    coordinator.StartGame(sc2::kMapBelShirVestigeLE);
    while (coordinator.Update())
    {
        const sc2::ObservationInterface* ObservationPtrValue = agent.Observation();
        if (MaxGameLoopOverrideValue > 0U && ObservationPtrValue != nullptr &&
            ObservationPtrValue->GetGameLoop() >= MaxGameLoopOverrideValue)
        {
            std::cout << "Reached SC2_TUTORIAL_MAX_GAME_LOOP=" << MaxGameLoopOverrideValue
                      << ", ending tutorial match early." << std::endl;
            coordinator.LeaveGame();
            break;
        }

        if (sc2::PollKeyPress())
        {
            break;
        }
    }

    std::cout << "Game ended, Press any key to continue..." << std::endl;
    return 0;
}
