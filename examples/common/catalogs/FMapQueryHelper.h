#pragma once

#include <cstdint>

#include "common/catalogs/FMapLayoutTypes.h"
#include "sc2api/sc2_common.h"

namespace sc2
{

// Provides static query functions for map layout data.
// All functions operate on const references to FMapDescriptor and FMapSpawnLayout.
// Used as a read-only lookup throughout the bot after initialization.
struct FMapQueryHelper
{
public:
    // -- Map Identification --

    // Returns the map identifier for the loaded map descriptor
    static EMapId GetMapId(const FMapDescriptor& MapDescriptorValue);

    // Returns the center point of the playable map area
    static Point2D GetMapCenter(const FMapDescriptor& MapDescriptorValue);

    // -- Base Queries --

    // Returns the total number of base locations on the map
    static uint8_t GetBaseCount(const FMapDescriptor& MapDescriptorValue);

    // Returns the base descriptor for the given index, or nullptr if invalid
    static const FMapBaseDescriptor* GetBaseByIndex(
        const FMapDescriptor& MapDescriptorValue,
        uint8_t BaseIndexValue);

    // Returns the base index nearest to the given world position
    static uint8_t GetNearestBaseIndex(
        const FMapDescriptor& MapDescriptorValue,
        const Point2D& PositionValue);

    // Returns the next base index to expand to based on current expansion count
    static uint8_t GetNextExpansionBaseIndex(
        const FMapSpawnLayout& SpawnLayoutValue,
        uint8_t CurrentExpansionCountValue);

    // Returns the predicted enemy next expansion base index
    static uint8_t GetEnemyNextExpansionBaseIndex(
        const FMapDescriptor& MapDescriptorValue,
        const FMapSpawnLayout& OwnSpawnLayoutValue,
        uint8_t EnemyExpansionCountValue);

    // -- Distance Queries --

    // Returns the precomputed ground pathing distance between two bases
    static float GetGroundDistance(
        const FMapDescriptor& MapDescriptorValue,
        uint8_t FromBaseIndexValue,
        uint8_t ToBaseIndexValue);

    // Returns the ground distance from the given base to the enemy main
    static float GetDistanceToEnemyMain(
        const FMapDescriptor& MapDescriptorValue,
        const FMapSpawnLayout& OwnSpawnLayoutValue,
        uint8_t FromBaseIndexValue);

    // -- Ramp Queries --

    // Returns the ramp descriptor for the given index, or nullptr
    static const FMapRampDescriptor* GetRampByIndex(
        const FMapDescriptor& MapDescriptorValue,
        uint8_t RampIndexValue);

    // Returns the main base ramp descriptor for the given spawn
    static const FMapRampDescriptor* GetMainRamp(
        const FMapDescriptor& MapDescriptorValue,
        const FMapSpawnLayout& SpawnLayoutValue);

    // Returns the ramp connecting two bases, or nullptr if none exists
    static const FMapRampDescriptor* GetRampBetweenBases(
        const FMapDescriptor& MapDescriptorValue,
        uint8_t BaseIndexAValue,
        uint8_t BaseIndexBValue);

    // -- Placement Queries --

    // Returns the ramp wall layout for the given spawn
    static const FMapRampWallLayout& GetMainRampWall(
        const FMapSpawnLayout& SpawnLayoutValue);

    // Returns the production column layout for the given spawn
    static const FMapProductionColumnLayout& GetProductionColumn(
        const FMapSpawnLayout& SpawnLayoutValue);

    // Returns the natural entrance wall layout for the given spawn
    static const FMapNaturalWallLayout& GetNaturalWall(
        const FMapSpawnLayout& SpawnLayoutValue);

    // -- Landmark Queries --

    // Returns the position of the watchtower nearest to the given point,
    // or (0,0) if no watchtowers exist on the map
    static Point2D GetNearestWatchtowerPosition(
        const FMapDescriptor& MapDescriptorValue,
        const Point2D& PositionValue);

    // -- Territory Queries --

    // Returns true if the position is closer to own main than enemy main
    static bool IsInOwnTerritory(
        const FMapDescriptor& MapDescriptorValue,
        const FMapSpawnLayout& OwnSpawnLayoutValue,
        const Point2D& PositionValue);
};

}  // namespace sc2
