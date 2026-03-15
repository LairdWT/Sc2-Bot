#include "test_terran_army_order_pipeline.h"

#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/agent_framework.h"
#include "common/bot_status_models.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/planning/ECommandAuthorityLayer.h"
#include "common/planning/ECommandTaskType.h"
#include "common/planning/EIntentDomain.h"
#include "common/planning/EOrderLifecycleState.h"
#include "common/planning/FCommandOrderRecord.h"
#include "common/planning/FTerranArmyOrderExpander.h"
#include "common/planning/FTerranArmyUnitExecutionPlanner.h"
#include "common/planning/FTerranSquadOrderExpander.h"
#include "sc2api/sc2_api.h"
#include "sc2api/sc2_score.h"

namespace sc2
{
namespace
{

bool Check(const bool ConditionValue, bool& SuccessValue, const std::string& MessageValue)
{
    if (!ConditionValue)
    {
        SuccessValue = false;
        std::cerr << "    " << MessageValue << std::endl;
    }

    return ConditionValue;
}

bool ApproxEqual(const float LeftValue, const float RightValue)
{
    constexpr float CoordinateToleranceValue = 0.001f;
    return std::fabs(LeftValue - RightValue) <= CoordinateToleranceValue;
}

struct FakeObservation final : ObservationInterface
{
    struct FPointVisibilityOverride
    {
        Point2D PointValue;
        Visibility VisibilityValue;
    };

    uint32_t PlayerIdValue = 1U;
    uint32_t GameLoopValue = 0U;
    Units AllUnitsValue;
    std::unordered_map<Tag, const Unit*> UnitsByTagValue;
    RawActions RawActionsValue;
    SpatialActions FeatureLayerActionsValue;
    SpatialActions RenderedActionsValue;
    std::vector<ChatMessage> ChatMessagesValue;
    std::vector<PowerSource> PowerSourcesValue;
    std::vector<Effect> EffectsValue;
    std::vector<UpgradeID> UpgradesValue;
    Score ScoreValue{};
    Abilities AbilityDataValue;
    UnitTypes UnitTypeDataValue;
    Upgrades UpgradeDataValue;
    Buffs BuffDataValue;
    Effects EffectDataValue;
    GameInfo GameInfoValue;
    uint32_t MineralsValue = 500U;
    uint32_t VespeneValue = 0U;
    uint32_t FoodCapValue = 200U;
    uint32_t FoodUsedValue = 40U;
    uint32_t FoodArmyValue = 0U;
    uint32_t FoodWorkersValue = 0U;
    uint32_t IdleWorkerCountValue = 0U;
    uint32_t ArmyCountValue = 0U;
    uint32_t WarpGateCountValue = 0U;
    uint32_t LarvaCountValue = 0U;
    Point2D CameraPositionValue;
    Point3D StartLocationValue;
    std::vector<PlayerResult> ResultsValue;
    std::vector<FPointVisibilityOverride> PointVisibilityOverridesValue;
    SC2APIProtocol::Observation RawObservationValue;
    Visibility DefaultVisibilityValue = Visibility::Visible;

    FakeObservation()
        : CameraPositionValue(32.0f, 32.0f),
          StartLocationValue(10.0f, 10.0f, 0.0f)
    {
        GameInfoValue.width = 64;
        GameInfoValue.height = 64;
        GameInfoValue.playable_min = Point2D(0.0f, 0.0f);
        GameInfoValue.playable_max = Point2D(63.0f, 63.0f);
        GameInfoValue.options.feature_layer.camera_width = 24.0f;
        GameInfoValue.options.feature_layer.map_resolution_x = 4;
        GameInfoValue.options.feature_layer.map_resolution_y = 4;
        GameInfoValue.options.feature_layer.minimap_resolution_x = 4;
        GameInfoValue.options.feature_layer.minimap_resolution_y = 4;
        GameInfoValue.start_locations.push_back(Point2D(10.0f, 10.0f));
        GameInfoValue.enemy_start_locations.push_back(Point2D(50.0f, 50.0f));
    }

    void SetUnits(const Units& NewUnitsValue)
    {
        AllUnitsValue = NewUnitsValue;
        UnitsByTagValue.clear();
        FoodArmyValue = 0U;
        FoodWorkersValue = 0U;
        IdleWorkerCountValue = 0U;
        ArmyCountValue = 0U;

        for (const Unit* UnitValue : AllUnitsValue)
        {
            if (UnitValue == nullptr)
            {
                continue;
            }

            UnitsByTagValue[UnitValue->tag] = UnitValue;
            if (UnitValue->alliance != Unit::Alliance::Self)
            {
                continue;
            }

            if (UnitValue->unit_type.ToType() == UNIT_TYPEID::TERRAN_SCV)
            {
                ++FoodWorkersValue;
                if (UnitValue->orders.empty())
                {
                    ++IdleWorkerCountValue;
                }
                continue;
            }

            if (IsTerranUnit(UnitValue->unit_type.ToType()))
            {
                ++FoodArmyValue;
                ++ArmyCountValue;
            }
        }
    }

