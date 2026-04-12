#include "common/descriptors/FEnemyObservationDescriptor.h"

#include <string>

#include "common/logging.h"

namespace sc2
{
namespace
{

bool IsEnemyWorkerUnitType(const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_SCV:
        case UNIT_TYPEID::PROTOSS_PROBE:
        case UNIT_TYPEID::ZERG_DRONE:
        case UNIT_TYPEID::ZERG_DRONEBURROWED:
            return true;
        default:
            return false;
    }
}

float EstimateUnitSupplyCost(const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::ZERG_ZERGLING:
            return 0.5f;
        case UNIT_TYPEID::TERRAN_MARINE:
        case UNIT_TYPEID::TERRAN_REAPER:
        case UNIT_TYPEID::PROTOSS_ZEALOT:
        case UNIT_TYPEID::ZERG_BANELING:
        case UNIT_TYPEID::ZERG_ROACH:
            return 1.0f;
        case UNIT_TYPEID::TERRAN_MARAUDER:
        case UNIT_TYPEID::PROTOSS_STALKER:
        case UNIT_TYPEID::PROTOSS_ADEPT:
        case UNIT_TYPEID::PROTOSS_SENTRY:
        case UNIT_TYPEID::ZERG_HYDRALISK:
        case UNIT_TYPEID::ZERG_MUTALISK:
        case UNIT_TYPEID::TERRAN_HELLION:
        case UNIT_TYPEID::TERRAN_HELLIONTANK:
        case UNIT_TYPEID::TERRAN_WIDOWMINE:
        case UNIT_TYPEID::TERRAN_CYCLONE:
            return 2.0f;
        case UNIT_TYPEID::TERRAN_SIEGETANK:
        case UNIT_TYPEID::TERRAN_SIEGETANKSIEGED:
        case UNIT_TYPEID::TERRAN_MEDIVAC:
        case UNIT_TYPEID::TERRAN_LIBERATOR:
        case UNIT_TYPEID::TERRAN_VIKINGFIGHTER:
        case UNIT_TYPEID::TERRAN_VIKINGASSAULT:
        case UNIT_TYPEID::PROTOSS_IMMORTAL:
        case UNIT_TYPEID::PROTOSS_DISRUPTOR:
        case UNIT_TYPEID::PROTOSS_PHOENIX:
        case UNIT_TYPEID::PROTOSS_ORACLE:
        case UNIT_TYPEID::ZERG_INFESTOR:
        case UNIT_TYPEID::ZERG_SWARMHOSTMP:
        case UNIT_TYPEID::ZERG_RAVAGER:
        case UNIT_TYPEID::ZERG_LURKERMP:
            return 3.0f;
        case UNIT_TYPEID::TERRAN_THOR:
        case UNIT_TYPEID::PROTOSS_COLOSSUS:
        case UNIT_TYPEID::PROTOSS_ARCHON:
        case UNIT_TYPEID::PROTOSS_VOIDRAY:
        case UNIT_TYPEID::ZERG_CORRUPTOR:
        case UNIT_TYPEID::ZERG_VIPER:
            return 4.0f;
        case UNIT_TYPEID::TERRAN_BATTLECRUISER:
        case UNIT_TYPEID::PROTOSS_CARRIER:
        case UNIT_TYPEID::PROTOSS_TEMPEST:
        case UNIT_TYPEID::ZERG_BROODLORD:
        case UNIT_TYPEID::ZERG_ULTRALISK:
            return 6.0f;
        default:
            return 2.0f;
    }
}

}  // namespace

void FEnemyCompositionSummary::Reset()
{
    TotalUnitCount = 0U;
    GroundUnitCount = 0U;
    AirUnitCount = 0U;
    StructureCount = 0U;
    WorkerCount = 0U;
    CombatUnitCount = 0U;
    EstimatedArmySupply = 0.0f;
    ArmyCentroid = Point2D(0.0f, 0.0f);
    HasArmyCentroid = false;
}

