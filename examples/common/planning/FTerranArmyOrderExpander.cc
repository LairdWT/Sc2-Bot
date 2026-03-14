#include "common/planning/FTerranArmyOrderExpander.h"

#include <algorithm>
#include <limits>
#include <vector>

#include "common/armies/EArmyMissionType.h"
#include "common/armies/FArmyMissionDescriptor.h"
#include "common/bot_status_models.h"
#include "sc2api/sc2_interfaces.h"
#include "sc2api/sc2_unit_filters.h"

namespace sc2
{
namespace
{

constexpr float BaseThreatDistanceSquaredValue = 625.0f;
constexpr float OutsideFriendlyTerritoryDistanceSquaredValue = 900.0f;
constexpr float SweepObjectiveRadiusValue = 10.0f;
constexpr float PressureObjectiveRadiusValue = 12.0f;

bool IsWorkerUnitType(const UNIT_TYPEID UnitTypeIdValue)
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

bool IsTerranCombatUnitType(const UNIT_TYPEID UnitTypeIdValue)
{
    switch (UnitTypeIdValue)
    {
        case UNIT_TYPEID::TERRAN_MARINE:
        case UNIT_TYPEID::TERRAN_MARAUDER:
        case UNIT_TYPEID::TERRAN_HELLION:
        case UNIT_TYPEID::TERRAN_HELLIONTANK:
        case UNIT_TYPEID::TERRAN_CYCLONE:
        case UNIT_TYPEID::TERRAN_SIEGETANK:
        case UNIT_TYPEID::TERRAN_SIEGETANKSIEGED:
        case UNIT_TYPEID::TERRAN_WIDOWMINE:
        case UNIT_TYPEID::TERRAN_WIDOWMINEBURROWED:
        case UNIT_TYPEID::TERRAN_MEDIVAC:
        case UNIT_TYPEID::TERRAN_LIBERATOR:
        case UNIT_TYPEID::TERRAN_LIBERATORAG:
        case UNIT_TYPEID::TERRAN_VIKINGFIGHTER:
        case UNIT_TYPEID::TERRAN_VIKINGASSAULT:
            return true;
        default:
            return false;
    }
}

bool IsEnemyCombatThreat(const Unit& EnemyUnitValue)
{
    return EnemyUnitValue.build_progress >= 1.0f && !EnemyUnitValue.is_building &&
           !IsWorkerUnitType(EnemyUnitValue.unit_type.ToType());
}

Point2D ComputeArmyCenterPoint(const FAgentState& AgentStateValue, const Point2D& RallyPointValue)
{
    float SumXValue = 0.0f;
    float SumYValue = 0.0f;
    uint32_t UnitCountValue = 0U;

    for (const Unit* ControlledUnitPtrValue : AgentStateValue.UnitContainer.ControlledUnits)
    {
        if (ControlledUnitPtrValue == nullptr || ControlledUnitPtrValue->build_progress < 1.0f ||
            !IsTerranCombatUnitType(ControlledUnitPtrValue->unit_type.ToType()))
        {
            continue;
        }

        SumXValue += ControlledUnitPtrValue->pos.x;
        SumYValue += ControlledUnitPtrValue->pos.y;
        ++UnitCountValue;
    }

    if (UnitCountValue == 0U)
    {
        return RallyPointValue;
    }

    return Point2D(SumXValue / static_cast<float>(UnitCountValue), SumYValue / static_cast<float>(UnitCountValue));
}

const Unit* FindNearestUnitToPoint(const Point2D& OriginPointValue, const Units& CandidateUnitsValue)
{
    const Unit* BestUnitPtrValue = nullptr;
    float BestDistanceSquaredValue = std::numeric_limits<float>::max();

    for (const Unit* CandidateUnitPtrValue : CandidateUnitsValue)
    {
        if (CandidateUnitPtrValue == nullptr)
        {
            continue;
        }

        const float DistanceSquaredValue = DistanceSquared2D(OriginPointValue, Point2D(CandidateUnitPtrValue->pos));
        if (DistanceSquaredValue >= BestDistanceSquaredValue)
        {
            continue;
        }

        BestDistanceSquaredValue = DistanceSquaredValue;
        BestUnitPtrValue = CandidateUnitPtrValue;
    }

    return BestUnitPtrValue;
}

bool TryFindThreatenedBase(const Units& SelfTownHallUnitsValue, const Units& EnemyUnitsValue,
                           Point2D& OutObjectivePointValue)
{
    for (const Unit* SelfTownHallUnitPtrValue : SelfTownHallUnitsValue)
    {
        if (SelfTownHallUnitPtrValue == nullptr || SelfTownHallUnitPtrValue->build_progress < 1.0f)
        {
            continue;
        }

        for (const Unit* EnemyUnitPtrValue : EnemyUnitsValue)
        {
            if (EnemyUnitPtrValue == nullptr || !IsEnemyCombatThreat(*EnemyUnitPtrValue))
            {
                continue;
            }

            if (DistanceSquared2D(Point2D(SelfTownHallUnitPtrValue->pos), Point2D(EnemyUnitPtrValue->pos)) <=
                BaseThreatDistanceSquaredValue)
            {
                OutObjectivePointValue = Point2D(SelfTownHallUnitPtrValue->pos);
                return true;
            }
        }
    }

    return false;
}

std::vector<Point2D> BuildSweepTargets(const ObservationInterface& ObservationValue,
                                       const std::vector<Point2D>& ExpansionLocationsValue,
                                       const Units& SelfTownHallUnitsValue)
{
    std::vector<Point2D> SweepTargetsValue;
    SweepTargetsValue.reserve(ExpansionLocationsValue.size() + 1U);

    const Point2D EnemyStartLocationValue = !ObservationValue.GetGameInfo().enemy_start_locations.empty()
                                                ? ObservationValue.GetGameInfo().enemy_start_locations.front()
                                                : Point2D();
    SweepTargetsValue.push_back(EnemyStartLocationValue);

    std::vector<Point2D> SortedExpansionLocationsValue = ExpansionLocationsValue;
    std::stable_sort(SortedExpansionLocationsValue.begin(), SortedExpansionLocationsValue.end(),
                     [&EnemyStartLocationValue](const Point2D& LeftPointValue, const Point2D& RightPointValue)
                     {
                         return DistanceSquared2D(LeftPointValue, EnemyStartLocationValue) <
                                DistanceSquared2D(RightPointValue, EnemyStartLocationValue);
                     });

    for (const Point2D& ExpansionLocationValue : SortedExpansionLocationsValue)
    {
        bool IsOwnedExpansionValue = false;
        for (const Unit* SelfTownHallUnitPtrValue : SelfTownHallUnitsValue)
        {
            if (SelfTownHallUnitPtrValue != nullptr &&
                DistanceSquared2D(Point2D(SelfTownHallUnitPtrValue->pos), ExpansionLocationValue) <= 64.0f)
            {
                IsOwnedExpansionValue = true;
                break;
            }
        }

        if (!IsOwnedExpansionValue)
        {
            SweepTargetsValue.push_back(ExpansionLocationValue);
        }
    }

    return SweepTargetsValue;
}

bool DoesObjectiveContainVisibleEnemyStructure(const Point2D& ObjectivePointValue, const Units& EnemyUnitsValue,
                                               const float ObjectiveRadiusValue)
{
    const float ObjectiveRadiusSquaredValue = ObjectiveRadiusValue * ObjectiveRadiusValue;
    for (const Unit* EnemyUnitPtrValue : EnemyUnitsValue)
    {
        if (EnemyUnitPtrValue == nullptr || !EnemyUnitPtrValue->is_building)
        {
            continue;
        }

        if (DistanceSquared2D(Point2D(EnemyUnitPtrValue->pos), ObjectivePointValue) <= ObjectiveRadiusSquaredValue)
        {
            return true;
        }
    }

    return false;
}

uint32_t ClampSearchOrdinal(const uint32_t SearchExpansionOrdinalValue, const size_t TargetCountValue)
{
    if (TargetCountValue == 0U)
    {
        return 0U;
    }

    return std::min<uint32_t>(SearchExpansionOrdinalValue, static_cast<uint32_t>(TargetCountValue - 1U));
}

void ApplyMissionDescriptor(FArmyMissionDescriptor& MissionDescriptorValue, const EArmyMissionType MissionTypeValue,
                            const uint32_t SourceGoalIdValue, const Point2D& ObjectivePointValue,
                            const float ObjectiveRadiusValue, const uint32_t SearchExpansionOrdinalValue)
{
    MissionDescriptorValue.MissionType = MissionTypeValue;
    MissionDescriptorValue.SourceGoalId = SourceGoalIdValue;
    MissionDescriptorValue.ObjectivePoint = ObjectivePointValue;
    MissionDescriptorValue.ObjectiveRadius = ObjectiveRadiusValue;
    MissionDescriptorValue.SearchExpansionOrdinal = SearchExpansionOrdinalValue;
}

}  // namespace

void FTerranArmyOrderExpander::ExpandArmyOrders(
    const FFrameContext& FrameValue, const FAgentState& AgentStateValue, FGameStateDescriptor& GameStateDescriptorValue,
    const std::vector<Point2D>& ExpansionLocationsValue, const Point2D& RallyPointValue,
    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue) const
{
    if (FrameValue.Observation == nullptr)
    {
        return;
    }

    GameStateDescriptorValue.ArmyState.EnsurePrimaryArmyExists();
    FArmyMissionDescriptor& PrimaryMissionDescriptorValue = GameStateDescriptorValue.ArmyState.ArmyMissions.front();
    uint32_t SourceGoalIdValue = PrimaryMissionDescriptorValue.SourceGoalId;
    for (const size_t ArmyOrderIndexValue : CommandAuthoritySchedulingStateValue.ArmyOrderIndices)
    {
        const FCommandOrderRecord ArmyOrderValue = CommandAuthoritySchedulingStateValue.GetOrderRecord(ArmyOrderIndexValue);
        if (ArmyOrderValue.SourceLayer != ECommandAuthorityLayer::Army ||
            IsTerminalLifecycleState(ArmyOrderValue.LifecycleState) || ArmyOrderValue.OwningArmyIndex != 0)
        {
            continue;
        }

        SourceGoalIdValue = ArmyOrderValue.SourceGoalId;
        break;
    }

    const Units SelfTownHallUnitsValue = FrameValue.Observation->GetUnits(Unit::Alliance::Self, IsTownHall());
    const Units EnemyUnitsValue = FrameValue.Observation->GetUnits(Unit::Alliance::Enemy);
    const Point2D ArmyCenterPointValue = ComputeArmyCenterPoint(AgentStateValue, RallyPointValue);

    Point2D ThreatenedBasePointValue;
    if (TryFindThreatenedBase(SelfTownHallUnitsValue, EnemyUnitsValue, ThreatenedBasePointValue))
    {
        ApplyMissionDescriptor(PrimaryMissionDescriptorValue, EArmyMissionType::DefendOwnedBase,
                               SourceGoalIdValue, ThreatenedBasePointValue,
                               PressureObjectiveRadiusValue, PrimaryMissionDescriptorValue.SearchExpansionOrdinal);
        return;
    }

    const float DistanceToRallySquaredValue = DistanceSquared2D(ArmyCenterPointValue, RallyPointValue);
    if (GameStateDescriptorValue.MacroState.ArmySupply <
            GameStateDescriptorValue.ArmyState.PrimaryArmyDisengageSupplyThreshold &&
        DistanceToRallySquaredValue > OutsideFriendlyTerritoryDistanceSquaredValue)
    {
        ApplyMissionDescriptor(PrimaryMissionDescriptorValue, EArmyMissionType::Regroup,
                               SourceGoalIdValue, RallyPointValue, PressureObjectiveRadiusValue,
                               PrimaryMissionDescriptorValue.SearchExpansionOrdinal);
        return;
    }

    if (GameStateDescriptorValue.MacroState.ArmySupply <
        GameStateDescriptorValue.ArmyState.PrimaryArmyAttackSupplyThreshold)
    {
        ApplyMissionDescriptor(PrimaryMissionDescriptorValue, EArmyMissionType::AssembleAtRally,
                               SourceGoalIdValue, RallyPointValue, PressureObjectiveRadiusValue,
                               PrimaryMissionDescriptorValue.SearchExpansionOrdinal);
        return;
    }

    Units VisibleEnemyTownHallUnitsValue;
    Units VisibleEnemyStructureUnitsValue;
    VisibleEnemyTownHallUnitsValue.reserve(EnemyUnitsValue.size());
    VisibleEnemyStructureUnitsValue.reserve(EnemyUnitsValue.size());
    const IsTownHall TownHallFilterValue;
    for (const Unit* EnemyUnitPtrValue : EnemyUnitsValue)
    {
        if (EnemyUnitPtrValue == nullptr || EnemyUnitPtrValue->build_progress < 1.0f)
        {
            continue;
        }

        if (EnemyUnitPtrValue->is_building)
        {
            VisibleEnemyStructureUnitsValue.push_back(EnemyUnitPtrValue);
        }

        if (TownHallFilterValue(*EnemyUnitPtrValue))
        {
            VisibleEnemyTownHallUnitsValue.push_back(EnemyUnitPtrValue);
        }
    }

    const Unit* EnemyTownHallUnitPtrValue = FindNearestUnitToPoint(ArmyCenterPointValue, VisibleEnemyTownHallUnitsValue);
    if (EnemyTownHallUnitPtrValue != nullptr)
    {
        ApplyMissionDescriptor(PrimaryMissionDescriptorValue, EArmyMissionType::PressureKnownEnemyBase,
                               SourceGoalIdValue, Point2D(EnemyTownHallUnitPtrValue->pos),
                               PressureObjectiveRadiusValue, PrimaryMissionDescriptorValue.SearchExpansionOrdinal);
        return;
    }

    const Unit* EnemyStructureUnitPtrValue =
        FindNearestUnitToPoint(ArmyCenterPointValue, VisibleEnemyStructureUnitsValue);
    if (EnemyStructureUnitPtrValue != nullptr)
    {
        ApplyMissionDescriptor(PrimaryMissionDescriptorValue, EArmyMissionType::ClearKnownEnemyStructures,
                               SourceGoalIdValue, Point2D(EnemyStructureUnitPtrValue->pos),
                               PressureObjectiveRadiusValue, PrimaryMissionDescriptorValue.SearchExpansionOrdinal);
        return;
    }

    const std::vector<Point2D> SweepTargetsValue =
        BuildSweepTargets(*FrameValue.Observation, ExpansionLocationsValue, SelfTownHallUnitsValue);
    if (SweepTargetsValue.empty())
    {
        ApplyMissionDescriptor(PrimaryMissionDescriptorValue, EArmyMissionType::AssembleAtRally,
                               SourceGoalIdValue, RallyPointValue, PressureObjectiveRadiusValue,
                               0U);
        return;
    }

    uint32_t SearchExpansionOrdinalValue =
        ClampSearchOrdinal(PrimaryMissionDescriptorValue.SearchExpansionOrdinal, SweepTargetsValue.size());
    Point2D SweepObjectivePointValue = SweepTargetsValue[SearchExpansionOrdinalValue];
    while (FrameValue.Observation->GetVisibility(SweepObjectivePointValue) == Visibility::Visible &&
           !DoesObjectiveContainVisibleEnemyStructure(SweepObjectivePointValue, EnemyUnitsValue, SweepObjectiveRadiusValue) &&
           (SearchExpansionOrdinalValue + 1U) < SweepTargetsValue.size())
    {
        ++SearchExpansionOrdinalValue;
        SweepObjectivePointValue = SweepTargetsValue[SearchExpansionOrdinalValue];
    }

    ApplyMissionDescriptor(PrimaryMissionDescriptorValue, EArmyMissionType::SweepExpansionLocations,
                           SourceGoalIdValue, SweepObjectivePointValue,
                           SweepObjectiveRadiusValue, SearchExpansionOrdinalValue);
}

}  // namespace sc2