    void SetPointVisibilityOverride(const Point2D& PointValue, const Visibility VisibilityValue)
    {
        FPointVisibilityOverride PointVisibilityOverrideValue;
        PointVisibilityOverrideValue.PointValue = PointValue;
        PointVisibilityOverrideValue.VisibilityValue = VisibilityValue;
        PointVisibilityOverridesValue.push_back(PointVisibilityOverrideValue);
    }

    uint32_t GetPlayerID() const override { return PlayerIdValue; }
    uint32_t GetGameLoop() const override { return GameLoopValue; }
    Units GetUnits() const override { return AllUnitsValue; }

    Units GetUnits(Unit::Alliance AllianceValue, Filter FilterValue = {}) const override
    {
        Units FilteredUnitsValue;
        for (const Unit* UnitValue : AllUnitsValue)
        {
            if (UnitValue == nullptr || UnitValue->alliance != AllianceValue)
            {
                continue;
            }

            if (!FilterValue || FilterValue(*UnitValue))
            {
                FilteredUnitsValue.push_back(UnitValue);
            }
        }

        return FilteredUnitsValue;
    }

    Units GetUnits(Filter FilterValue) const override
    {
        Units FilteredUnitsValue;
        for (const Unit* UnitValue : AllUnitsValue)
        {
            if (UnitValue == nullptr || UnitValue->alliance != Unit::Alliance::Self)
            {
                continue;
            }

            if (!FilterValue || FilterValue(*UnitValue))
            {
                FilteredUnitsValue.push_back(UnitValue);
            }
        }

        return FilteredUnitsValue;
    }

    const Unit* GetUnit(const Tag TagValue) const override
    {
        const std::unordered_map<Tag, const Unit*>::const_iterator FoundUnitIteratorValue = UnitsByTagValue.find(TagValue);
        return FoundUnitIteratorValue == UnitsByTagValue.end() ? nullptr : FoundUnitIteratorValue->second;
    }

