#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "common/catalogs/generated/EMapId.generated.h"
#include "common/catalogs/generated/ESpawnId.generated.h"
#include "common/services/FRampOrientationConstants.h"
#include "sc2api/sc2_common.h"

namespace sc2
{

// Describes a single expansion base location on the map.
// Contains exact resource positions for mineral patches and geysers.
struct FMapBaseDescriptor
{
public:
    // Ordered index for this base location (0-based, ascending)
    uint8_t BaseIndex;

    // Exact center position where a Command Center is placed
    Point2D Location;

    // Whether this position is a valid starting location for players
    bool bIsStartLocation;

    // Exact world positions of each individual mineral patch at this base
    std::array<Point2D, 8> MineralPatchPositions;

    // Number of valid entries in MineralPatchPositions
    uint8_t MineralPatchCount;

    // Exact world positions of each vespene geyser at this base
    std::array<Point2D, 2> GeyserPositions;

    // Number of valid entries in GeyserPositions
    uint8_t GeyserCount;
};

// Describes a single ramp connecting two elevation levels on the map.
// Contains exact tile positions for the top and bottom edges.
struct FMapRampDescriptor
{
public:
    // Ordered index for this ramp on the map
    uint8_t RampIndex;

    // Exact tile positions along the upper edge of the ramp
    std::array<Point2DI, 8> TopTiles;

    // Number of valid entries in TopTiles
    uint8_t TopTileCount;

    // Exact tile positions along the lower edge of the ramp
    std::array<Point2DI, 8> BottomTiles;

    // Number of valid entries in BottomTiles
    uint8_t BottomTileCount;

    // Center point of the upper ramp edge in world coordinates
    Point2D TopCenter;

    // Center point of the lower ramp edge in world coordinates
    Point2D BottomCenter;

    // Cardinal diagonal direction the ramp faces (NE/SE/SW/NW)
    ERampOrientation Orientation;

    // Number of tiles wide the ramp is at its narrowest
    uint8_t Width;

    // Base index connected at the top of this ramp
    uint8_t ConnectedBaseIndexA;

    // Base index connected at the bottom of this ramp
    uint8_t ConnectedBaseIndexB;
};

// Position of a Xel'Naga watchtower providing vision control
struct FMapWatchtower
{
public:
    // Exact world position of the watchtower
    Point2D Position;
};

// Position of destructible rocks that block pathing between bases
struct FMapDestructibleRock
{
public:
    // Exact world position of the destructible obstacle
    Point2D Position;

    // Base index on one side of the blocked path
    uint8_t BlocksBaseIndexA;

    // Base index on the other side of the blocked path
    uint8_t BlocksBaseIndexB;
};

// Precomputed ground pathing distance between two base locations
struct FMapGroundDistanceEntry
{
public:
    // Source base index
    uint8_t FromBaseIndex;

    // Destination base index
    uint8_t ToBaseIndex;

    // Approximate ground distance in world units
    float Distance;
};

// Authored ramp wall structure positions for a specific spawn location.
// Exact grid-snapped coordinates for the 3-structure wall-off.
struct FMapRampWallLayout
{
public:
    // Center point of the wall formation between the two depots
    Point2D WallCenter;

    // Exact barracks build point on the ramp wall
    Point2D BarracksPosition;

    // Exact left supply depot build point
    Point2D LeftDepotPosition;

    // Exact right supply depot build point
    Point2D RightDepotPosition;

    // Whether the barracks position supports addon construction
    bool bAddonValid;
};

// Grid-snapped offsets for production column relative to ramp barracks.
// Determined by ramp orientation.
struct FMapProductionColumnLayout
{
public:
    // Y-axis grid offset per row into the base (+3 or -3)
    int8_t DepthStep;

    // X-axis grid offset per row away from the ramp (+1 or -1)
    int8_t StaggerStep;
};

// Exact positions for natural expansion entrance wall structures.
// Grid-snapped authored positions — no direction math needed.
struct FMapNaturalWallLayout
{
public:
    // Exact position of the left flanking supply depot
    Point2D LeftDepotPosition;

    // Exact position of the center bunker
    Point2D BunkerPosition;

    // Exact position of the right flanking supply depot
    Point2D RightDepotPosition;

    // Exact position of the back depot behind the bunker
    Point2D BackDepotPosition;
};

// Complete layout configuration for a single spawn location on a map.
// Contains ramp wall positions, production column parameters, natural
// wall geometry, and expansion priority ordering.
struct FMapSpawnLayout
{
public:
    // Identifier for this spawn position on the map
    ESpawnId SpawnId;

    // Index into FMapDescriptor::Bases for the main base
    uint8_t MainBaseIndex;

    // Index into FMapDescriptor::Bases for the natural expansion
    uint8_t NaturalBaseIndex;

    // Index into FMapDescriptor::Ramps for the main base ramp
    uint8_t MainRampIndex;

    // Cardinal diagonal direction the main ramp faces
    ERampOrientation RampOrientation;

    // Spawn identifier for the enemy at the opposing position
    ESpawnId EnemySpawnId;

    // Preferred expansion order as base indices (0xFF = unused slot)
    std::array<uint8_t, 8> ExpansionOrder;

    // Number of valid entries in ExpansionOrder
    uint8_t ExpansionOrderCount;

    // Ramp wall structure positions for the main base
    FMapRampWallLayout RampWall;

    // Production column grid offsets for factory/starport/barracks
    FMapProductionColumnLayout ProductionColumn;

    // Natural expansion entrance wall geometry
    FMapNaturalWallLayout NaturalWall;
};

// Complete static description of a playable map including all base
// locations, ramp geometry, landmarks, distances, and spawn configurations.
// Populated at compile time from YAML source. Immutable at runtime.
struct FMapDescriptor
{
public:
    // Unique identifier for this map
    EMapId MapId;

    // Lower-left corner of the playable area
    Point2D PlayableMin;

    // Upper-right corner of the playable area
    Point2D PlayableMax;

    // Center point of the playable area
    Point2D MapCenter;

    // Lowercase normalized map name variants for string matching
    std::vector<std::string> NormalizedNames;

    // All expansion base locations on this map, indexed by BaseIndex
    std::vector<FMapBaseDescriptor> Bases;

    // All ramps on this map with exact tile geometry
    std::vector<FMapRampDescriptor> Ramps;

    // Xel'Naga watchtower positions for vision control
    std::vector<FMapWatchtower> Watchtowers;

    // Destructible rock positions that block paths between bases
    std::vector<FMapDestructibleRock> DestructibleRocks;

    // Precomputed ground pathing distances between all base pairs
    std::vector<FMapGroundDistanceEntry> GroundDistances;

    // Per-spawn layout configurations (one per starting location)
    std::vector<FMapSpawnLayout> Spawns;
};

}  // namespace sc2
