#include "test_singularity_framework.h"

#include <cmath>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/agent_framework.h"
#include "common/bot_status_models.h"
#include "common/terran_unit_container.h"
#include "sc2api/sc2_api.h"
#include "sc2api/sc2_score.h"

namespace sc2
{
namespace
{

bool Check(bool Condition, bool& Success, const std::string& Message)
{
    if (!Condition)
    {
        Success = false;
        std::cerr << "    " << Message << std::endl;
    }
    return Condition;
}

bool ApproxEqual(float Lhs, float Rhs, float Epsilon = 0.001f)
{
    return std::fabs(Lhs - Rhs) <= Epsilon;
}

struct FakeQuery : QueryInterface
{
    bool PlacementResult = true;
    float PointPathingResult = 1.0f;
    float UnitPathingResult = 1.0f;

    AvailableAbilities GetAbilitiesForUnit(const Unit* UnitPtr, bool IgnoreResourceRequirements = false,
                                           bool UseGeneralizedAbility = true) override
    {
        (void)UnitPtr;
        (void)IgnoreResourceRequirements;
        (void)UseGeneralizedAbility;
        return {};
    }

    std::vector<AvailableAbilities> GetAbilitiesForUnits(const Units& UnitsToQuery,
                                                         bool IgnoreResourceRequirements = false,
                                                         bool UseGeneralizedAbility = true) override
    {
        (void)IgnoreResourceRequirements;
        (void)UseGeneralizedAbility;
        return std::vector<AvailableAbilities>(UnitsToQuery.size());
    }

    float PathingDistance(const Point2D& Start, const Point2D& End) override
    {
        (void)Start;
        (void)End;
        return PointPathingResult;
    }

    float PathingDistance(const Unit* Start, const Point2D& End) override
    {
        (void)Start;
        (void)End;
        return UnitPathingResult;
    }

    std::vector<float> PathingDistance(const std::vector<PathingQuery>& Queries) override
    {
        return std::vector<float>(Queries.size(), PointPathingResult);
    }

    bool Placement(const AbilityID& Ability, const Point2D& TargetPos, const Unit* UnitPtr = nullptr) override
    {
        (void)Ability;
        (void)TargetPos;
        (void)UnitPtr;
        return PlacementResult;
    }

    std::vector<bool> Placement(const std::vector<PlacementQuery>& Queries) override
    {
        return std::vector<bool>(Queries.size(), PlacementResult);
    }
};

struct FakeObservation : ObservationInterface
{
    uint32_t PlayerId = 1;
    uint32_t GameLoopValue = 0;
    Units AllUnits;
    std::unordered_map<Tag, const Unit*> TagLookup;
    RawActions RawActionsValue;
    SpatialActions FeatureLayerActions;
    SpatialActions RenderedActions;
    std::vector<ChatMessage> ChatMessages;
    std::vector<PowerSource> PowerSources;
    std::vector<Effect> EffectList;
    std::vector<UpgradeID> UpgradeList;
    Score ScoreValue{};
    Abilities AbilitiesValue;
    UnitTypes UnitTypeData;
    Upgrades UpgradeDataValue;
    Buffs BuffDataValue;
    Effects EffectDataValue;
    GameInfo GameInfoData;
    uint32_t MineralsValue = 0;
    uint32_t VespeneValue = 0;
    uint32_t FoodCap = 0;
    uint32_t FoodUsed = 0;
    uint32_t FoodArmy = 0;
    uint32_t FoodWorkers = 0;
    uint32_t IdleWorkerCount = 0;
    uint32_t ArmyCountValue = 0;
    uint32_t WarpGateCount = 0;
    uint32_t LarvaCount = 0;
    Point2D CameraPos;
    Point3D StartLocation;
    std::vector<PlayerResult> ResultsValue;
    SC2APIProtocol::Observation RawObservationValue;

