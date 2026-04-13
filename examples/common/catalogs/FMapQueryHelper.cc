#include "common/catalogs/FMapQueryHelper.h"

#include <limits>

#include "common/catalogs/FMapLayoutDictionary.h"

namespace sc2
{

EMapId FMapQueryHelper::GetMapId(const FMapDescriptor& MapDescriptorValue)
{
    return MapDescriptorValue.MapId;
}

Point2D FMapQueryHelper::GetMapCenter(const FMapDescriptor& MapDescriptorValue)
{
    return MapDescriptorValue.MapCenter;
}

uint8_t FMapQueryHelper::GetBaseCount(const FMapDescriptor& MapDescriptorValue)
{
    return static_cast<uint8_t>(MapDescriptorValue.Bases.size());
}

const FMapBaseDescriptor* FMapQueryHelper::GetBaseByIndex(
    const FMapDescriptor& MapDescriptorValue,
    const uint8_t BaseIndexValue)
{
    for (const FMapBaseDescriptor& BaseDescriptorValue : MapDescriptorValue.Bases)
    {
        if (BaseDescriptorValue.BaseIndex == BaseIndexValue)
        {
            return &BaseDescriptorValue;
        }
    }

    return nullptr;
}

uint8_t FMapQueryHelper::GetNearestBaseIndex(
    const FMapDescriptor& MapDescriptorValue,
    const Point2D& PositionValue)
{
    uint8_t NearestBaseIndexValue = 0U;
    float BestDistanceSquaredValue = std::numeric_limits<float>::max();

    for (const FMapBaseDescriptor& BaseDescriptorValue : MapDescriptorValue.Bases)
    {
        const float DeltaXValue = BaseDescriptorValue.Location.x - PositionValue.x;
        const float DeltaYValue = BaseDescriptorValue.Location.y - PositionValue.y;
        const float DistanceSquaredValue = (DeltaXValue * DeltaXValue) + (DeltaYValue * DeltaYValue);

        if (DistanceSquaredValue < BestDistanceSquaredValue)
        {
            BestDistanceSquaredValue = DistanceSquaredValue;
            NearestBaseIndexValue = BaseDescriptorValue.BaseIndex;
        }
    }

    return NearestBaseIndexValue;
}

uint8_t FMapQueryHelper::GetNextExpansionBaseIndex(
    const FMapSpawnLayout& SpawnLayoutValue,
    const uint8_t CurrentExpansionCountValue)
{
    if (CurrentExpansionCountValue >= SpawnLayoutValue.ExpansionOrderCount)
    {
        return 0xFFU;
    }

    return SpawnLayoutValue.ExpansionOrder[CurrentExpansionCountValue];
}

uint8_t FMapQueryHelper::GetEnemyNextExpansionBaseIndex(
    const FMapDescriptor& MapDescriptorValue,
    const FMapSpawnLayout& OwnSpawnLayoutValue,
    const uint8_t EnemyExpansionCountValue)
{
    const FMapSpawnLayout* EnemySpawnPtrValue =
        FMapLayoutDictionary::TryGetEnemySpawnLayout(MapDescriptorValue, OwnSpawnLayoutValue);
    if (EnemySpawnPtrValue == nullptr)
    {
        return 0xFFU;
    }

    return GetNextExpansionBaseIndex(*EnemySpawnPtrValue, EnemyExpansionCountValue);
}

float FMapQueryHelper::GetGroundDistance(
    const FMapDescriptor& MapDescriptorValue,
    const uint8_t FromBaseIndexValue,
    const uint8_t ToBaseIndexValue)
{
    if (FromBaseIndexValue == ToBaseIndexValue)
    {
        return 0.0f;
    }

    for (const FMapGroundDistanceEntry& DistanceEntryValue : MapDescriptorValue.GroundDistances)
    {
        if ((DistanceEntryValue.FromBaseIndex == FromBaseIndexValue &&
             DistanceEntryValue.ToBaseIndex == ToBaseIndexValue) ||
            (DistanceEntryValue.FromBaseIndex == ToBaseIndexValue &&
             DistanceEntryValue.ToBaseIndex == FromBaseIndexValue))
        {
            return DistanceEntryValue.Distance;
        }
    }

    return std::numeric_limits<float>::max();
}

float FMapQueryHelper::GetDistanceToEnemyMain(
    const FMapDescriptor& MapDescriptorValue,
    const FMapSpawnLayout& OwnSpawnLayoutValue,
    const uint8_t FromBaseIndexValue)
{
    const FMapSpawnLayout* EnemySpawnPtrValue =
        FMapLayoutDictionary::TryGetEnemySpawnLayout(MapDescriptorValue, OwnSpawnLayoutValue);
    if (EnemySpawnPtrValue == nullptr)
    {
        return std::numeric_limits<float>::max();
    }

    return GetGroundDistance(MapDescriptorValue, FromBaseIndexValue, EnemySpawnPtrValue->MainBaseIndex);
}

const FMapRampDescriptor* FMapQueryHelper::GetRampByIndex(
    const FMapDescriptor& MapDescriptorValue,
    const uint8_t RampIndexValue)
{
    for (const FMapRampDescriptor& RampDescriptorValue : MapDescriptorValue.Ramps)
    {
        if (RampDescriptorValue.RampIndex == RampIndexValue)
        {
            return &RampDescriptorValue;
        }
    }

    return nullptr;
}

const FMapRampDescriptor* FMapQueryHelper::GetMainRamp(
    const FMapDescriptor& MapDescriptorValue,
    const FMapSpawnLayout& SpawnLayoutValue)
{
    return GetRampByIndex(MapDescriptorValue, SpawnLayoutValue.MainRampIndex);
}

const FMapRampDescriptor* FMapQueryHelper::GetRampBetweenBases(
    const FMapDescriptor& MapDescriptorValue,
    const uint8_t BaseIndexAValue,
    const uint8_t BaseIndexBValue)
{
    for (const FMapRampDescriptor& RampDescriptorValue : MapDescriptorValue.Ramps)
    {
        if ((RampDescriptorValue.ConnectedBaseIndexA == BaseIndexAValue &&
             RampDescriptorValue.ConnectedBaseIndexB == BaseIndexBValue) ||
            (RampDescriptorValue.ConnectedBaseIndexA == BaseIndexBValue &&
             RampDescriptorValue.ConnectedBaseIndexB == BaseIndexAValue))
        {
            return &RampDescriptorValue;
        }
    }

    return nullptr;
}

const FMapRampWallLayout& FMapQueryHelper::GetMainRampWall(
    const FMapSpawnLayout& SpawnLayoutValue)
{
    return SpawnLayoutValue.RampWall;
}

const FMapProductionColumnLayout& FMapQueryHelper::GetProductionColumn(
    const FMapSpawnLayout& SpawnLayoutValue)
{
    return SpawnLayoutValue.ProductionColumn;
}

const FMapNaturalWallLayout& FMapQueryHelper::GetNaturalWall(
    const FMapSpawnLayout& SpawnLayoutValue)
{
    return SpawnLayoutValue.NaturalWall;
}

Point2D FMapQueryHelper::GetNearestWatchtowerPosition(
    const FMapDescriptor& MapDescriptorValue,
    const Point2D& PositionValue)
{
    if (MapDescriptorValue.Watchtowers.empty())
    {
        return Point2D(0.0f, 0.0f);
    }

    Point2D NearestPositionValue = MapDescriptorValue.Watchtowers[0].Position;
    float BestDistanceSquaredValue = std::numeric_limits<float>::max();

    for (const FMapWatchtower& WatchtowerValue : MapDescriptorValue.Watchtowers)
    {
        const float DeltaXValue = WatchtowerValue.Position.x - PositionValue.x;
        const float DeltaYValue = WatchtowerValue.Position.y - PositionValue.y;
        const float DistanceSquaredValue = (DeltaXValue * DeltaXValue) + (DeltaYValue * DeltaYValue);

        if (DistanceSquaredValue < BestDistanceSquaredValue)
        {
            BestDistanceSquaredValue = DistanceSquaredValue;
            NearestPositionValue = WatchtowerValue.Position;
        }
    }

    return NearestPositionValue;
}

bool FMapQueryHelper::IsInOwnTerritory(
    const FMapDescriptor& MapDescriptorValue,
    const FMapSpawnLayout& OwnSpawnLayoutValue,
    const Point2D& PositionValue)
{
    const FMapBaseDescriptor* OwnMainPtrValue =
        GetBaseByIndex(MapDescriptorValue, OwnSpawnLayoutValue.MainBaseIndex);
    const FMapSpawnLayout* EnemySpawnPtrValue =
        FMapLayoutDictionary::TryGetEnemySpawnLayout(MapDescriptorValue, OwnSpawnLayoutValue);

    if (OwnMainPtrValue == nullptr || EnemySpawnPtrValue == nullptr)
    {
        return true;
    }

    const FMapBaseDescriptor* EnemyMainPtrValue =
        GetBaseByIndex(MapDescriptorValue, EnemySpawnPtrValue->MainBaseIndex);
    if (EnemyMainPtrValue == nullptr)
    {
        return true;
    }

    const float OwnDeltaXValue = OwnMainPtrValue->Location.x - PositionValue.x;
    const float OwnDeltaYValue = OwnMainPtrValue->Location.y - PositionValue.y;
    const float OwnDistanceSquaredValue = (OwnDeltaXValue * OwnDeltaXValue) + (OwnDeltaYValue * OwnDeltaYValue);

    const float EnemyDeltaXValue = EnemyMainPtrValue->Location.x - PositionValue.x;
    const float EnemyDeltaYValue = EnemyMainPtrValue->Location.y - PositionValue.y;
    const float EnemyDistanceSquaredValue = (EnemyDeltaXValue * EnemyDeltaXValue) + (EnemyDeltaYValue * EnemyDeltaYValue);

    return OwnDistanceSquaredValue <= EnemyDistanceSquaredValue;
}

}  // namespace sc2
