#include "test_map_paths.h"

#include <iostream>
#include <string>

#include "sc2api/sc2_game_settings.h"
#include "sc2utils/sc2_manage_process.h"

namespace sc2 {

bool TestMapPaths(int argc, char** argv) {
    (void)argc;
    (void)argv;

    const std::string LibraryMapsDirectory = GetLibraryMapsDirectory();
    if (LibraryMapsDirectory.empty()) {
        std::cerr << "Library maps directory could not be resolved." << std::endl;
        return false;
    }

    static const char* ExpectedMaps[] = {
        kMapEmpty,
        kMapEmptyLong,
        kMapEmptyTall,
        kMapMarineMicro,
        kMapBelShirVestigeLE,
    };

    for (const char* ExpectedMap : ExpectedMaps) {
        const std::string FullMapPath = LibraryMapsDirectory + ExpectedMap;
        if (!DoesFileExist(FullMapPath)) {
            std::cerr << "Expected map not found: " << FullMapPath << std::endl;
            return false;
        }
    }

    return true;
}

}  // namespace sc2