    FakeObservation()
        : CameraPos(32.0f, 32.0f), StartLocation(10.0f, 10.0f, 0.0f)
    {
        GameInfoData.width = 64;
        GameInfoData.height = 64;
        GameInfoData.playable_min = Point2D(0.0f, 0.0f);
        GameInfoData.playable_max = Point2D(63.0f, 63.0f);
        GameInfoData.options.feature_layer.camera_width = 24.0f;
        GameInfoData.options.feature_layer.map_resolution_x = 4;
        GameInfoData.options.feature_layer.map_resolution_y = 4;
        GameInfoData.options.feature_layer.minimap_resolution_x = 4;
        GameInfoData.options.feature_layer.minimap_resolution_y = 4;
        GameInfoData.enemy_start_locations.push_back(Point2D(50.0f, 50.0f));
        GameInfoData.start_locations.push_back(Point2D(10.0f, 10.0f));
    }

    void SetUnits(const Units& NewUnits)
    {
        AllUnits = NewUnits;
        TagLookup.clear();
        IdleWorkerCount = 0;
        ArmyCountValue = 0;
        FoodWorkers = 0;
        FoodArmy = 0;

        for (const Unit* UnitPtr : AllUnits)
        {
            if (!UnitPtr)
            {
                continue;
            }

            TagLookup[UnitPtr->tag] = UnitPtr;
            if (UnitPtr->alliance != Unit::Alliance::Self)
            {
                continue;
            }

            if (UnitPtr->unit_type.ToType() == UNIT_TYPEID::TERRAN_SCV)
            {
                ++FoodWorkers;
                if (UnitPtr->orders.empty())
                {
                    ++IdleWorkerCount;
                }
            }
            else if (IsTerranUnit(UnitPtr->unit_type.ToType()))
            {
                ++ArmyCountValue;
                ++FoodArmy;
            }
        }
    }

    uint32_t GetPlayerID() const override
    {
        return PlayerId;
    }

    uint32_t GetGameLoop() const override
    {
        return GameLoopValue;
    }

    Units GetUnits() const override
    {
        return AllUnits;
    }

    Units GetUnits(Unit::Alliance Alliance, Filter FilterValue = {}) const override
    {
        Units Result;
        for (const Unit* UnitPtr : AllUnits)
        {
            if (!UnitPtr || UnitPtr->alliance != Alliance)
            {
                continue;
            }
            if (!FilterValue || FilterValue(*UnitPtr))
            {
                Result.push_back(UnitPtr);
            }
        }
        return Result;
    }

    Units GetUnits(Filter FilterValue) const override
    {
        Units Result;
        for (const Unit* UnitPtr : AllUnits)
        {
            if (!UnitPtr || UnitPtr->alliance != Unit::Alliance::Self)
            {
                continue;
            }
            if (!FilterValue || FilterValue(*UnitPtr))
            {
                Result.push_back(UnitPtr);
            }
        }
        return Result;
    }

    const Unit* GetUnit(Tag TagValue) const override
    {
        auto Found = TagLookup.find(TagValue);
        return Found == TagLookup.end() ? nullptr : Found->second;
    }

    const RawActions& GetRawActions() const override
    {
        return RawActionsValue;
    }

    const SpatialActions& GetFeatureLayerActions() const override
    {
        return FeatureLayerActions;
    }

    const SpatialActions& GetRenderedActions() const override
    {
        return RenderedActions;
    }

    const std::vector<ChatMessage>& GetChatMessages() const override
    {
        return ChatMessages;
    }

    const std::vector<PowerSource>& GetPowerSources() const override
    {
        return PowerSources;
    }

    const std::vector<Effect>& GetEffects() const override
    {
        return EffectList;
    }

    const std::vector<UpgradeID>& GetUpgrades() const override
    {
        return UpgradeList;
    }

    const Score& GetScore() const override
    {
        return ScoreValue;
    }

    const Abilities& GetAbilityData(bool ForceRefresh = false) const override
    {
        (void)ForceRefresh;
        return AbilitiesValue;
    }

