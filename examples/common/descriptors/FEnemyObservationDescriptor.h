#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "sc2api/sc2_common.h"
#include "sc2api/sc2_typeenums.h"
#include "sc2api/sc2_unit.h"

namespace sc2
{

struct FEnemyCompositionSummary
{
    uint32_t TotalUnitCount{0U};
    uint32_t GroundUnitCount{0U};
    uint32_t AirUnitCount{0U};
    uint32_t StructureCount{0U};
    uint32_t WorkerCount{0U};
    uint32_t CombatUnitCount{0U};
    float EstimatedArmySupply{0.0f};
    Point2D ArmyCentroid;
    bool HasArmyCentroid{false};

    void Reset();
};

struct FEnemyObservationDescriptor
{
public:
    static constexpr uint64_t DefaultStaleEntryThresholdGameLoopsValue = 672U;

    std::vector<Tag> UnitTags;
    std::vector<UNIT_TYPEID> UnitTypeIds;
    std::vector<Point2D> LastSeenPositions;
    std::vector<float> LastSeenHealth;
    std::vector<float> LastSeenHealthMax;
    std::vector<float> LastSeenShield;
    std::vector<float> LastSeenShieldMax;
    std::vector<uint8_t> IsFlying;
    std::vector<uint8_t> IsStructure;
    std::vector<uint64_t> FirstSeenGameLoops;
    std::vector<uint64_t> LastSeenGameLoops;

    FEnemyCompositionSummary CompositionSummary;
    uint64_t LastFullObservationGameLoop{0U};
    uint64_t CurrentGameLoop{0U};

    void Reset();
    bool HasSynchronizedSizes() const;
    size_t GetObservedUnitCount() const;
    void AddOrUpdateUnit(const Unit& EnemyUnitValue, uint64_t CurrentGameLoopValue);
    void RebuildCompositionSummary();
    void PruneStaleEntries(uint64_t CurrentGameLoopValue, uint64_t MaxStaleGameLoopsValue);

private:
    std::unordered_map<Tag, size_t> TagToIndexMap;
    void RemoveAtIndex(size_t IndexValue);
    void AssertSynchronizedSizes() const;
};

}  // namespace sc2