    const RawActions& GetRawActions() const override { return RawActionsValue; }
    const SpatialActions& GetFeatureLayerActions() const override { return FeatureLayerActionsValue; }
    const SpatialActions& GetRenderedActions() const override { return RenderedActionsValue; }
    const std::vector<ChatMessage>& GetChatMessages() const override { return ChatMessagesValue; }
    const std::vector<PowerSource>& GetPowerSources() const override { return PowerSourcesValue; }
    const std::vector<Effect>& GetEffects() const override { return EffectsValue; }
    const std::vector<UpgradeID>& GetUpgrades() const override { return UpgradesValue; }
    const Score& GetScore() const override { return ScoreValue; }
    const Abilities& GetAbilityData(bool ForceRefresh = false) const override
    {
        (void)ForceRefresh;
        return AbilityDataValue;
    }
    const UnitTypes& GetUnitTypeData(bool ForceRefresh = false) const override
    {
        (void)ForceRefresh;
        return UnitTypeDataValue;
    }
    const Upgrades& GetUpgradeData(bool ForceRefresh = false) const override
    {
        (void)ForceRefresh;
        return UpgradeDataValue;
    }
    const Buffs& GetBuffData(bool ForceRefresh = false) const override
    {
        (void)ForceRefresh;
        return BuffDataValue;
    }
    const Effects& GetEffectData(bool ForceRefresh = false) const override
    {
        (void)ForceRefresh;
        return EffectDataValue;
    }
    const GameInfo& GetGameInfo() const override { return GameInfoValue; }
    uint32_t GetMinerals() const override { return MineralsValue; }
    uint32_t GetVespene() const override { return VespeneValue; }
    uint32_t GetFoodCap() const override { return FoodCapValue; }
    uint32_t GetFoodUsed() const override { return FoodUsedValue; }
    uint32_t GetFoodArmy() const override { return FoodArmyValue; }
    uint32_t GetFoodWorkers() const override { return FoodWorkersValue; }
    uint32_t GetIdleWorkerCount() const override { return IdleWorkerCountValue; }
    uint32_t GetArmyCount() const override { return ArmyCountValue; }
    uint32_t GetWarpGateCount() const override { return WarpGateCountValue; }
    uint32_t GetLarvaCount() const override { return LarvaCountValue; }
    Point2D GetCameraPos() const override { return CameraPositionValue; }
    Point3D GetStartLocation() const override { return StartLocationValue; }
    const std::vector<PlayerResult>& GetResults() const override { return ResultsValue; }
    bool HasCreep(const Point2D& PointValue) const override
    {
        (void)PointValue;
        return false;
    }
    Visibility GetVisibility(const Point2D& PointValue) const override
    {
        for (const FPointVisibilityOverride& PointVisibilityOverrideValue : PointVisibilityOverridesValue)
        {
            if (ApproxEqual(PointVisibilityOverrideValue.PointValue.x, PointValue.x) &&
                ApproxEqual(PointVisibilityOverrideValue.PointValue.y, PointValue.y))
            {
                return PointVisibilityOverrideValue.VisibilityValue;
            }
        }

        return DefaultVisibilityValue;
    }
    bool IsPathable(const Point2D& PointValue) const override
    {
        (void)PointValue;
        return true;
    }
    bool IsPlacable(const Point2D& PointValue) const override
    {
        (void)PointValue;
        return true;
    }
    float TerrainHeight(const Point2D& PointValue) const override
    {
        (void)PointValue;
        return 0.0f;
    }
    const SC2APIProtocol::Observation* GetRawObservation() const override { return &RawObservationValue; }
};

Unit MakeUnit(const Tag TagValue, const UNIT_TYPEID UnitTypeIdValue, const Unit::Alliance AllianceValue,
              const Point2D& PositionValue, const bool IsBuildingValue)
{
    Unit UnitValue;
    UnitValue.display_type = Unit::Visible;
    UnitValue.alliance = AllianceValue;
    UnitValue.tag = TagValue;
    UnitValue.unit_type = UnitTypeIdValue;
    UnitValue.owner = AllianceValue == Unit::Alliance::Self ? 1 : 2;
    UnitValue.pos = Point3D(PositionValue.x, PositionValue.y, 0.0f);
    UnitValue.facing = 0.0f;
    UnitValue.radius = 0.5f;
    UnitValue.build_progress = 1.0f;
    UnitValue.cloak = Unit::NotCloaked;
    UnitValue.detect_range = 0.0f;
    UnitValue.radar_range = 0.0f;
    UnitValue.is_selected = false;
    UnitValue.is_on_screen = true;
    UnitValue.is_blip = false;
    UnitValue.health = 45.0f;
    UnitValue.health_max = 45.0f;
    UnitValue.shield = 0.0f;
    UnitValue.shield_max = 0.0f;
    UnitValue.energy = 0.0f;
    UnitValue.energy_max = 0.0f;
    UnitValue.mineral_contents = 0;
    UnitValue.vespene_contents = 0;
    UnitValue.is_flying = false;
    UnitValue.is_burrowed = false;
    UnitValue.is_hallucination = false;
    UnitValue.weapon_cooldown = 0.0f;
    UnitValue.add_on_tag = NullTag;
    UnitValue.cargo_space_taken = 0;
    UnitValue.cargo_space_max = 0;
    UnitValue.assigned_harvesters = 0;
    UnitValue.ideal_harvesters = 0;
    UnitValue.engaged_target_tag = NullTag;
    UnitValue.is_powered = true;
    UnitValue.is_alive = true;
    UnitValue.last_seen_game_loop = 0;
    UnitValue.attack_upgrade_level = 0;
    UnitValue.armor_upgrade_level = 0;
    UnitValue.shield_upgrade_level = 0;
    UnitValue.is_building = IsBuildingValue;
    return UnitValue;
}

void AppendUnitPointers(const std::vector<Unit>& UnitStorageValue, Units& OutUnitsValue)
{
    OutUnitsValue.clear();
    OutUnitsValue.reserve(UnitStorageValue.size());
    for (const Unit& UnitValue : UnitStorageValue)
    {
        OutUnitsValue.push_back(&UnitValue);
    }
}

FGameStateDescriptor CreateArmyTestDescriptor()
{
    FGameStateDescriptor GameStateDescriptorValue;
    GameStateDescriptorValue.MacroState.ArmySupply = 50U;
    GameStateDescriptorValue.MacroState.ActiveBaseCount = 2U;
    GameStateDescriptorValue.ArmyState.EnsurePrimaryArmyExists();
    GameStateDescriptorValue.ArmyState.PrimaryArmyAttackSupplyThreshold = 40U;
    GameStateDescriptorValue.ArmyState.PrimaryArmyDisengageSupplyThreshold = 20U;
    return GameStateDescriptorValue;
}

FCommandOrderRecord CreateArmyMissionOrder(const uint32_t SourceGoalIdValue)
{
    FCommandOrderRecord ArmyOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::Army, NullTag, ABILITY_ID::INVALID, 100, EIntentDomain::ArmyCombat, 10U, 0U, 1U, 0);
    ArmyOrderValue.SourceGoalId = SourceGoalIdValue;
    ArmyOrderValue.TaskType = ECommandTaskType::ArmyMission;
    return ArmyOrderValue;
}

}  // namespace

