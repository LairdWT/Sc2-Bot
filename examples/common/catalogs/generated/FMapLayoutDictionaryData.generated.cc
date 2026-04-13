#include "common/catalogs/generated/FMapLayoutDictionaryData.generated.h"

namespace sc2
{

const std::vector<FMapDescriptor> GMapDescriptors = {
    // BelShir Vestige LE
    FMapDescriptor{
        EMapId::BelShirVestige,
        Point2D(0.0f, 0.0f),       // PlayableMin
        Point2D(200.0f, 176.0f),    // PlayableMax
        Point2D(100.0f, 88.0f),     // MapCenter
        {"belshirvestige", "belshirvestigele"},  // NormalizedNames

        // Bases
        {
            // Base 0: NW main (top-left starting location)
            FMapBaseDescriptor{0U, Point2D(29.5f, 134.5f), true,
                {Point2D(24.0f, 138.0f), Point2D(24.5f, 135.5f), Point2D(25.0f, 133.0f),
                 Point2D(27.0f, 131.0f), Point2D(22.0f, 139.5f), Point2D(23.5f, 141.0f),
                 Point2D(26.0f, 142.0f), Point2D(28.5f, 141.5f)}, 8U,
                {Point2D(25.5f, 130.5f), Point2D(22.5f, 138.5f)}, 2U},

            // Base 1: NW natural expansion
            FMapBaseDescriptor{1U, Point2D(44.5f, 112.5f), false,
                {Point2D(49.0f, 114.0f), Point2D(50.0f, 111.5f), Point2D(49.5f, 109.0f),
                 Point2D(48.0f, 107.0f), Point2D(44.0f, 116.5f), Point2D(46.5f, 117.0f),
                 Point2D(42.0f, 115.0f), Point2D(40.5f, 113.5f)}, 8U,
                {Point2D(48.5f, 116.5f), Point2D(40.5f, 108.5f)}, 2U},

            // Base 2: Center-left third
            FMapBaseDescriptor{2U, Point2D(29.5f, 88.5f), false,
                {}, 0U, {}, 0U},

            // Base 3: Center-right third
            FMapBaseDescriptor{3U, Point2D(114.5f, 88.5f), false,
                {}, 0U, {}, 0U},

            // Base 4: SE natural expansion
            FMapBaseDescriptor{4U, Point2D(96.5f, 47.5f), false,
                {}, 0U, {}, 0U},

            // Base 5: SE main (bottom-right starting location)
            FMapBaseDescriptor{5U, Point2D(114.5f, 25.5f), true,
                {Point2D(120.0f, 22.0f), Point2D(119.5f, 24.5f), Point2D(119.0f, 27.0f),
                 Point2D(117.0f, 29.0f), Point2D(122.0f, 20.5f), Point2D(120.5f, 19.0f),
                 Point2D(118.0f, 18.0f), Point2D(115.5f, 18.5f)}, 8U,
                {Point2D(118.5f, 29.5f), Point2D(121.5f, 21.5f)}, 2U},
        },

        // Ramps
        {
            // Ramp 0: NW main to NW natural (SE-facing)
            FMapRampDescriptor{0U,
                {Point2DI(43, 133), Point2DI(44, 133), Point2DI(45, 133)}, 3U,
                {Point2DI(43, 129), Point2DI(44, 129), Point2DI(45, 129)}, 3U,
                Point2D(44.0f, 133.0f), Point2D(44.0f, 129.0f),
                ERampOrientation::SouthEast, 3U, 0U, 1U},

            // Ramp 1: SE main to SE natural (NW-facing)
            FMapRampDescriptor{1U,
                {Point2DI(99, 27), Point2DI(100, 27), Point2DI(101, 27)}, 3U,
                {Point2DI(99, 31), Point2DI(100, 31), Point2DI(101, 31)}, 3U,
                Point2D(100.0f, 27.0f), Point2D(100.0f, 31.0f),
                ERampOrientation::NorthWest, 3U, 5U, 4U},
        },

        // Watchtowers
        {},

        // DestructibleRocks
        {},

        // GroundDistances
        {
            FMapGroundDistanceEntry{0U, 1U, 45.0f},
            FMapGroundDistanceEntry{0U, 5U, 160.0f},
            FMapGroundDistanceEntry{1U, 4U, 100.0f},
            FMapGroundDistanceEntry{1U, 5U, 130.0f},
            FMapGroundDistanceEntry{4U, 5U, 45.0f},
        },

        // Spawns
        {
            // NW spawn (top-left, SE-facing ramp)
            FMapSpawnLayout{
                ESpawnId::NorthWest, 0U, 1U, 0U,
                ERampOrientation::SouthEast, ESpawnId::SouthEast,
                {0U, 1U, 2U, 3U, 4U, 5U, 0xFFU, 0xFFU}, 6U,
                FMapRampWallLayout{
                    Point2D(44.0f, 132.0f), Point2D(41.5f, 132.5f),
                    Point2D(46.0f, 133.0f), Point2D(43.0f, 130.0f), true},
                FMapProductionColumnLayout{3, -1},
                FMapNaturalWallLayout{4.0f, 3.5f, -2.5f}},

            // SE spawn (bottom-right, NW-facing ramp)
            FMapSpawnLayout{
                ESpawnId::SouthEast, 5U, 4U, 1U,
                ERampOrientation::NorthWest, ESpawnId::NorthWest,
                {5U, 4U, 3U, 2U, 1U, 0U, 0xFFU, 0xFFU}, 6U,
                FMapRampWallLayout{
                    Point2D(100.0f, 28.0f), Point2D(100.5f, 27.5f),
                    Point2D(98.0f, 27.0f), Point2D(101.0f, 30.0f), true},
                FMapProductionColumnLayout{-3, 1},
                FMapNaturalWallLayout{4.0f, 3.5f, -2.5f}},
        },
    },
};

}  // namespace sc2