    const UnitTypes& GetUnitTypeData(bool ForceRefresh = false) const override
    {
        (void)ForceRefresh;
        return UnitTypeData;
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

    const GameInfo& GetGameInfo() const override
    {
        return GameInfoData;
    }

    uint32_t GetMinerals() const override
    {
        return MineralsValue;
    }

    uint32_t GetVespene() const override
    {
        return VespeneValue;
    }

    uint32_t GetFoodCap() const override
    {
        return FoodCap;
    }

    uint32_t GetFoodUsed() const override
    {
        return FoodUsed;
    }

    uint32_t GetFoodArmy() const override
    {
        return FoodArmy;
    }

    uint32_t GetFoodWorkers() const override
    {
        return FoodWorkers;
    }

    uint32_t GetIdleWorkerCount() const override
    {
        return IdleWorkerCount;
    }

    uint32_t GetArmyCount() const override
    {
        return ArmyCountValue;
    }

    uint32_t GetWarpGateCount() const override
    {
        return WarpGateCount;
    }

    uint32_t GetLarvaCount() const override
    {
        return LarvaCount;
    }

    Point2D GetCameraPos() const override
    {
        return CameraPos;
    }

    Point3D GetStartLocation() const override
    {
        return StartLocation;
    }

    const std::vector<PlayerResult>& GetResults() const override
    {
        return ResultsValue;
    }

    bool HasCreep(const Point2D& PointValue) const override
    {
        (void)PointValue;
        return false;
    }

    Visibility GetVisibility(const Point2D& PointValue) const override
    {
        (void)PointValue;
        return Visibility::Visible;
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

    const SC2APIProtocol::Observation* GetRawObservation() const override
    {
        return &RawObservationValue;
    }
};

Unit MakeUnit(Tag TagValue, UNIT_TYPEID UnitTypeId, Unit::Alliance Alliance, const Point2D& Position,
              bool IsBuilding = false, float BuildProgress = 1.0f)
{
    Unit UnitValue;
    UnitValue.display_type = Unit::Visible;
    UnitValue.alliance = Alliance;
    UnitValue.tag = TagValue;
    UnitValue.unit_type = UnitTypeId;
    UnitValue.owner = Alliance == Unit::Alliance::Self ? 1 : 0;
    UnitValue.pos = Point3D(Position.x, Position.y, 0.0f);
    UnitValue.facing = 0.0f;
    UnitValue.radius = 0.5f;
    UnitValue.build_progress = BuildProgress;
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
    UnitValue.mineral_contents = 1500;
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
    UnitValue.is_building = IsBuilding;
    return UnitValue;
}

void SetImageData(SC2APIProtocol::ImageData* Image, int Width, int Height, const std::vector<uint8_t>& Values)
{
    Image->mutable_size()->set_x(Width);
    Image->mutable_size()->set_y(Height);
    Image->set_bits_per_pixel(8);
    Image->set_data(std::string(Values.begin(), Values.end()));
}

void SetValidFeatureLayers(FakeObservation& ObservationValue)
{
    SC2APIProtocol::ObservationFeatureLayer* FeatureLayerData = ObservationValue.RawObservationValue.mutable_feature_layer_data();
    SC2APIProtocol::FeatureLayers* MapLayers = FeatureLayerData->mutable_renders();
    SC2APIProtocol::FeatureLayersMinimap* MinimapLayers = FeatureLayerData->mutable_minimap_renders();

    SetImageData(MapLayers->mutable_player_relative(), 4, 4,
                 {
                     0, 0, 0, 4,
                     0, 1, 0, 0,
                     0, 0, 3, 0,
                     0, 0, 0, 0,
                 });

    SetImageData(MinimapLayers->mutable_player_relative(), 4, 4,
                 {
                     0, 4, 0, 0,
                     0, 0, 0, 0,
                     0, 1, 0, 0,
                     0, 0, 3, 0,
                 });

    SetImageData(MinimapLayers->mutable_height_map(), 4, 4,
                 {
                     1, 2, 3, 4,
                     5, 6, 7, 8,
                     9, 10, 11, 12,
                     13, 14, 15, 16,
                 });
}

bool TestContainerResetAndSync()
{
    bool Success = true;

    Unit WorkerUnit = MakeUnit(1, UNIT_TYPEID::TERRAN_SCV, Unit::Alliance::Self, Point2D(10.0f, 10.0f));
    WorkerUnit.orders.push_back({ABILITY_ID::HARVEST_GATHER, NullTag, Point2D(12.0f, 12.0f), 0.5f});
    WorkerUnit.buffs.push_back(static_cast<BuffID>(1));

    Unit MarineUnit = MakeUnit(2, UNIT_TYPEID::TERRAN_MARINE, Unit::Alliance::Self, Point2D(11.0f, 11.0f));
    MarineUnit.buffs.push_back(static_cast<BuffID>(2));

    FTerranUnitContainer Container;
    Container.SetUnits({&WorkerUnit, &MarineUnit});

    Check(Container.HasSynchronizedSizes(), Success, "SetUnits should keep every SoA container synchronized.");
    Check(Container.UnitBuffs.size() == 2, Success, "Unit buffs should track the number of controlled units.");

    Container.SetUnits({&WorkerUnit});
    Check(Container.HasSynchronizedSizes(), Success, "ResetAll should keep repeated SetUnits calls synchronized.");
    Check(Container.UnitBuffs.size() == 1, Success, "ResetAll should clear stale unit buff entries.");

    Container.ResetAll();
    Check(Container.HasSynchronizedSizes(), Success, "ResetAll should leave the container empty and synchronized.");
    Check(Container.UnitBuffs.empty(), Success, "ResetAll should clear unit buff storage.");

    return Success;
}

bool TestBuildingCountsAndConstructionTracking()
{
    bool Success = true;

    FakeObservation ObservationValue;
    ObservationValue.MineralsValue = 400;
    ObservationValue.FoodCap = 23;
    ObservationValue.FoodUsed = 11;

    Unit CommandCenter = MakeUnit(10, UNIT_TYPEID::TERRAN_COMMANDCENTER, Unit::Alliance::Self, Point2D(12.0f, 12.0f),
                                  true);
    CommandCenter.orders.push_back({ABILITY_ID::TRAIN_SCV, NullTag, Point2D(), 0.25f});

    Unit Barracks = MakeUnit(11, UNIT_TYPEID::TERRAN_BARRACKS, Unit::Alliance::Self, Point2D(16.0f, 15.0f), true);
    Barracks.orders.push_back({ABILITY_ID::TRAIN_MARINE, NullTag, Point2D(), 0.5f});

    Unit UnfinishedBarracks = MakeUnit(12, UNIT_TYPEID::TERRAN_BARRACKS, Unit::Alliance::Self,
                                       Point2D(18.0f, 15.0f), true, 0.4f);

    Unit BarracksReactor = MakeUnit(13, UNIT_TYPEID::TERRAN_BARRACKSREACTOR, Unit::Alliance::Self,
                                    Point2D(16.0f, 14.0f), true);
    Unit Factory = MakeUnit(14, UNIT_TYPEID::TERRAN_FACTORY, Unit::Alliance::Self, Point2D(20.0f, 15.0f), true);
    Unit FactoryTechLab = MakeUnit(15, UNIT_TYPEID::TERRAN_FACTORYTECHLAB, Unit::Alliance::Self,
                                   Point2D(20.0f, 14.0f), true);
    Unit Starport = MakeUnit(16, UNIT_TYPEID::TERRAN_STARPORT, Unit::Alliance::Self, Point2D(24.0f, 15.0f), true);
    Unit StarportReactor = MakeUnit(17, UNIT_TYPEID::TERRAN_STARPORTREACTOR, Unit::Alliance::Self,
                                    Point2D(24.0f, 14.0f), true);

    ObservationValue.SetUnits({&CommandCenter, &Barracks, &UnfinishedBarracks, &BarracksReactor, &Factory,
                               &FactoryTechLab, &Starport, &StarportReactor});

    FAgentState StateValue;
    StateValue.Update(FFrameContext::Create(&ObservationValue, nullptr, 1));

    Check(StateValue.Buildings.GetBarracksCount() == 1, Success, "Barracks count should exclude Barracks add-ons.");
    Check(StateValue.Buildings.GetBarracksAddonCount() == 1, Success,
          "Barracks add-on count should track Barracks-specific add-ons separately.");
    Check(StateValue.Buildings.GetFactoryCount() == 1, Success, "Factory count should exclude Factory add-ons.");
    Check(StateValue.Buildings.GetFactoryAddonCount() == 1, Success,
          "Factory add-on count should track Factory-specific add-ons separately.");
    Check(StateValue.Buildings.GetStarportCount() == 1, Success, "Starport count should exclude Starport add-ons.");
    Check(StateValue.Buildings.GetStarportAddonCount() == 1, Success,
          "Starport add-on count should track Starport-specific add-ons separately.");
    Check(StateValue.Units.GetUnitsInConstruction(UNIT_TYPEID::TERRAN_SCV) == 1, Success,
          "Town hall training orders should count units in construction.");
    Check(StateValue.Units.GetUnitsInConstruction(UNIT_TYPEID::TERRAN_MARINE) == 1, Success,
          "Barracks training orders should count marines in construction.");
    Check(StateValue.Buildings.GetCurrentlyInConstruction(UNIT_TYPEID::TERRAN_BARRACKS) == 1, Success,
          "Unfinished buildings should be counted separately from finished buildings.");

    CommandCenter.orders.clear();
    Barracks.orders.clear();
    ObservationValue.SetUnits({&CommandCenter, &Barracks, &BarracksReactor});
    StateValue.Update(FFrameContext::Create(&ObservationValue, nullptr, 2));

    Check(StateValue.Units.GetUnitsInConstruction(UNIT_TYPEID::TERRAN_SCV) == 0, Success,
          "SCV construction counts should reset cleanly across frames.");
    Check(StateValue.Units.GetUnitsInConstruction(UNIT_TYPEID::TERRAN_MARINE) == 0, Success,
          "Marine construction counts should reset cleanly across frames.");
    Check(StateValue.Buildings.GetCurrentlyInConstruction(UNIT_TYPEID::TERRAN_BARRACKS) == 0, Success,
          "Building construction counts should reset when the unfinished structure disappears.");

    return Success;
}

bool TestSpatialChannelsAndMetrics()
{
    bool Success = true;

    FakeObservation ObservationValue;
    ObservationValue.MineralsValue = 300;
    ObservationValue.FoodCap = 15;
    ObservationValue.FoodUsed = 6;
    SetValidFeatureLayers(ObservationValue);

    Unit MarineUnit = MakeUnit(20, UNIT_TYPEID::TERRAN_MARINE, Unit::Alliance::Self, Point2D(10.0f, 10.0f));
    ObservationValue.SetUnits({&MarineUnit});

    FAgentState StateValue;
    StateValue.Update(FFrameContext::Create(&ObservationValue, nullptr, 4));

    Check(StateValue.SpatialChannels.Valid, Success,
          "Feature-layer channels should load valid player-relative and height maps.");
    Check(StateValue.SpatialMetrics.Valid, Success, "Spatial metrics should be derived when channels are valid.");
    Check(StateValue.SpatialMetrics.Map.SelfCount == 1, Success, "Map metrics should count self cells.");
    Check(StateValue.SpatialMetrics.Map.EnemyCount == 1, Success, "Map metrics should count enemy cells.");
    Check(StateValue.SpatialMetrics.Map.NeutralCount == 1, Success, "Map metrics should count neutral cells.");
    Check(ApproxEqual(StateValue.SpatialMetrics.Map.SelfCentroid.x, 1.0f) &&
              ApproxEqual(StateValue.SpatialMetrics.Map.SelfCentroid.y, 1.0f),
          Success, "Map self centroid should be derived from player-relative occupancy.");
    Check(ApproxEqual(StateValue.SpatialMetrics.Map.EnemyCentroid.x, 3.0f) &&
              ApproxEqual(StateValue.SpatialMetrics.Map.EnemyCentroid.y, 0.0f),
          Success, "Map enemy centroid should be derived from player-relative occupancy.");
    Check(ApproxEqual(StateValue.SpatialMetrics.Map.NeutralCentroid.x, 2.0f) &&
              ApproxEqual(StateValue.SpatialMetrics.Map.NeutralCentroid.y, 2.0f),
          Success, "Map neutral centroid should be derived from player-relative occupancy.");
    Check(StateValue.SpatialMetrics.Minimap.HasEnemy, Success, "Minimap metrics should surface enemy presence.");
    Check(StateValue.SpatialMetrics.Minimap.HasSelf, Success, "Minimap metrics should surface self presence.");

    FakeObservation InvalidObservation;
    InvalidObservation.RawObservationValue.mutable_feature_layer_data()->mutable_renders()->mutable_player_relative();
    InvalidObservation.SetUnits({&MarineUnit});

    FAgentSpatialChannels ChannelsValue;
    ChannelsValue.Update(FFrameContext::Create(&InvalidObservation, nullptr, 5));
    Check(!ChannelsValue.Valid, Success,
          "Channels should stay invalid when required feature-layer textures are missing.");

    FAgentSpatialMetrics MetricsValue;
    MetricsValue.Update(ChannelsValue);
    Check(!MetricsValue.Valid, Success, "Metrics should stay invalid when channels could not be extracted.");

    return Success;
}

bool TestIntentArbitrationAndValidation()
{
    bool Success = true;

    Unit WorkerUnit = MakeUnit(30, UNIT_TYPEID::TERRAN_SCV, Unit::Alliance::Self, Point2D(10.0f, 10.0f));
    Unit MarineUnit = MakeUnit(31, UNIT_TYPEID::TERRAN_MARINE, Unit::Alliance::Self, Point2D(14.0f, 14.0f));

    FTerranUnitContainer Container;
    Container.SetUnits({&WorkerUnit, &MarineUnit});

    FakeObservation ObservationValue;
    ObservationValue.SetUnits({&WorkerUnit, &MarineUnit});

    FakeQuery QueryValue;
    QueryValue.PlacementResult = true;
    QueryValue.UnitPathingResult = 6.0f;

    const FFrameContext FrameValue = FFrameContext::Create(&ObservationValue, &QueryValue, 6);

    FIntentBuffer BufferValue;
    BufferValue.Add(FUnitIntent::CreatePointTarget(WorkerUnit.tag, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                   Point2D(100.0f, 100.0f), 100,
                                                   EIntentDomain::StructureBuild, false, true));
    BufferValue.Add(FUnitIntent::CreatePointTarget(WorkerUnit.tag, ABILITY_ID::ATTACK_ATTACK, Point2D(20.0f, 20.0f),
                                                   100, EIntentDomain::Recovery, true));
    BufferValue.Add(FUnitIntent::CreatePointTarget(MarineUnit.tag, ABILITY_ID::ATTACK_ATTACK, Point2D(50.0f, 50.0f),
                                                   50, EIntentDomain::ArmyCombat, true));

    FIntentArbiter ArbiterValue;
    const std::vector<FUnitIntent> ResolvedIntents = ArbiterValue.Resolve(FrameValue, Container, BufferValue);

    Check(ResolvedIntents.size() == 2, Success, "Arbitration should resolve exactly one winning intent per actor.");

    const FUnitIntent* WorkerIntent = nullptr;
    const FUnitIntent* MarineIntent = nullptr;
    for (const FUnitIntent& IntentValue : ResolvedIntents)
    {
        if (IntentValue.ActorTag == WorkerUnit.tag)
        {
            WorkerIntent = &IntentValue;
        }
        if (IntentValue.ActorTag == MarineUnit.tag)
        {
            MarineIntent = &IntentValue;
        }
    }

    Check(WorkerIntent != nullptr, Success, "Worker winner should survive arbitration.");
    Check(WorkerIntent && WorkerIntent->Domain == EIntentDomain::Recovery, Success,
          "Equal-priority ties should resolve by producer registration order.");
    Check(MarineIntent != nullptr, Success, "Marine winner should survive arbitration.");
    Check(MarineIntent && MarineIntent->Ability == ABILITY_ID::ATTACK_ATTACK, Success,
          "Independent actors should keep their own winning intents.");

    FIntentBuffer BuildConflictBuffer;
    BuildConflictBuffer.Add(FUnitIntent::CreatePointTarget(WorkerUnit.tag, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                           Point2D(80.0f, 80.0f), 100,
                                                           EIntentDomain::StructureBuild, false, true));
    BuildConflictBuffer.Add(FUnitIntent::CreatePointTarget(WorkerUnit.tag, ABILITY_ID::BUILD_BARRACKS,
                                                           Point2D(70.0f, 70.0f), 100,
                                                           EIntentDomain::StructureBuild, false, true));

    const std::vector<FUnitIntent> BuildResolved = ArbiterValue.Resolve(FrameValue, Container, BuildConflictBuffer);
    Check(BuildResolved.size() == 1, Success,
          "A worker should not receive multiple winning structure-build intents in the same frame.");
    Check(BuildResolved.size() == 1 && BuildResolved.front().Ability == ABILITY_ID::BUILD_SUPPLYDEPOT, Success,
          "Same-domain ties should preserve the earlier registered structure intent.");
    Check(BuildResolved.size() == 1 && ApproxEqual(BuildResolved.front().TargetPoint.x, 63.0f) &&
              ApproxEqual(BuildResolved.front().TargetPoint.y, 63.0f),
          Success, "Resolved point targets should be clamped to playable space.");

    QueryValue.PlacementResult = false;
    const std::vector<FUnitIntent> PlacementRejected = ArbiterValue.Resolve(FrameValue, Container, BuildConflictBuffer);
    Check(PlacementRejected.empty(), Success,
          "Placement validation should drop invalid structure-build intents before execution.");

    QueryValue.PlacementResult = true;
    QueryValue.UnitPathingResult = -1.0f;

    FIntentBuffer PathingBuffer;
    PathingBuffer.Add(FUnitIntent::CreatePointTarget(MarineUnit.tag, ABILITY_ID::ATTACK_ATTACK, Point2D(40.0f, 40.0f),
                                                     50, EIntentDomain::ArmyCombat, true));

    const std::vector<FUnitIntent> PathingRejected = ArbiterValue.Resolve(FrameValue, Container, PathingBuffer);
    Check(PathingRejected.empty(), Success,
          "Pathing validation should drop unreachable point-target intents before execution.");

    return Success;
}

}  // namespace

bool TestSingularityFramework(int ArgC, char** ArgV)
{
    (void)ArgC;
    (void)ArgV;

    bool Success = true;

    std::cout << "  Checking Singularity container sync..." << std::endl;
    Success = TestContainerResetAndSync() && Success;

    std::cout << "  Checking Singularity state counting..." << std::endl;
    Success = TestBuildingCountsAndConstructionTracking() && Success;

    std::cout << "  Checking Singularity spatial channels..." << std::endl;
    Success = TestSpatialChannelsAndMetrics() && Success;

    std::cout << "  Checking Singularity intent arbitration..." << std::endl;
    Success = TestIntentArbitrationAndValidation() && Success;

    return Success;
}

}  // namespace sc2
