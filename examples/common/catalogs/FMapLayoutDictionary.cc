#include "common/catalogs/FMapLayoutDictionary.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>

#include "common/catalogs/generated/FMapLayoutDictionaryData.generated.h"

namespace sc2
{

const FMapDescriptor* FMapLayoutDictionary::TryGetMapByName(const std::string& NormalizedMapNameValue)
{
    for (const FMapDescriptor& MapDescriptorValue : GMapDescriptors)
    {
        for (const std::string& CandidateNameValue : MapDescriptorValue.NormalizedNames)
        {
            if (NormalizedMapNameValue.find(CandidateNameValue) != std::string::npos)
            {
                return &MapDescriptorValue;
            }
        }
    }

    return nullptr;
}

const FMapSpawnLayout* FMapLayoutDictionary::TryGetSpawnLayout(
    const FMapDescriptor& MapDescriptorValue,
    const Point2D& StartLocationValue)
{
    const FMapSpawnLayout* BestSpawnLayoutPtrValue = nullptr;
    float BestDistanceSquaredValue = std::numeric_limits<float>::max();

    for (const FMapSpawnLayout& SpawnLayoutValue : MapDescriptorValue.Spawns)
    {
        if (SpawnLayoutValue.MainBaseIndex >= MapDescriptorValue.Bases.size())
        {
            continue;
        }

        const Point2D& BaseLocationValue = MapDescriptorValue.Bases[SpawnLayoutValue.MainBaseIndex].Location;
        const float DeltaXValue = BaseLocationValue.x - StartLocationValue.x;
        const float DeltaYValue = BaseLocationValue.y - StartLocationValue.y;
        const float DistanceSquaredValue = (DeltaXValue * DeltaXValue) + (DeltaYValue * DeltaYValue);

        if (DistanceSquaredValue < BestDistanceSquaredValue)
        {
            BestDistanceSquaredValue = DistanceSquaredValue;
            BestSpawnLayoutPtrValue = &SpawnLayoutValue;
        }
    }

    return BestSpawnLayoutPtrValue;
}

const FMapSpawnLayout* FMapLayoutDictionary::TryGetEnemySpawnLayout(
    const FMapDescriptor& MapDescriptorValue,
    const FMapSpawnLayout& OwnSpawnLayoutValue)
{
    for (const FMapSpawnLayout& SpawnLayoutValue : MapDescriptorValue.Spawns)
    {
        if (SpawnLayoutValue.SpawnId == OwnSpawnLayoutValue.EnemySpawnId)
        {
            return &SpawnLayoutValue;
        }
    }

    return nullptr;
}

std::string FMapLayoutDictionary::NormalizeMapName(const std::string& MapNameValue)
{
    std::string NormalizedValue;
    NormalizedValue.reserve(MapNameValue.size());
    for (const char CharacterValue : MapNameValue)
    {
        if (std::isalnum(static_cast<unsigned char>(CharacterValue)))
        {
            NormalizedValue += static_cast<char>(std::tolower(static_cast<unsigned char>(CharacterValue)));
        }
    }
    return NormalizedValue;
}

}  // namespace sc2