void FEnemyObservationDescriptor::Reset()
{
    UnitTags.clear();
    UnitTypeIds.clear();
    LastSeenPositions.clear();
    LastSeenHealth.clear();
    LastSeenHealthMax.clear();
    LastSeenShield.clear();
    LastSeenShieldMax.clear();
    IsFlying.clear();
    IsStructure.clear();
    FirstSeenGameLoops.clear();
    LastSeenGameLoops.clear();
    TagToIndexMap.clear();
    CompositionSummary.Reset();
    LastFullObservationGameLoop = 0U;
    CurrentGameLoop = 0U;
}

bool FEnemyObservationDescriptor::HasSynchronizedSizes() const
{
    const size_t ExpectedSizeValue = UnitTags.size();
    return UnitTypeIds.size() == ExpectedSizeValue && LastSeenPositions.size() == ExpectedSizeValue &&
           LastSeenHealth.size() == ExpectedSizeValue && LastSeenHealthMax.size() == ExpectedSizeValue &&
           LastSeenShield.size() == ExpectedSizeValue && LastSeenShieldMax.size() == ExpectedSizeValue &&
           IsFlying.size() == ExpectedSizeValue && IsStructure.size() == ExpectedSizeValue &&
           FirstSeenGameLoops.size() == ExpectedSizeValue && LastSeenGameLoops.size() == ExpectedSizeValue &&
           TagToIndexMap.size() == ExpectedSizeValue;
}

size_t FEnemyObservationDescriptor::GetObservedUnitCount() const
{
    return UnitTags.size();
}

void FEnemyObservationDescriptor::AddOrUpdateUnit(const Unit& EnemyUnitValue, const uint64_t CurrentGameLoopValue)
{
    const std::unordered_map<Tag, size_t>::const_iterator FoundIterator = TagToIndexMap.find(EnemyUnitValue.tag);
    if (FoundIterator != TagToIndexMap.end())
    {
        const size_t ExistingIndexValue = FoundIterator->second;
        UnitTypeIds[ExistingIndexValue] = EnemyUnitValue.unit_type.ToType();
        LastSeenPositions[ExistingIndexValue] = Point2D(EnemyUnitValue.pos);
        LastSeenHealth[ExistingIndexValue] = EnemyUnitValue.health;
        LastSeenHealthMax[ExistingIndexValue] = EnemyUnitValue.health_max;
        LastSeenShield[ExistingIndexValue] = EnemyUnitValue.shield;
        LastSeenShieldMax[ExistingIndexValue] = EnemyUnitValue.shield_max;
        IsFlying[ExistingIndexValue] = EnemyUnitValue.is_flying ? 1U : 0U;
        IsStructure[ExistingIndexValue] = EnemyUnitValue.is_building ? 1U : 0U;
        LastSeenGameLoops[ExistingIndexValue] = CurrentGameLoopValue;
        return;
    }

    const size_t NewIndexValue = UnitTags.size();
    UnitTags.push_back(EnemyUnitValue.tag);
    UnitTypeIds.push_back(EnemyUnitValue.unit_type.ToType());
    LastSeenPositions.push_back(Point2D(EnemyUnitValue.pos));
    LastSeenHealth.push_back(EnemyUnitValue.health);
    LastSeenHealthMax.push_back(EnemyUnitValue.health_max);
    LastSeenShield.push_back(EnemyUnitValue.shield);
    LastSeenShieldMax.push_back(EnemyUnitValue.shield_max);
    IsFlying.push_back(EnemyUnitValue.is_flying ? 1U : 0U);
    IsStructure.push_back(EnemyUnitValue.is_building ? 1U : 0U);
    FirstSeenGameLoops.push_back(CurrentGameLoopValue);
    LastSeenGameLoops.push_back(CurrentGameLoopValue);
    TagToIndexMap[EnemyUnitValue.tag] = NewIndexValue;
}