bool TestTerranArmyOrderPipeline(int ArgC, char** ArgV)
{
    (void)ArgC;
    (void)ArgV;

    bool SuccessValue = true;

    FTerranArmyOrderExpander ArmyOrderExpanderValue;
    FTerranSquadOrderExpander SquadOrderExpanderValue;
    FTerranArmyUnitExecutionPlanner UnitExecutionPlannerValue;

    {
        FakeObservation ObservationValue;
        std::vector<Unit> UnitStorageValue;
        UnitStorageValue.push_back(
            MakeUnit(101U, UNIT_TYPEID::TERRAN_COMMANDCENTER, Unit::Alliance::Self, Point2D(10.0f, 10.0f), true));
        UnitStorageValue.push_back(
            MakeUnit(102U, UNIT_TYPEID::TERRAN_MARINE, Unit::Alliance::Self, Point2D(12.0f, 12.0f), false));
        UnitStorageValue.push_back(
            MakeUnit(201U, UNIT_TYPEID::PROTOSS_STALKER, Unit::Alliance::Enemy, Point2D(18.0f, 18.0f), false));
        Units ObservationUnitsValue;
        AppendUnitPointers(UnitStorageValue, ObservationUnitsValue);
        ObservationValue.SetUnits(ObservationUnitsValue);

        const FFrameContext FrameValue = FFrameContext::Create(&ObservationValue, nullptr, 1U);
        FAgentState AgentStateValue;
        AgentStateValue.Update(FrameValue);
        FGameStateDescriptor GameStateDescriptorValue = CreateArmyTestDescriptor();
        FCommandAuthoritySchedulingState SchedulingStateValue;
        SchedulingStateValue.EnqueueOrder(CreateArmyMissionOrder(400U));

        const std::vector<Point2D> ExpansionLocationsValue = {Point2D(20.0f, 20.0f), Point2D(50.0f, 50.0f)};
        ArmyOrderExpanderValue.ExpandArmyOrders(FrameValue, AgentStateValue, GameStateDescriptorValue,
                                                ExpansionLocationsValue, Point2D(12.0f, 12.0f), SchedulingStateValue);

        Check(!GameStateDescriptorValue.ArmyState.ArmyMissions.empty(), SuccessValue,
              "Army order expander should keep a primary army mission descriptor.");
        Check(GameStateDescriptorValue.ArmyState.ArmyMissions.front().MissionType == EArmyMissionType::DefendOwnedBase,
              SuccessValue, "Enemy pressure near an owned base should force the DefendOwnedBase mission.");
        Check(ApproxEqual(GameStateDescriptorValue.ArmyState.ArmyMissions.front().ObjectivePoint.x, 10.0f) &&
                  ApproxEqual(GameStateDescriptorValue.ArmyState.ArmyMissions.front().ObjectivePoint.y, 10.0f),
              SuccessValue, "DefendOwnedBase should target the threatened town hall location.");
        Check(GameStateDescriptorValue.ArmyState.ArmyMissions.front().SourceGoalId == 400U, SuccessValue,
              "Army mission selection should preserve the source goal identifier from the queued army order.");
    }

    {
        FakeObservation ObservationValue;
        std::vector<Unit> UnitStorageValue;
        UnitStorageValue.push_back(
            MakeUnit(301U, UNIT_TYPEID::TERRAN_COMMANDCENTER, Unit::Alliance::Self, Point2D(10.0f, 10.0f), true));
        UnitStorageValue.push_back(
            MakeUnit(302U, UNIT_TYPEID::TERRAN_MARINE, Unit::Alliance::Self, Point2D(14.0f, 12.0f), false));
        Units ObservationUnitsValue;
        AppendUnitPointers(UnitStorageValue, ObservationUnitsValue);
        ObservationValue.SetUnits(ObservationUnitsValue);

        const FFrameContext FrameValue = FFrameContext::Create(&ObservationValue, nullptr, 1U);
        FAgentState AgentStateValue;
        AgentStateValue.Update(FrameValue);
        FGameStateDescriptor GameStateDescriptorValue = CreateArmyTestDescriptor();
        FCommandAuthoritySchedulingState SchedulingStateValue;
        SchedulingStateValue.EnqueueOrder(CreateArmyMissionOrder(420U));

        const std::vector<Point2D> ExpansionLocationsValue = {Point2D(20.0f, 20.0f), Point2D(40.0f, 40.0f)};
        ArmyOrderExpanderValue.ExpandArmyOrders(FrameValue, AgentStateValue, GameStateDescriptorValue,
                                                ExpansionLocationsValue, Point2D(12.0f, 12.0f), SchedulingStateValue);

        Check(GameStateDescriptorValue.ArmyState.ArmyMissions.front().MissionType ==
                  EArmyMissionType::SweepExpansionLocations,
              SuccessValue, "Without visible enemy structures the army should move into sweep mode.");
        Check(ApproxEqual(GameStateDescriptorValue.ArmyState.ArmyMissions.front().ObjectivePoint.x, 20.0f) &&
                  ApproxEqual(GameStateDescriptorValue.ArmyState.ArmyMissions.front().ObjectivePoint.y, 20.0f),
              SuccessValue, "Visible empty sweep targets should advance to the next unresolved expansion.");
    }

    {
        FGameStateDescriptor GameStateDescriptorValue = CreateArmyTestDescriptor();
        GameStateDescriptorValue.ArmyState.ArmyMissions.front().MissionType = EArmyMissionType::PressureKnownEnemyBase;
        GameStateDescriptorValue.ArmyState.ArmyMissions.front().ObjectivePoint = Point2D(40.0f, 40.0f);
        GameStateDescriptorValue.ArmyState.ArmyMissions.front().SourceGoalId = 401U;

        FCommandAuthoritySchedulingState SchedulingStateValue;
        const uint32_t ArmyOrderIdValue = SchedulingStateValue.EnqueueOrder(CreateArmyMissionOrder(401U));

        FAgentState AgentStateValue;
        SquadOrderExpanderValue.ExpandSquadOrders(FFrameContext(), AgentStateValue, GameStateDescriptorValue,
                                                  Point2D(12.0f, 12.0f), SchedulingStateValue);

        Check(SchedulingStateValue.SquadOrderIndices.size() == 1U, SuccessValue,
              "Squad expansion should create one squad-layer order from the active army mission order.");
        size_t ArmyOrderIndexValue = 0U;
        Check(SchedulingStateValue.TryGetOrderIndex(ArmyOrderIdValue, ArmyOrderIndexValue), SuccessValue,
              "Army mission order should remain addressable after squad expansion.");
        Check(SchedulingStateValue.LifecycleStates[ArmyOrderIndexValue] == EOrderLifecycleState::Queued, SuccessValue,
              "Army mission orders should remain active after squad expansion so the squad child can persist until the mission changes.");
    }

    {
        FakeObservation ObservationValue;
        std::vector<Unit> UnitStorageValue;
        UnitStorageValue.push_back(
            MakeUnit(401U, UNIT_TYPEID::TERRAN_MARINE, Unit::Alliance::Self, Point2D(12.0f, 12.0f), false));
        UnitStorageValue.push_back(
            MakeUnit(402U, UNIT_TYPEID::TERRAN_MEDIVAC, Unit::Alliance::Self, Point2D(14.0f, 12.0f), false));
        Units ObservationUnitsValue;
        AppendUnitPointers(UnitStorageValue, ObservationUnitsValue);
        ObservationValue.SetUnits(ObservationUnitsValue);

        const FFrameContext FrameValue = FFrameContext::Create(&ObservationValue, nullptr, 1U);
        FAgentState AgentStateValue;
        AgentStateValue.Update(FrameValue);

        FGameStateDescriptor GameStateDescriptorValue = CreateArmyTestDescriptor();
        GameStateDescriptorValue.ArmyState.ArmyMissions.front().MissionType = EArmyMissionType::PressureKnownEnemyBase;
        GameStateDescriptorValue.ArmyState.ArmyMissions.front().ObjectivePoint = Point2D(40.0f, 40.0f);
        GameStateDescriptorValue.ArmyState.ArmyMissions.front().SourceGoalId = 402U;

        FCommandAuthoritySchedulingState SchedulingStateValue;
        FCommandOrderRecord SquadOrderValue = FCommandOrderRecord::CreatePointTarget(
            ECommandAuthorityLayer::Squad, NullTag, ABILITY_ID::INVALID, Point2D(40.0f, 40.0f), 100,
            EIntentDomain::ArmyCombat, 20U, 0U, 77U, 0, 0);
        SquadOrderValue.SourceGoalId = 402U;
        SquadOrderValue.TaskType = ECommandTaskType::ArmyMission;
        SchedulingStateValue.EnqueueOrder(SquadOrderValue);

        const uint32_t CreatedOrderCountValue = UnitExecutionPlannerValue.ExpandUnitExecutionOrders(
            FrameValue, AgentStateValue, GameStateDescriptorValue, Point2D(16.0f, 16.0f), SchedulingStateValue);

        Check(CreatedOrderCountValue == 2U, SuccessValue,
              "Unit execution planner should emit one execution order per eligible combat unit.");
        Check(SchedulingStateValue.ReadyIntentIndices.size() == 2U, SuccessValue,
              "Created execution orders should enter the ready-intent buffer immediately.");

        size_t MarineOrderIndexValue = 0U;
        size_t MedivacOrderIndexValue = 0U;
        for (const size_t ReadyIntentIndexValue : SchedulingStateValue.ReadyIntentIndices)
        {
            if (SchedulingStateValue.ActorTags[ReadyIntentIndexValue] == 401U)
            {
                MarineOrderIndexValue = ReadyIntentIndexValue;
            }
            if (SchedulingStateValue.ActorTags[ReadyIntentIndexValue] == 402U)
            {
                MedivacOrderIndexValue = ReadyIntentIndexValue;
            }
        }

        Check(SchedulingStateValue.AbilityIds[MarineOrderIndexValue] == ABILITY_ID::ATTACK_ATTACK, SuccessValue,
              "Pressure missions should use attack-move for marine execution orders.");
        Check(SchedulingStateValue.AbilityIds[MedivacOrderIndexValue] == ABILITY_ID::MOVE_MOVE, SuccessValue,
              "Support air units should use move orders when following the army mission.");
        Check(!SchedulingStateValue.QueuedValues[MarineOrderIndexValue] &&
                  !SchedulingStateValue.QueuedValues[MedivacOrderIndexValue],
              SuccessValue, "Combat execution orders should replace prior commands instead of being queued behind them.");
    }

    {
        FakeObservation ObservationValue;
        std::vector<Unit> UnitStorageValue;
        UnitStorageValue.push_back(
            MakeUnit(411U, UNIT_TYPEID::TERRAN_MARINE, Unit::Alliance::Self, Point2D(12.0f, 12.0f), false));
        UnitStorageValue.push_back(
            MakeUnit(412U, UNIT_TYPEID::TERRAN_MEDIVAC, Unit::Alliance::Self, Point2D(14.0f, 12.0f), false));
        Units ObservationUnitsValue;
        AppendUnitPointers(UnitStorageValue, ObservationUnitsValue);
        ObservationValue.SetUnits(ObservationUnitsValue);

        const FFrameContext FrameValue = FFrameContext::Create(&ObservationValue, nullptr, 1U);
        FAgentState AgentStateValue;
        AgentStateValue.Update(FrameValue);

        FGameStateDescriptor GameStateDescriptorValue = CreateArmyTestDescriptor();
        GameStateDescriptorValue.ArmyState.ArmyMissions.front().MissionType = EArmyMissionType::AssembleAtRally;
        GameStateDescriptorValue.ArmyState.ArmyMissions.front().ObjectivePoint = Point2D(24.0f, 24.0f);
        GameStateDescriptorValue.ArmyState.ArmyMissions.front().SourceGoalId = 403U;

        FCommandAuthoritySchedulingState SchedulingStateValue;
        FCommandOrderRecord SquadOrderValue = FCommandOrderRecord::CreatePointTarget(
            ECommandAuthorityLayer::Squad, NullTag, ABILITY_ID::INVALID, Point2D(24.0f, 24.0f), 100,
            EIntentDomain::ArmyCombat, 20U, 0U, 79U, 0, 0);
        SquadOrderValue.SourceGoalId = 403U;
        SquadOrderValue.TaskType = ECommandTaskType::ArmyMission;
        SchedulingStateValue.EnqueueOrder(SquadOrderValue);

        const uint32_t CreatedOrderCountValue = UnitExecutionPlannerValue.ExpandUnitExecutionOrders(
            FrameValue, AgentStateValue, GameStateDescriptorValue, Point2D(24.0f, 24.0f), SchedulingStateValue);

        Check(CreatedOrderCountValue == 2U, SuccessValue,
              "Assemble missions should issue explicit execution orders toward the assembly anchor for units outside the radius.");

        size_t MarineOrderIndexValue = 0U;
        size_t MedivacOrderIndexValue = 0U;
        for (const size_t ReadyIntentIndexValue : SchedulingStateValue.ReadyIntentIndices)
        {
            if (SchedulingStateValue.ActorTags[ReadyIntentIndexValue] == 411U)
            {
                MarineOrderIndexValue = ReadyIntentIndexValue;
            }
            if (SchedulingStateValue.ActorTags[ReadyIntentIndexValue] == 412U)
            {
                MedivacOrderIndexValue = ReadyIntentIndexValue;
            }
        }

        Check(SchedulingStateValue.AbilityIds[MarineOrderIndexValue] == ABILITY_ID::ATTACK_ATTACK, SuccessValue,
              "Ground combat units should assemble with attack-move orders toward the natural assembly anchor.");
        Check(SchedulingStateValue.AbilityIds[MedivacOrderIndexValue] == ABILITY_ID::MOVE_MOVE, SuccessValue,
              "Support air units should assemble with move orders toward the natural assembly anchor.");
        Check(ApproxEqual(SchedulingStateValue.TargetPoints[MarineOrderIndexValue].x, 24.0f) &&
                  ApproxEqual(SchedulingStateValue.TargetPoints[MarineOrderIndexValue].y, 24.0f),
              SuccessValue, "Assembly execution orders should target the provided assembly anchor exactly.");
    }

    {
        FakeObservation ObservationValue;
        std::vector<Unit> UnitStorageValue;
        UnitStorageValue.push_back(
            MakeUnit(421U, UNIT_TYPEID::TERRAN_MARINE, Unit::Alliance::Self, Point2D(23.0f, 23.0f), false));
        Units ObservationUnitsValue;
        AppendUnitPointers(UnitStorageValue, ObservationUnitsValue);
        ObservationValue.SetUnits(ObservationUnitsValue);

        const FFrameContext FrameValue = FFrameContext::Create(&ObservationValue, nullptr, 1U);
        FAgentState AgentStateValue;
        AgentStateValue.Update(FrameValue);

        FGameStateDescriptor GameStateDescriptorValue = CreateArmyTestDescriptor();
        GameStateDescriptorValue.ArmyState.ArmyMissions.front().MissionType = EArmyMissionType::AssembleAtRally;
        GameStateDescriptorValue.ArmyState.ArmyMissions.front().ObjectivePoint = Point2D(24.0f, 24.0f);
        GameStateDescriptorValue.ArmyState.ArmyMissions.front().SourceGoalId = 404U;

        FCommandAuthoritySchedulingState SchedulingStateValue;
        FCommandOrderRecord SquadOrderValue = FCommandOrderRecord::CreatePointTarget(
            ECommandAuthorityLayer::Squad, NullTag, ABILITY_ID::INVALID, Point2D(24.0f, 24.0f), 100,
            EIntentDomain::ArmyCombat, 20U, 0U, 80U, 0, 0);
        SquadOrderValue.SourceGoalId = 404U;
        SquadOrderValue.TaskType = ECommandTaskType::ArmyMission;
        SchedulingStateValue.EnqueueOrder(SquadOrderValue);

        const uint32_t CreatedOrderCountValue = UnitExecutionPlannerValue.ExpandUnitExecutionOrders(
            FrameValue, AgentStateValue, GameStateDescriptorValue, Point2D(24.0f, 24.0f), SchedulingStateValue);

        Check(CreatedOrderCountValue == 1U, SuccessValue,
              "A unit already inside the assembly radius should receive one hold-position stabilization order.");
        Check(SchedulingStateValue.ReadyIntentIndices.size() == 1U, SuccessValue,
              "The inside-radius assembly case should emit exactly one execution order.");
        if (!SchedulingStateValue.ReadyIntentIndices.empty())
        {
            const size_t ReadyOrderIndexValue = SchedulingStateValue.ReadyIntentIndices.front();
            Check(SchedulingStateValue.AbilityIds[ReadyOrderIndexValue] == ABILITY_ID::GENERAL_HOLDPOSITION,
                  SuccessValue,
                  "A unit already inside the assembly radius should hold instead of being repeatedly advanced.");
        }
    }

    {
        FakeObservation ObservationValue;
        std::vector<Unit> UnitStorageValue;
        Unit UnitValue =
            MakeUnit(501U, UNIT_TYPEID::TERRAN_MARINE, Unit::Alliance::Self, Point2D(12.0f, 12.0f), false);
        UnitOrder ExistingOrderValue;
        ExistingOrderValue.ability_id = ABILITY_ID::ATTACK_ATTACK;
        ExistingOrderValue.target_pos = Point2D(40.0f, 40.0f);
        UnitValue.orders.push_back(ExistingOrderValue);
        UnitStorageValue.push_back(UnitValue);
        Units ObservationUnitsValue;
        AppendUnitPointers(UnitStorageValue, ObservationUnitsValue);
        ObservationValue.SetUnits(ObservationUnitsValue);

        const FFrameContext FrameValue = FFrameContext::Create(&ObservationValue, nullptr, 1U);
        FAgentState AgentStateValue;
        AgentStateValue.Update(FrameValue);

        FGameStateDescriptor GameStateDescriptorValue = CreateArmyTestDescriptor();
        GameStateDescriptorValue.ArmyState.ArmyMissions.front().MissionType = EArmyMissionType::PressureKnownEnemyBase;
        GameStateDescriptorValue.ArmyState.ArmyMissions.front().ObjectivePoint = Point2D(40.0f, 40.0f);

        FCommandAuthoritySchedulingState SchedulingStateValue;
        FCommandOrderRecord SquadOrderValue = FCommandOrderRecord::CreatePointTarget(
            ECommandAuthorityLayer::Squad, NullTag, ABILITY_ID::INVALID, Point2D(40.0f, 40.0f), 100,
            EIntentDomain::ArmyCombat, 20U, 0U, 88U, 0, 0);
        SquadOrderValue.TaskType = ECommandTaskType::ArmyMission;
        SchedulingStateValue.EnqueueOrder(SquadOrderValue);

        const uint32_t CreatedOrderCountValue = UnitExecutionPlannerValue.ExpandUnitExecutionOrders(
            FrameValue, AgentStateValue, GameStateDescriptorValue, Point2D(16.0f, 16.0f), SchedulingStateValue);

        Check(CreatedOrderCountValue == 0U, SuccessValue,
              "Unit execution planner should not duplicate an already matching attack order.");
        Check(SchedulingStateValue.ReadyIntentIndices.empty(), SuccessValue,
              "No refreshed execution order should be queued when the actor already has the correct order.");
    }

    {
        FakeObservation ObservationValue;
        std::vector<Unit> UnitStorageValue;
        UnitStorageValue.push_back(
            MakeUnit(601U, UNIT_TYPEID::TERRAN_MARINE, Unit::Alliance::Self, Point2D(12.0f, 12.0f), false));
        Units ObservationUnitsValue;
        AppendUnitPointers(UnitStorageValue, ObservationUnitsValue);
        ObservationValue.SetUnits(ObservationUnitsValue);

        const FFrameContext FrameValue = FFrameContext::Create(&ObservationValue, nullptr, 1U);
        FAgentState AgentStateValue;
        AgentStateValue.Update(FrameValue);

        FGameStateDescriptor GameStateDescriptorValue = CreateArmyTestDescriptor();
        GameStateDescriptorValue.ArmyState.ArmyMissions.front().MissionType = EArmyMissionType::PressureKnownEnemyBase;
        GameStateDescriptorValue.ArmyState.ArmyMissions.front().ObjectivePoint = Point2D(40.0f, 40.0f);

        FCommandAuthoritySchedulingState SchedulingStateValue;
        FCommandOrderRecord SquadOrderValue = FCommandOrderRecord::CreatePointTarget(
            ECommandAuthorityLayer::Squad, NullTag, ABILITY_ID::INVALID, Point2D(40.0f, 40.0f), 100,
            EIntentDomain::ArmyCombat, 20U, 0U, 98U, 0, 0);
        SquadOrderValue.TaskType = ECommandTaskType::ArmyMission;
        SchedulingStateValue.EnqueueOrder(SquadOrderValue);

        FCommandOrderRecord ExistingExecutionOrderValue = FCommandOrderRecord::CreatePointTarget(
            ECommandAuthorityLayer::UnitExecution, 601U, ABILITY_ID::ATTACK_ATTACK, Point2D(40.0f, 40.0f), 100,
            EIntentDomain::ArmyCombat, 20U, 0U, SquadOrderValue.OrderId, 0, 0, true, false, false);
        ExistingExecutionOrderValue.LifecycleState = EOrderLifecycleState::Ready;
        SchedulingStateValue.EnqueueOrder(ExistingExecutionOrderValue);

        const size_t ReadyIntentCountBeforeValue = SchedulingStateValue.ReadyIntentIndices.size();
        const uint32_t CreatedOrderCountValue = UnitExecutionPlannerValue.ExpandUnitExecutionOrders(
            FrameValue, AgentStateValue, GameStateDescriptorValue, Point2D(16.0f, 16.0f), SchedulingStateValue);

        Check(CreatedOrderCountValue == 0U, SuccessValue,
              "Unit execution planner should not enqueue a duplicate order when a matching active execution order already exists.");
        Check(SchedulingStateValue.ReadyIntentIndices.size() == ReadyIntentCountBeforeValue, SuccessValue,
              "Matching active execution orders should keep the ready-intent backlog unchanged.");
    }

    {
        FakeObservation ObservationValue;
        std::vector<Unit> UnitStorageValue;
        UnitStorageValue.push_back(
            MakeUnit(701U, UNIT_TYPEID::TERRAN_MARINE, Unit::Alliance::Self, Point2D(12.0f, 12.0f), false));
        UnitStorageValue.push_back(
            MakeUnit(702U, UNIT_TYPEID::TERRAN_MARINE, Unit::Alliance::Self, Point2D(14.0f, 12.0f), false));
        Units ObservationUnitsValue;
        AppendUnitPointers(UnitStorageValue, ObservationUnitsValue);
        ObservationValue.SetUnits(ObservationUnitsValue);

        const FFrameContext FrameValue = FFrameContext::Create(&ObservationValue, nullptr, 1U);
        FAgentState AgentStateValue;
        AgentStateValue.Update(FrameValue);

        FGameStateDescriptor GameStateDescriptorValue = CreateArmyTestDescriptor();
        GameStateDescriptorValue.ArmyState.ArmyMissions.front().MissionType = EArmyMissionType::PressureKnownEnemyBase;
        GameStateDescriptorValue.ArmyState.ArmyMissions.front().ObjectivePoint = Point2D(40.0f, 40.0f);

        FCommandAuthoritySchedulingState SchedulingStateValue;
        SchedulingStateValue.MaxActiveUnitExecutionOrders = 1U;
        FCommandOrderRecord SquadOrderValue = FCommandOrderRecord::CreatePointTarget(
            ECommandAuthorityLayer::Squad, NullTag, ABILITY_ID::INVALID, Point2D(40.0f, 40.0f), 100,
            EIntentDomain::ArmyCombat, 20U, 0U, 108U, 0, 0);
        SquadOrderValue.TaskType = ECommandTaskType::ArmyMission;
        SchedulingStateValue.EnqueueOrder(SquadOrderValue);

        const uint32_t CreatedOrderCountValue = UnitExecutionPlannerValue.ExpandUnitExecutionOrders(
            FrameValue, AgentStateValue, GameStateDescriptorValue, Point2D(16.0f, 16.0f), SchedulingStateValue);

        Check(CreatedOrderCountValue == 1U, SuccessValue,
              "Unit execution planner should stop creating new execution orders once the active admission cap is reached.");
        Check(SchedulingStateValue.RejectedUnitExecutionAdmissionCount == 1U, SuccessValue,
              "Rejected unit-execution admissions should be counted when the active admission cap blocks new work.");
    }

    return SuccessValue;
}

}  // namespace sc2