void FEnemyObservationDescriptor::RemoveAtIndex(const size_t IndexValue)
{
    const size_t LastIndexValue = UnitTags.size() - 1U;

    if (IndexValue != LastIndexValue)
    {
        TagToIndexMap[UnitTags[LastIndexValue]] = IndexValue;
        UnitTags[IndexValue] = UnitTags[LastIndexValue];
        UnitTypeIds[IndexValue] = UnitTypeIds[LastIndexValue];
        LastSeenPositions[IndexValue] = LastSeenPositions[LastIndexValue];
        LastSeenHealth[IndexValue] = LastSeenHealth[LastIndexValue];
        LastSeenHealthMax[IndexValue] = LastSeenHealthMax[LastIndexValue];
        LastSeenShield[IndexValue] = LastSeenShield[LastIndexValue];
        LastSeenShieldMax[IndexValue] = LastSeenShieldMax[LastIndexValue];
        IsFlying[IndexValue] = IsFlying[LastIndexValue];
        IsStructure[IndexValue] = IsStructure[LastIndexValue];
        FirstSeenGameLoops[IndexValue] = FirstSeenGameLoops[LastIndexValue];
        LastSeenGameLoops[IndexValue] = LastSeenGameLoops[LastIndexValue];
    }

    TagToIndexMap.erase(UnitTags[LastIndexValue]);
    UnitTags.pop_back();
    UnitTypeIds.pop_back();
    LastSeenPositions.pop_back();
    LastSeenHealth.pop_back();
    LastSeenHealthMax.pop_back();
    LastSeenShield.pop_back();
    LastSeenShieldMax.pop_back();
    IsFlying.pop_back();
    IsStructure.pop_back();
    FirstSeenGameLoops.pop_back();
    LastSeenGameLoops.pop_back();
}

void FEnemyObservationDescriptor::PruneStaleEntries(const uint64_t CurrentGameLoopValue,
                                                     const uint64_t MaxStaleGameLoopsValue)
{
    size_t EntryIndexValue = 0U;
    while (EntryIndexValue < UnitTags.size())
    {
        if (LastSeenGameLoops[EntryIndexValue] + MaxStaleGameLoopsValue < CurrentGameLoopValue)
        {
            RemoveAtIndex(EntryIndexValue);
        }
        else
        {
            ++EntryIndexValue;
        }
    }

    AssertSynchronizedSizes();
}

void FEnemyObservationDescriptor::RebuildCompositionSummary()
{
    CompositionSummary.Reset();

    float CentroidSumX = 0.0f;
    float CentroidSumY = 0.0f;
    uint32_t CombatPositionCountValue = 0U;

    const size_t UnitCountValue = UnitTags.size();
    for (size_t UnitIndexValue = 0U; UnitIndexValue < UnitCountValue; ++UnitIndexValue)
    {
        ++CompositionSummary.TotalUnitCount;

        const bool IsStructureValue = IsStructure[UnitIndexValue] != 0U;
        const bool IsFlyingValue = IsFlying[UnitIndexValue] != 0U;
        const UNIT_TYPEID UnitTypeIdValue = UnitTypeIds[UnitIndexValue];

        if (IsStructureValue)
        {
            ++CompositionSummary.StructureCount;
            continue;
        }

        if (IsEnemyWorkerUnitType(UnitTypeIdValue))
        {
            ++CompositionSummary.WorkerCount;
            continue;
        }

        ++CompositionSummary.CombatUnitCount;
        CompositionSummary.EstimatedArmySupply += EstimateUnitSupplyCost(UnitTypeIdValue);

        if (IsFlyingValue)
        {
            ++CompositionSummary.AirUnitCount;
        }
        else
        {
            ++CompositionSummary.GroundUnitCount;
        }

        CentroidSumX += LastSeenPositions[UnitIndexValue].x;
        CentroidSumY += LastSeenPositions[UnitIndexValue].y;
        ++CombatPositionCountValue;
    }

    if (CombatPositionCountValue > 0U)
    {
        CompositionSummary.ArmyCentroid = Point2D(CentroidSumX / static_cast<float>(CombatPositionCountValue),
                                                   CentroidSumY / static_cast<float>(CombatPositionCountValue));
        CompositionSummary.HasArmyCentroid = true;
    }
}

void FEnemyObservationDescriptor::AssertSynchronizedSizes() const
{
    if (!HasSynchronizedSizes())
    {
        SCLOG(LoggingVerbosity::error,
              "INVARIANT VIOLATION: FEnemyObservationDescriptor vector sizes desynchronized at UnitCount=" +
                  std::to_string(UnitTags.size()));
    }
}

}  // namespace sc2
