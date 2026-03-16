#include "test_terran_economy_production_order_expander.h"

#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "common/agent_framework.h"
#include "common/bot_status_models.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/planning/ECommandAuthorityLayer.h"
#include "common/planning/ECommandOrderDeferralReason.h"
#include "common/planning/ECommandTaskType.h"
#include "common/planning/EOrderLifecycleState.h"
#include "common/planning/FCommandOrderRecord.h"
#include "common/planning/FTerranEconomyProductionOrderExpander.h"
#include "common/services/EBuildPlacementFootprintPolicy.h"
#include "common/services/FTerranBuildPlacementService.h"
#include "sc2api/sc2_api.h"
#include "sc2api/sc2_score.h"
#include "sc2api/sc2_unit_filters.h"

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

struct FakeQuery : QueryInterface
{
    bool RejectWorkerSpecificCommandCenterPlacementValue = false;

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

    float PathingDistance(const Point2D& StartPointValue, const Point2D& EndPointValue) override
    {
        (void)StartPointValue;
        (void)EndPointValue;
        return 1.0f;
    }

    float PathingDistance(const Unit* StartUnitValue, const Point2D& EndPointValue) override
    {
        (void)StartUnitValue;
        (void)EndPointValue;
        return 1.0f;
    }

    std::vector<float> PathingDistance(const std::vector<PathingQuery>& QueriesValue) override
    {
        return std::vector<float>(QueriesValue.size(), 1.0f);
    }

    bool Placement(const AbilityID& AbilityIdValue, const Point2D& TargetPointValue,
                   const Unit* UnitPtr = nullptr) override
    {
        (void)TargetPointValue;
        if (RejectWorkerSpecificCommandCenterPlacementValue &&
            AbilityIdValue == ABILITY_ID::BUILD_COMMANDCENTER &&
            UnitPtr != nullptr)
        {
            return false;
        }

        return true;
    }

    std::vector<bool> Placement(const std::vector<PlacementQuery>& QueriesValue) override
    {
        return std::vector<bool>(QueriesValue.size(), true);
    }
};

struct FakeObservation : ObservationInterface
{
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
    uint32_t FoodCapValue = 46U;
    uint32_t FoodUsedValue = 20U;
    uint32_t FoodArmyValue = 0U;
    uint32_t FoodWorkersValue = 0U;
    uint32_t IdleWorkerCountValue = 0U;
    uint32_t ArmyCountValue = 0U;
    uint32_t WarpGateCountValue = 0U;
    uint32_t LarvaCountValue = 0U;
    Point2D CameraPositionValue;
    Point3D StartLocationValue;
    std::vector<PlayerResult> ResultsValue;
    SC2APIProtocol::Observation RawObservationValue;

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

        GameInfoValue.pathing_grid.width = GameInfoValue.width;
        GameInfoValue.pathing_grid.height = GameInfoValue.height;
        GameInfoValue.pathing_grid.bits_per_pixel = 8;
        GameInfoValue.pathing_grid.data.assign(static_cast<size_t>(GameInfoValue.width * GameInfoValue.height),
                                               static_cast<char>(0));

        GameInfoValue.placement_grid.width = GameInfoValue.width;
        GameInfoValue.placement_grid.height = GameInfoValue.height;
        GameInfoValue.placement_grid.bits_per_pixel = 8;
        GameInfoValue.placement_grid.data.assign(static_cast<size_t>(GameInfoValue.width * GameInfoValue.height),
                                                 static_cast<char>(255));

        GameInfoValue.terrain_height.width = GameInfoValue.width;
        GameInfoValue.terrain_height.height = GameInfoValue.height;
        GameInfoValue.terrain_height.bits_per_pixel = 8;
        GameInfoValue.terrain_height.data.assign(static_cast<size_t>(GameInfoValue.width * GameInfoValue.height),
                                                 static_cast<char>(127));
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

    uint32_t GetPlayerID() const override
    {
        return PlayerIdValue;
    }

    uint32_t GetGameLoop() const override
    {
        return GameLoopValue;
    }

    Units GetUnits() const override
    {
        return AllUnitsValue;
    }

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
        const std::unordered_map<Tag, const Unit*>::const_iterator FoundUnitIteratorValue =
            UnitsByTagValue.find(TagValue);
        return FoundUnitIteratorValue == UnitsByTagValue.end() ? nullptr : FoundUnitIteratorValue->second;
    }

    const RawActions& GetRawActions() const override
    {
        return RawActionsValue;
    }

    const SpatialActions& GetFeatureLayerActions() const override
    {
        return FeatureLayerActionsValue;
    }

    const SpatialActions& GetRenderedActions() const override
    {
        return RenderedActionsValue;
    }

    const std::vector<ChatMessage>& GetChatMessages() const override
    {
        return ChatMessagesValue;
    }

    const std::vector<PowerSource>& GetPowerSources() const override
    {
        return PowerSourcesValue;
    }

    const std::vector<Effect>& GetEffects() const override
    {
        return EffectsValue;
    }

    const std::vector<UpgradeID>& GetUpgrades() const override
    {
        return UpgradesValue;
    }

    const Score& GetScore() const override
    {
        return ScoreValue;
    }

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

    const GameInfo& GetGameInfo() const override
    {
        return GameInfoValue;
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
        return FoodCapValue;
    }

    uint32_t GetFoodUsed() const override
    {
        return FoodUsedValue;
    }

    uint32_t GetFoodArmy() const override
    {
        return FoodArmyValue;
    }

    uint32_t GetFoodWorkers() const override
    {
        return FoodWorkersValue;
    }

    uint32_t GetIdleWorkerCount() const override
    {
        return IdleWorkerCountValue;
    }

    uint32_t GetArmyCount() const override
    {
        return ArmyCountValue;
    }

    uint32_t GetWarpGateCount() const override
    {
        return WarpGateCountValue;
    }

    uint32_t GetLarvaCount() const override
    {
        return LarvaCountValue;
    }

    Point2D GetCameraPos() const override
    {
        return CameraPositionValue;
    }

    Point3D GetStartLocation() const override
    {
        return StartLocationValue;
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

uint32_t CountActiveChildOrders(const FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue,
                                const uint32_t ParentOrderIdValue, const ECommandAuthorityLayer SourceLayerValue)
{
    uint32_t ActiveChildOrderCountValue = 0U;
    for (size_t OrderIndexValue = 0U; OrderIndexValue < CommandAuthoritySchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (CommandAuthoritySchedulingStateValue.ParentOrderIds[OrderIndexValue] != ParentOrderIdValue ||
            CommandAuthoritySchedulingStateValue.SourceLayers[OrderIndexValue] != SourceLayerValue ||
            IsTerminalLifecycleState(CommandAuthoritySchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        ++ActiveChildOrderCountValue;
    }

    return ActiveChildOrderCountValue;
}

}  // namespace

bool TestTerranEconomyProductionOrderExpander(int ArgC, char** ArgV)
{
    (void)ArgC;
    (void)ArgV;

    bool SuccessValue = true;

    std::vector<Unit> UnitStorageValue;
    UnitStorageValue.push_back(
        MakeUnit(101U, UNIT_TYPEID::TERRAN_BARRACKS, Unit::Alliance::Self, Point2D(20.0f, 20.0f), true));
    UnitStorageValue.push_back(MakeUnit(201U, UNIT_TYPEID::TERRAN_BARRACKSREACTOR, Unit::Alliance::Self,
                                        Point2D(22.5f, 19.5f), true));
    UnitStorageValue[0].add_on_tag = 201U;

    Units UnitPointersValue;
    AppendUnitPointers(UnitStorageValue, UnitPointersValue);

    FakeObservation ObservationValue;
    ObservationValue.SetUnits(UnitPointersValue);
    FakeQuery QueryValue;
    const FFrameContext FrameValue = FFrameContext::Create(&ObservationValue, &QueryValue, 1U);

    FAgentState AgentStateValue;
    AgentStateValue.Update(FrameValue);

    FGameStateDescriptor GameStateDescriptorValue;
    GameStateDescriptorValue.CurrentStep = 1U;
    GameStateDescriptorValue.CurrentGameLoop = 1U;
    GameStateDescriptorValue.BuildPlanning.AvailableMinerals = 500U;
    GameStateDescriptorValue.BuildPlanning.AvailableVespene = 0U;
    GameStateDescriptorValue.BuildPlanning.AvailableSupply = 20U;

    FCommandAuthoritySchedulingState& CommandAuthoritySchedulingStateValue =
        GameStateDescriptorValue.CommandAuthoritySchedulingState;

    FCommandOrderRecord ExistingCommittedMarineOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::UnitExecution, 101U, ABILITY_ID::TRAIN_MARINE, 150,
        EIntentDomain::UnitProduction, 1U, 0U, 900U);
    ExistingCommittedMarineOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKS;
    ExistingCommittedMarineOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_MARINE;
    CommandAuthoritySchedulingStateValue.EnqueueOrder(ExistingCommittedMarineOrderValue);

    FCommandOrderRecord MarineEconomyOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::EconomyAndProduction, NullTag, ABILITY_ID::TRAIN_MARINE, 150,
        EIntentDomain::UnitProduction, 1U);
    MarineEconomyOrderValue.TargetCount = 10U;
    MarineEconomyOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKS;
    MarineEconomyOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_MARINE;
    const uint32_t MarineEconomyOrderIdValue =
        CommandAuthoritySchedulingStateValue.EnqueueOrder(MarineEconomyOrderValue);

    FTerranEconomyProductionOrderExpander EconomyProductionOrderExpanderValue;
    FTerranBuildPlacementService BuildPlacementServiceValue;
    FIntentBuffer IntentBufferValue;
    const std::vector<Point2D> ExpansionLocationsValue;

    EconomyProductionOrderExpanderValue.ExpandEconomyAndProductionOrders(
        FrameValue, AgentStateValue, GameStateDescriptorValue, IntentBufferValue, BuildPlacementServiceValue,
        ExpansionLocationsValue);

    size_t MarineChildOrderIndexValue = 0U;
    Check(CommandAuthoritySchedulingStateValue.TryGetActiveChildOrderIndex(
              MarineEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution, MarineChildOrderIndexValue),
          SuccessValue, "A reactor-backed barracks should accept a second committed marine order while one slot is in use.");
    const FCommandOrderRecord MarineChildOrderValue =
        CommandAuthoritySchedulingStateValue.GetOrderRecord(MarineChildOrderIndexValue);
    Check(MarineChildOrderValue.ActorTag == 101U, SuccessValue,
          "The second committed marine order should target the same reactor-backed barracks actor.");
    Check(MarineChildOrderValue.AbilityId == ABILITY_ID::TRAIN_MARINE, SuccessValue,
          "The created child order should preserve the marine training ability.");
    Check(MarineChildOrderValue.ProducerUnitTypeId == UNIT_TYPEID::TERRAN_BARRACKS, SuccessValue,
          "The created child order should preserve the producer unit type.");
    Check(MarineChildOrderValue.ResultUnitTypeId == UNIT_TYPEID::TERRAN_MARINE, SuccessValue,
          "The created child order should preserve the result unit type.");

    FGameStateDescriptor ReactorQueueGameStateDescriptorValue;
    ReactorQueueGameStateDescriptorValue.CurrentStep = 2U;
    ReactorQueueGameStateDescriptorValue.CurrentGameLoop = 2U;
    ReactorQueueGameStateDescriptorValue.BuildPlanning.AvailableMinerals = 500U;
    ReactorQueueGameStateDescriptorValue.BuildPlanning.AvailableVespene = 0U;
    ReactorQueueGameStateDescriptorValue.BuildPlanning.AvailableSupply = 20U;

    FCommandAuthoritySchedulingState& ReactorQueueSchedulingStateValue =
        ReactorQueueGameStateDescriptorValue.CommandAuthoritySchedulingState;
    FCommandOrderRecord ReactorQueueEconomyOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::EconomyAndProduction, NullTag, ABILITY_ID::TRAIN_MARINE, 150,
        EIntentDomain::UnitProduction, 2U);
    ReactorQueueEconomyOrderValue.TargetCount = 10U;
    ReactorQueueEconomyOrderValue.RequestedQueueCount = 2U;
    ReactorQueueEconomyOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKS;
    ReactorQueueEconomyOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_MARINE;
    const uint32_t ReactorQueueEconomyOrderIdValue =
        ReactorQueueSchedulingStateValue.EnqueueOrder(ReactorQueueEconomyOrderValue);

    FCommandOrderRecord ExistingReactorQueueChildOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::UnitExecution, 101U, ABILITY_ID::TRAIN_MARINE, 150,
        EIntentDomain::UnitProduction, 2U, 0U, ReactorQueueEconomyOrderIdValue);
    ExistingReactorQueueChildOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKS;
    ExistingReactorQueueChildOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_MARINE;
    ReactorQueueSchedulingStateValue.EnqueueOrder(ExistingReactorQueueChildOrderValue);

    FIntentBuffer ReactorQueueIntentBufferValue;
    EconomyProductionOrderExpanderValue.ExpandEconomyAndProductionOrders(
        FrameValue, AgentStateValue, ReactorQueueGameStateDescriptorValue, ReactorQueueIntentBufferValue,
        BuildPlacementServiceValue, ExpansionLocationsValue);

    Check(CountActiveChildOrders(ReactorQueueSchedulingStateValue, ReactorQueueEconomyOrderIdValue,
                                 ECommandAuthorityLayer::UnitExecution) == 2U,
          SuccessValue,
          "A unit-production economy order with requested queue count two should keep filling a reactor-backed queue.");

    std::vector<Unit> ReactorStarportUnitStorageValue;
    ReactorStarportUnitStorageValue.push_back(
        MakeUnit(121U, UNIT_TYPEID::TERRAN_STARPORT, Unit::Alliance::Self, Point2D(20.0f, 20.0f), true));
    ReactorStarportUnitStorageValue.push_back(MakeUnit(122U, UNIT_TYPEID::TERRAN_STARPORTREACTOR,
                                                       Unit::Alliance::Self, Point2D(22.5f, 19.5f), true));
    ReactorStarportUnitStorageValue[0].add_on_tag = 122U;

    Units ReactorStarportUnitPointersValue;
    AppendUnitPointers(ReactorStarportUnitStorageValue, ReactorStarportUnitPointersValue);

    FakeObservation ReactorStarportObservationValue;
    ReactorStarportObservationValue.SetUnits(ReactorStarportUnitPointersValue);
    ReactorStarportObservationValue.MineralsValue = 500U;
    ReactorStarportObservationValue.VespeneValue = 300U;
    FakeQuery ReactorStarportQueryValue;
    const FFrameContext ReactorStarportFrameValue =
        FFrameContext::Create(&ReactorStarportObservationValue, &ReactorStarportQueryValue, 3U);

    FAgentState ReactorStarportAgentStateValue;
    ReactorStarportAgentStateValue.Update(ReactorStarportFrameValue);

    FGameStateDescriptor ReactorStarportGameStateDescriptorValue;
    ReactorStarportGameStateDescriptorValue.CurrentStep = 3U;
    ReactorStarportGameStateDescriptorValue.CurrentGameLoop = 3U;
    ReactorStarportGameStateDescriptorValue.BuildPlanning.AvailableMinerals = 500U;
    ReactorStarportGameStateDescriptorValue.BuildPlanning.AvailableVespene = 300U;
    ReactorStarportGameStateDescriptorValue.BuildPlanning.AvailableSupply = 20U;

    FCommandAuthoritySchedulingState& ReactorStarportSchedulingStateValue =
        ReactorStarportGameStateDescriptorValue.CommandAuthoritySchedulingState;
    FCommandOrderRecord ReactorStarportEconomyOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::EconomyAndProduction, NullTag, ABILITY_ID::TRAIN_MEDIVAC, 150,
        EIntentDomain::UnitProduction, 3U);
    ReactorStarportEconomyOrderValue.TargetCount = 6U;
    ReactorStarportEconomyOrderValue.RequestedQueueCount = 2U;
    ReactorStarportEconomyOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_STARPORT;
    ReactorStarportEconomyOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_MEDIVAC;
    const uint32_t ReactorStarportEconomyOrderIdValue =
        ReactorStarportSchedulingStateValue.EnqueueOrder(ReactorStarportEconomyOrderValue);

    FCommandOrderRecord ExistingReactorStarportChildOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::UnitExecution, 121U, ABILITY_ID::TRAIN_MEDIVAC, 150,
        EIntentDomain::UnitProduction, 3U, 0U, ReactorStarportEconomyOrderIdValue);
    ExistingReactorStarportChildOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_STARPORT;
    ExistingReactorStarportChildOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_MEDIVAC;
    ReactorStarportSchedulingStateValue.EnqueueOrder(ExistingReactorStarportChildOrderValue);

    FIntentBuffer ReactorStarportIntentBufferValue;
    EconomyProductionOrderExpanderValue.ExpandEconomyAndProductionOrders(
        ReactorStarportFrameValue, ReactorStarportAgentStateValue, ReactorStarportGameStateDescriptorValue,
        ReactorStarportIntentBufferValue, BuildPlacementServiceValue, ExpansionLocationsValue);

    Check(CountActiveChildOrders(ReactorStarportSchedulingStateValue, ReactorStarportEconomyOrderIdValue,
                                 ECommandAuthorityLayer::UnitExecution) == 2U,
          SuccessValue,
          "A reactor-backed starport should keep filling both queue slots for a medivac order.");

    std::vector<Unit> FactoryTankUnitStorageValue;
    FactoryTankUnitStorageValue.push_back(
        MakeUnit(131U, UNIT_TYPEID::TERRAN_FACTORY, Unit::Alliance::Self, Point2D(20.0f, 20.0f), true));
    FactoryTankUnitStorageValue.push_back(
        MakeUnit(132U, UNIT_TYPEID::TERRAN_FACTORYTECHLAB, Unit::Alliance::Self, Point2D(22.5f, 19.5f), true));
    FactoryTankUnitStorageValue.push_back(
        MakeUnit(133U, UNIT_TYPEID::TERRAN_FACTORY, Unit::Alliance::Self, Point2D(28.0f, 20.0f), true));
    FactoryTankUnitStorageValue.push_back(
        MakeUnit(134U, UNIT_TYPEID::TERRAN_FACTORYTECHLAB, Unit::Alliance::Self, Point2D(30.5f, 19.5f), true));
    FactoryTankUnitStorageValue[0].add_on_tag = 132U;
    FactoryTankUnitStorageValue[2].add_on_tag = 134U;

    Units FactoryTankUnitPointersValue;
    AppendUnitPointers(FactoryTankUnitStorageValue, FactoryTankUnitPointersValue);

    FakeObservation FactoryTankObservationValue;
    FactoryTankObservationValue.SetUnits(FactoryTankUnitPointersValue);
    FactoryTankObservationValue.MineralsValue = 500U;
    FactoryTankObservationValue.VespeneValue = 400U;
    FakeQuery FactoryTankQueryValue;
    const FFrameContext FactoryTankFrameValue =
        FFrameContext::Create(&FactoryTankObservationValue, &FactoryTankQueryValue, 4U);

    FAgentState FactoryTankAgentStateValue;
    FactoryTankAgentStateValue.Update(FactoryTankFrameValue);

    FGameStateDescriptor FactoryTankGameStateDescriptorValue;
    FactoryTankGameStateDescriptorValue.CurrentStep = 4U;
    FactoryTankGameStateDescriptorValue.CurrentGameLoop = 4U;
    FactoryTankGameStateDescriptorValue.BuildPlanning.AvailableMinerals = 500U;
    FactoryTankGameStateDescriptorValue.BuildPlanning.AvailableVespene = 400U;
    FactoryTankGameStateDescriptorValue.BuildPlanning.AvailableSupply = 20U;

    FCommandAuthoritySchedulingState& FactoryTankSchedulingStateValue =
        FactoryTankGameStateDescriptorValue.CommandAuthoritySchedulingState;
    FCommandOrderRecord FactoryTankEconomyOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::EconomyAndProduction, NullTag, ABILITY_ID::TRAIN_SIEGETANK, 150,
        EIntentDomain::UnitProduction, 4U);
    FactoryTankEconomyOrderValue.TargetCount = 2U;
    FactoryTankEconomyOrderValue.RequestedQueueCount = 2U;
    FactoryTankEconomyOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_FACTORY;
    FactoryTankEconomyOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_SIEGETANK;
    const uint32_t FactoryTankEconomyOrderIdValue =
        FactoryTankSchedulingStateValue.EnqueueOrder(FactoryTankEconomyOrderValue);

    FIntentBuffer FactoryTankIntentBufferValue;
    EconomyProductionOrderExpanderValue.ExpandEconomyAndProductionOrders(
        FactoryTankFrameValue, FactoryTankAgentStateValue, FactoryTankGameStateDescriptorValue,
        FactoryTankIntentBufferValue, BuildPlacementServiceValue, ExpansionLocationsValue);

    Check(CountActiveChildOrders(FactoryTankSchedulingStateValue, FactoryTankEconomyOrderIdValue,
                                 ECommandAuthorityLayer::UnitExecution) == 2U,
          SuccessValue,
          "A requested queue count of two should let one siege-tank order fan out across two idle tech-lab factories.");

    std::vector<Unit> WorkerTownHallUnitStorageValue;
    WorkerTownHallUnitStorageValue.push_back(
        MakeUnit(211U, UNIT_TYPEID::TERRAN_COMMANDCENTER, Unit::Alliance::Self, Point2D(10.0f, 10.0f), true));
    WorkerTownHallUnitStorageValue.push_back(
        MakeUnit(212U, UNIT_TYPEID::TERRAN_ORBITALCOMMAND, Unit::Alliance::Self, Point2D(24.0f, 10.0f), true));

    Units WorkerTownHallUnitPointersValue;
    AppendUnitPointers(WorkerTownHallUnitStorageValue, WorkerTownHallUnitPointersValue);

    FakeObservation WorkerTownHallObservationValue;
    WorkerTownHallObservationValue.SetUnits(WorkerTownHallUnitPointersValue);
    WorkerTownHallObservationValue.MineralsValue = 300U;
    WorkerTownHallObservationValue.FoodCapValue = 60U;
    WorkerTownHallObservationValue.FoodUsedValue = 20U;

    FakeQuery WorkerTownHallQueryValue;
    const FFrameContext WorkerTownHallFrameValue =
        FFrameContext::Create(&WorkerTownHallObservationValue, &WorkerTownHallQueryValue, 2U);

    FAgentState WorkerTownHallAgentStateValue;
    WorkerTownHallAgentStateValue.Update(WorkerTownHallFrameValue);

    FGameStateDescriptor WorkerTownHallGameStateDescriptorValue;
    WorkerTownHallGameStateDescriptorValue.CurrentStep = 2U;
    WorkerTownHallGameStateDescriptorValue.CurrentGameLoop = 2U;
    WorkerTownHallGameStateDescriptorValue.BuildPlanning.AvailableMinerals = 300U;
    WorkerTownHallGameStateDescriptorValue.BuildPlanning.AvailableVespene = 0U;
    WorkerTownHallGameStateDescriptorValue.BuildPlanning.AvailableSupply = 20U;
    WorkerTownHallGameStateDescriptorValue.BuildPlanning.ObservedTownHallCount = 2U;
    WorkerTownHallGameStateDescriptorValue.BuildPlanning.ObservedOrbitalCommandCount = 1U;
    WorkerTownHallGameStateDescriptorValue.EconomyState.ProjectedAvailableMineralsByHorizon[
        ShortForecastHorizonIndexValue] = 300U;
    WorkerTownHallGameStateDescriptorValue.EconomyState.ProjectedAvailableSupplyByHorizon[
        ShortForecastHorizonIndexValue] = 20U;

    FCommandAuthoritySchedulingState& WorkerTownHallSchedulingStateValue =
        WorkerTownHallGameStateDescriptorValue.CommandAuthoritySchedulingState;
    FCommandOrderRecord WorkerEconomyOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::EconomyAndProduction, NullTag, ABILITY_ID::TRAIN_SCV, 175,
        EIntentDomain::UnitProduction, 2U);
    WorkerEconomyOrderValue.TaskType = ECommandTaskType::WorkerProduction;
    WorkerEconomyOrderValue.TargetCount = 28U;
    WorkerEconomyOrderValue.RequestedQueueCount = 2U;
    WorkerEconomyOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_COMMANDCENTER;
    WorkerEconomyOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
    const uint32_t WorkerEconomyOrderIdValue =
        WorkerTownHallSchedulingStateValue.EnqueueOrder(WorkerEconomyOrderValue);

    IntentBufferValue.Reset();
    EconomyProductionOrderExpanderValue.ExpandEconomyAndProductionOrders(
        WorkerTownHallFrameValue, WorkerTownHallAgentStateValue, WorkerTownHallGameStateDescriptorValue,
        IntentBufferValue, BuildPlacementServiceValue, ExpansionLocationsValue);

    Check(CountActiveChildOrders(WorkerTownHallSchedulingStateValue, WorkerEconomyOrderIdValue,
                                 ECommandAuthorityLayer::UnitExecution) == 2U,
          SuccessValue,
          "Worker-production economy orders should create one SCV child per eligible command center or orbital.");

    bool HasCommandCenterWorkerOrderValue = false;
    bool HasOrbitalWorkerOrderValue = false;
    for (size_t OrderIndexValue = 0U; OrderIndexValue < WorkerTownHallSchedulingStateValue.OrderIds.size();
         ++OrderIndexValue)
    {
        if (WorkerTownHallSchedulingStateValue.ParentOrderIds[OrderIndexValue] != WorkerEconomyOrderIdValue ||
            WorkerTownHallSchedulingStateValue.SourceLayers[OrderIndexValue] != ECommandAuthorityLayer::UnitExecution ||
            IsTerminalLifecycleState(WorkerTownHallSchedulingStateValue.LifecycleStates[OrderIndexValue]))
        {
            continue;
        }

        if (WorkerTownHallSchedulingStateValue.ActorTags[OrderIndexValue] == 211U)
        {
            HasCommandCenterWorkerOrderValue = true;
        }
        if (WorkerTownHallSchedulingStateValue.ActorTags[OrderIndexValue] == 212U)
        {
            HasOrbitalWorkerOrderValue = true;
        }
    }

    Check(HasCommandCenterWorkerOrderValue, SuccessValue,
          "Worker-production child orders should include the idle command center.");
    Check(HasOrbitalWorkerOrderValue, SuccessValue,
          "Worker-production child orders should include the idle orbital command.");

    std::vector<Unit> WallBarracksUnitStorageValue;
    WallBarracksUnitStorageValue.push_back(
        MakeUnit(301U, UNIT_TYPEID::TERRAN_SCV, Unit::Alliance::Self, Point2D(57.0f, 52.0f), false));
    WallBarracksUnitStorageValue.push_back(MakeUnit(302U, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, Unit::Alliance::Self,
                                                    Point2D(61.0f, 54.0f), true));

    Units WallBarracksUnitPointersValue;
    AppendUnitPointers(WallBarracksUnitStorageValue, WallBarracksUnitPointersValue);

    FakeObservation WallBarracksObservationValue;
    WallBarracksObservationValue.SetUnits(WallBarracksUnitPointersValue);
    FakeQuery WallBarracksQueryValue;
    const FFrameContext WallBarracksFrameValue =
        FFrameContext::Create(&WallBarracksObservationValue, &WallBarracksQueryValue, 2U);

    FAgentState WallBarracksAgentStateValue;
    WallBarracksAgentStateValue.Update(WallBarracksFrameValue);

    FGameStateDescriptor WallBarracksGameStateDescriptorValue;
    WallBarracksGameStateDescriptorValue.CurrentStep = 2U;
    WallBarracksGameStateDescriptorValue.CurrentGameLoop = 2U;
    WallBarracksGameStateDescriptorValue.BuildPlanning.AvailableMinerals = 500U;
    WallBarracksGameStateDescriptorValue.BuildPlanning.AvailableVespene = 0U;
    WallBarracksGameStateDescriptorValue.BuildPlanning.AvailableSupply = 20U;
    WallBarracksGameStateDescriptorValue.RampWallDescriptor.bIsValid = true;
    WallBarracksGameStateDescriptorValue.RampWallDescriptor.LeftDepotSlot.SlotId.SlotType =
        EBuildPlacementSlotType::MainRampDepotLeft;
    WallBarracksGameStateDescriptorValue.RampWallDescriptor.LeftDepotSlot.FootprintPolicy =
        EBuildPlacementFootprintPolicy::StructureOnly;
    WallBarracksGameStateDescriptorValue.RampWallDescriptor.LeftDepotSlot.BuildPoint = Point2D(61.0f, 54.0f);
    WallBarracksGameStateDescriptorValue.RampWallDescriptor.BarracksSlot.SlotId.SlotType =
        EBuildPlacementSlotType::MainRampBarracksWithAddon;
    WallBarracksGameStateDescriptorValue.RampWallDescriptor.BarracksSlot.FootprintPolicy =
        EBuildPlacementFootprintPolicy::RequiresAddonClearance;
    WallBarracksGameStateDescriptorValue.RampWallDescriptor.BarracksSlot.BuildPoint = Point2D(58.0f, 52.0f);
    WallBarracksGameStateDescriptorValue.RampWallDescriptor.RightDepotSlot.SlotId.SlotType =
        EBuildPlacementSlotType::MainRampDepotRight;
    WallBarracksGameStateDescriptorValue.RampWallDescriptor.RightDepotSlot.FootprintPolicy =
        EBuildPlacementFootprintPolicy::StructureOnly;
    WallBarracksGameStateDescriptorValue.RampWallDescriptor.RightDepotSlot.BuildPoint = Point2D(61.0f, 50.0f);

    FCommandAuthoritySchedulingState& WallBarracksSchedulingStateValue =
        WallBarracksGameStateDescriptorValue.CommandAuthoritySchedulingState;
    FCommandOrderRecord WallBarracksEconomyOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::EconomyAndProduction, NullTag, ABILITY_ID::BUILD_BARRACKS, 160,
        EIntentDomain::StructureBuild, 2U);
    WallBarracksEconomyOrderValue.TargetCount = 1U;
    WallBarracksEconomyOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
    WallBarracksEconomyOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKS;
    WallBarracksEconomyOrderValue.PreferredPlacementSlotType =
        EBuildPlacementSlotType::MainRampBarracksWithAddon;
    const uint32_t WallBarracksEconomyOrderIdValue =
        WallBarracksSchedulingStateValue.EnqueueOrder(WallBarracksEconomyOrderValue);

    const std::vector<Point2D> WallBarracksExpansionLocationsValue =
    {
        Point2D(20.0f, 10.0f),
    };

    EconomyProductionOrderExpanderValue.ExpandEconomyAndProductionOrders(
        WallBarracksFrameValue, WallBarracksAgentStateValue, WallBarracksGameStateDescriptorValue,
        IntentBufferValue, BuildPlacementServiceValue, WallBarracksExpansionLocationsValue);

    size_t WallBarracksChildOrderIndexValue = 0U;
    Check(WallBarracksSchedulingStateValue.TryGetActiveChildOrderIndex(
              WallBarracksEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution,
              WallBarracksChildOrderIndexValue),
          SuccessValue,
          "A wall barracks order should remain placeable after the first wall depot is already present.");
    if (WallBarracksSchedulingStateValue.TryGetActiveChildOrderIndex(
            WallBarracksEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution,
            WallBarracksChildOrderIndexValue))
    {
        const FCommandOrderRecord WallBarracksChildOrderValue =
            WallBarracksSchedulingStateValue.GetOrderRecord(WallBarracksChildOrderIndexValue);
        Check(WallBarracksChildOrderValue.TargetPoint ==
                  WallBarracksGameStateDescriptorValue.RampWallDescriptor.BarracksSlot.BuildPoint,
              SuccessValue, "The wall barracks child order should preserve the reserved ramp barracks slot.");
    }

    std::vector<Unit> WallDepotUnitStorageValue;
    WallDepotUnitStorageValue.push_back(
        MakeUnit(311U, UNIT_TYPEID::TERRAN_SCV, Unit::Alliance::Self, Point2D(57.0f, 52.0f), false));
    WallDepotUnitStorageValue.push_back(MakeUnit(312U, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, Unit::Alliance::Self,
                                                 Point2D(68.0f, 60.0f), true));

    Units WallDepotUnitPointersValue;
    AppendUnitPointers(WallDepotUnitStorageValue, WallDepotUnitPointersValue);

    FakeObservation WallDepotObservationValue;
    WallDepotObservationValue.SetUnits(WallDepotUnitPointersValue);
    FakeQuery WallDepotQueryValue;
    const FFrameContext WallDepotFrameValue = FFrameContext::Create(&WallDepotObservationValue,
                                                                    &WallDepotQueryValue, 3U);

    FAgentState WallDepotAgentStateValue;
    WallDepotAgentStateValue.Update(WallDepotFrameValue);

    FGameStateDescriptor WallDepotGameStateDescriptorValue;
    WallDepotGameStateDescriptorValue.CurrentStep = 3U;
    WallDepotGameStateDescriptorValue.CurrentGameLoop = 3U;
    WallDepotGameStateDescriptorValue.BuildPlanning.AvailableMinerals = 500U;
    WallDepotGameStateDescriptorValue.BuildPlanning.AvailableVespene = 0U;
    WallDepotGameStateDescriptorValue.BuildPlanning.AvailableSupply = 20U;
    WallDepotGameStateDescriptorValue.RampWallDescriptor.bIsValid = true;
    WallDepotGameStateDescriptorValue.RampWallDescriptor.LeftDepotSlot.SlotId.SlotType =
        EBuildPlacementSlotType::MainRampDepotLeft;
    WallDepotGameStateDescriptorValue.RampWallDescriptor.LeftDepotSlot.FootprintPolicy =
        EBuildPlacementFootprintPolicy::StructureOnly;
    WallDepotGameStateDescriptorValue.RampWallDescriptor.LeftDepotSlot.BuildPoint = Point2D(61.0f, 54.0f);
    WallDepotGameStateDescriptorValue.RampWallDescriptor.BarracksSlot.SlotId.SlotType =
        EBuildPlacementSlotType::MainRampBarracksWithAddon;
    WallDepotGameStateDescriptorValue.RampWallDescriptor.BarracksSlot.FootprintPolicy =
        EBuildPlacementFootprintPolicy::RequiresAddonClearance;
    WallDepotGameStateDescriptorValue.RampWallDescriptor.BarracksSlot.BuildPoint = Point2D(58.0f, 52.0f);
    WallDepotGameStateDescriptorValue.RampWallDescriptor.RightDepotSlot.SlotId.SlotType =
        EBuildPlacementSlotType::MainRampDepotRight;
    WallDepotGameStateDescriptorValue.RampWallDescriptor.RightDepotSlot.FootprintPolicy =
        EBuildPlacementFootprintPolicy::StructureOnly;
    WallDepotGameStateDescriptorValue.RampWallDescriptor.RightDepotSlot.BuildPoint = Point2D(61.0f, 50.0f);

    FCommandAuthoritySchedulingState& WallDepotSchedulingStateValue =
        WallDepotGameStateDescriptorValue.CommandAuthoritySchedulingState;
    FCommandOrderRecord WallDepotEconomyOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::EconomyAndProduction, NullTag, ABILITY_ID::BUILD_SUPPLYDEPOT, 160,
        EIntentDomain::StructureBuild, 3U);
    WallDepotEconomyOrderValue.TargetCount = 1U;
    WallDepotEconomyOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
    WallDepotEconomyOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_SUPPLYDEPOT;
    WallDepotEconomyOrderValue.PreferredPlacementSlotType = EBuildPlacementSlotType::MainRampDepotLeft;
    const uint32_t WallDepotEconomyOrderIdValue =
        WallDepotSchedulingStateValue.EnqueueOrder(WallDepotEconomyOrderValue);

    const std::vector<Point2D> WallDepotExpansionLocationsValue =
    {
        Point2D(20.0f, 10.0f),
    };

    EconomyProductionOrderExpanderValue.ExpandEconomyAndProductionOrders(
        WallDepotFrameValue, WallDepotAgentStateValue, WallDepotGameStateDescriptorValue,
        IntentBufferValue, BuildPlacementServiceValue, WallDepotExpansionLocationsValue);

    size_t WallDepotChildOrderIndexValue = 0U;
    Check(WallDepotSchedulingStateValue.TryGetActiveChildOrderIndex(
              WallDepotEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution,
              WallDepotChildOrderIndexValue),
          SuccessValue,
          "A wall depot order should remain placeable when another completed depot exists away from the ramp wall.");
    if (WallDepotSchedulingStateValue.TryGetActiveChildOrderIndex(
            WallDepotEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution,
            WallDepotChildOrderIndexValue))
    {
        const FCommandOrderRecord WallDepotChildOrderValue =
            WallDepotSchedulingStateValue.GetOrderRecord(WallDepotChildOrderIndexValue);
        Check(WallDepotChildOrderValue.TargetPoint ==
                  WallDepotGameStateDescriptorValue.RampWallDescriptor.LeftDepotSlot.BuildPoint,
              SuccessValue,
              "The wall depot child order should target the exact ramp wall slot instead of treating aggregate depot count as satisfied.");
    }

    std::vector<Unit> StaleWallDepotUnitStorageValue;
    StaleWallDepotUnitStorageValue.push_back(
        MakeUnit(601U, UNIT_TYPEID::TERRAN_SCV, Unit::Alliance::Self, Point2D(57.0f, 52.0f), false));
    UnitOrder StaleWallDepotBuildOrderValue;
    StaleWallDepotBuildOrderValue.ability_id = ABILITY_ID::BUILD_SUPPLYDEPOT;
    StaleWallDepotBuildOrderValue.target_pos = Point2D(61.0f, 54.0f);
    StaleWallDepotUnitStorageValue[0].orders.push_back(StaleWallDepotBuildOrderValue);

    Units StaleWallDepotUnitPointersValue;
    AppendUnitPointers(StaleWallDepotUnitStorageValue, StaleWallDepotUnitPointersValue);

    FakeObservation StaleWallDepotObservationValue;
    StaleWallDepotObservationValue.GameLoopValue = 400U;
    StaleWallDepotObservationValue.SetUnits(StaleWallDepotUnitPointersValue);
    FakeQuery StaleWallDepotQueryValue;
    const FFrameContext StaleWallDepotFrameValue =
        FFrameContext::Create(&StaleWallDepotObservationValue, &StaleWallDepotQueryValue, 4U);

    FAgentState StaleWallDepotAgentStateValue;
    StaleWallDepotAgentStateValue.Update(StaleWallDepotFrameValue);

    FGameStateDescriptor StaleWallDepotGameStateDescriptorValue;
    StaleWallDepotGameStateDescriptorValue.CurrentStep = 400U;
    StaleWallDepotGameStateDescriptorValue.CurrentGameLoop = 400U;
    StaleWallDepotGameStateDescriptorValue.BuildPlanning.AvailableMinerals = 500U;
    StaleWallDepotGameStateDescriptorValue.BuildPlanning.AvailableVespene = 0U;
    StaleWallDepotGameStateDescriptorValue.BuildPlanning.AvailableSupply = 20U;
    StaleWallDepotGameStateDescriptorValue.RampWallDescriptor.bIsValid = true;
    StaleWallDepotGameStateDescriptorValue.RampWallDescriptor.LeftDepotSlot.SlotId.SlotType =
        EBuildPlacementSlotType::MainRampDepotLeft;
    StaleWallDepotGameStateDescriptorValue.RampWallDescriptor.LeftDepotSlot.FootprintPolicy =
        EBuildPlacementFootprintPolicy::StructureOnly;
    StaleWallDepotGameStateDescriptorValue.RampWallDescriptor.LeftDepotSlot.BuildPoint = Point2D(61.0f, 54.0f);
    StaleWallDepotGameStateDescriptorValue.RampWallDescriptor.BarracksSlot.SlotId.SlotType =
        EBuildPlacementSlotType::MainRampBarracksWithAddon;
    StaleWallDepotGameStateDescriptorValue.RampWallDescriptor.BarracksSlot.FootprintPolicy =
        EBuildPlacementFootprintPolicy::RequiresAddonClearance;
    StaleWallDepotGameStateDescriptorValue.RampWallDescriptor.BarracksSlot.BuildPoint = Point2D(58.0f, 52.0f);
    StaleWallDepotGameStateDescriptorValue.RampWallDescriptor.RightDepotSlot.SlotId.SlotType =
        EBuildPlacementSlotType::MainRampDepotRight;
    StaleWallDepotGameStateDescriptorValue.RampWallDescriptor.RightDepotSlot.FootprintPolicy =
        EBuildPlacementFootprintPolicy::StructureOnly;
    StaleWallDepotGameStateDescriptorValue.RampWallDescriptor.RightDepotSlot.BuildPoint = Point2D(61.0f, 50.0f);

    FCommandAuthoritySchedulingState& StaleWallDepotSchedulingStateValue =
        StaleWallDepotGameStateDescriptorValue.CommandAuthoritySchedulingState;
    FCommandOrderRecord StaleWallDepotEconomyOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::EconomyAndProduction, NullTag, ABILITY_ID::BUILD_SUPPLYDEPOT, 160,
        EIntentDomain::StructureBuild, 400U);
    StaleWallDepotEconomyOrderValue.TargetCount = 1U;
    StaleWallDepotEconomyOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
    StaleWallDepotEconomyOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_SUPPLYDEPOT;
    StaleWallDepotEconomyOrderValue.PreferredPlacementSlotType = EBuildPlacementSlotType::MainRampDepotLeft;
    const uint32_t StaleWallDepotEconomyOrderIdValue =
        StaleWallDepotSchedulingStateValue.EnqueueOrder(StaleWallDepotEconomyOrderValue);
    StaleWallDepotSchedulingStateValue.SetOrderReservedPlacementSlot(
        StaleWallDepotEconomyOrderIdValue,
        StaleWallDepotGameStateDescriptorValue.RampWallDescriptor.LeftDepotSlot.SlotId);
    StaleWallDepotSchedulingStateValue.SetOrderDeferralState(
        StaleWallDepotEconomyOrderIdValue, ECommandOrderDeferralReason::AwaitingObservedCompletion, 400U, 400U);

    FCommandOrderRecord StaleWallDepotCompletedChildOrderValue = FCommandOrderRecord::CreatePointTarget(
        ECommandAuthorityLayer::UnitExecution, 601U, ABILITY_ID::BUILD_SUPPLYDEPOT,
        StaleWallDepotGameStateDescriptorValue.RampWallDescriptor.LeftDepotSlot.BuildPoint, 160,
        EIntentDomain::StructureBuild, 200U, 0U, StaleWallDepotEconomyOrderIdValue, -1, -1, false, true, false);
    StaleWallDepotCompletedChildOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
    StaleWallDepotCompletedChildOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_SUPPLYDEPOT;
    const uint32_t StaleWallDepotCompletedChildOrderIdValue =
        StaleWallDepotSchedulingStateValue.EnqueueOrder(StaleWallDepotCompletedChildOrderValue);
    StaleWallDepotSchedulingStateValue.SetOrderDispatchState(StaleWallDepotCompletedChildOrderIdValue, 200U, 200U, 0U,
                                                             0U);
    StaleWallDepotSchedulingStateValue.SetOrderLifecycleState(StaleWallDepotCompletedChildOrderIdValue,
                                                              EOrderLifecycleState::Completed);

    const std::vector<Point2D> StaleWallDepotExpansionLocationsValue =
    {
        Point2D(20.0f, 10.0f),
    };

    EconomyProductionOrderExpanderValue.ExpandEconomyAndProductionOrders(
        StaleWallDepotFrameValue, StaleWallDepotAgentStateValue, StaleWallDepotGameStateDescriptorValue,
        IntentBufferValue, BuildPlacementServiceValue, StaleWallDepotExpansionLocationsValue);

    size_t StaleWallDepotReplacementChildOrderIndexValue = 0U;
    Check(StaleWallDepotSchedulingStateValue.TryGetActiveChildOrderIndex(
              StaleWallDepotEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution,
              StaleWallDepotReplacementChildOrderIndexValue),
          SuccessValue,
          "A stale reserved wall-slot build order should respawn a new child once the grace window expires.");
    if (StaleWallDepotSchedulingStateValue.TryGetActiveChildOrderIndex(
            StaleWallDepotEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution,
            StaleWallDepotReplacementChildOrderIndexValue))
    {
        const FCommandOrderRecord StaleWallDepotReplacementChildOrderValue =
            StaleWallDepotSchedulingStateValue.GetOrderRecord(StaleWallDepotReplacementChildOrderIndexValue);
        Check(StaleWallDepotReplacementChildOrderValue.TargetPoint ==
                  StaleWallDepotGameStateDescriptorValue.RampWallDescriptor.LeftDepotSlot.BuildPoint,
              SuccessValue,
              "The replacement wall depot child order should keep the original reserved ramp slot.");
        Check(StaleWallDepotReplacementChildOrderValue.ReservedPlacementSlotId ==
                  StaleWallDepotGameStateDescriptorValue.RampWallDescriptor.LeftDepotSlot.SlotId,
              SuccessValue,
              "The replacement wall depot child order should preserve the reserved placement slot id.");
    }

    std::vector<Unit> ProductionRailUnitStorageValue;
    ProductionRailUnitStorageValue.push_back(
        MakeUnit(701U, UNIT_TYPEID::TERRAN_SCV, Unit::Alliance::Self, Point2D(12.0f, 10.0f), false));
    ProductionRailUnitStorageValue.push_back(
        MakeUnit(702U, UNIT_TYPEID::TERRAN_SCV, Unit::Alliance::Self, Point2D(14.0f, 10.0f), false));

    Units ProductionRailUnitPointersValue;
    AppendUnitPointers(ProductionRailUnitStorageValue, ProductionRailUnitPointersValue);

    FakeObservation ProductionRailObservationValue;
    ProductionRailObservationValue.SetUnits(ProductionRailUnitPointersValue);
    FakeQuery ProductionRailQueryValue;
    const FFrameContext ProductionRailFrameValue =
        FFrameContext::Create(&ProductionRailObservationValue, &ProductionRailQueryValue, 5U);

    FAgentState ProductionRailAgentStateValue;
    ProductionRailAgentStateValue.Update(ProductionRailFrameValue);

    FGameStateDescriptor ProductionRailGameStateDescriptorValue;
    ProductionRailGameStateDescriptorValue.CurrentStep = 5U;
    ProductionRailGameStateDescriptorValue.CurrentGameLoop = 5U;
    ProductionRailGameStateDescriptorValue.BuildPlanning.AvailableMinerals = 500U;
    ProductionRailGameStateDescriptorValue.BuildPlanning.AvailableVespene = 100U;
    ProductionRailGameStateDescriptorValue.BuildPlanning.AvailableSupply = 20U;
    ProductionRailGameStateDescriptorValue.MainBaseLayoutDescriptor.bIsValid = true;

    FBuildPlacementSlot ProductionRailBarracksSlotValue;
    ProductionRailBarracksSlotValue.SlotId.SlotType = EBuildPlacementSlotType::MainProductionWithAddon;
    ProductionRailBarracksSlotValue.SlotId.Ordinal = 0U;
    ProductionRailBarracksSlotValue.FootprintPolicy = EBuildPlacementFootprintPolicy::RequiresAddonClearance;
    ProductionRailBarracksSlotValue.BuildPoint = Point2D(18.0f, 14.0f);
    ProductionRailGameStateDescriptorValue.MainBaseLayoutDescriptor.ProductionRailWithAddonSlots.push_back(
        ProductionRailBarracksSlotValue);

    FBuildPlacementSlot ProductionRailFactorySlotValue;
    ProductionRailFactorySlotValue.SlotId.SlotType = EBuildPlacementSlotType::MainProductionWithAddon;
    ProductionRailFactorySlotValue.SlotId.Ordinal = 1U;
    ProductionRailFactorySlotValue.FootprintPolicy = EBuildPlacementFootprintPolicy::RequiresAddonClearance;
    ProductionRailFactorySlotValue.BuildPoint = Point2D(22.0f, 18.0f);
    ProductionRailGameStateDescriptorValue.MainBaseLayoutDescriptor.ProductionRailWithAddonSlots.push_back(
        ProductionRailFactorySlotValue);

    FBuildPlacementSlot ProductionRailStarportSlotValue;
    ProductionRailStarportSlotValue.SlotId.SlotType = EBuildPlacementSlotType::MainProductionWithAddon;
    ProductionRailStarportSlotValue.SlotId.Ordinal = 2U;
    ProductionRailStarportSlotValue.FootprintPolicy = EBuildPlacementFootprintPolicy::RequiresAddonClearance;
    ProductionRailStarportSlotValue.BuildPoint = Point2D(26.0f, 22.0f);
    ProductionRailGameStateDescriptorValue.MainBaseLayoutDescriptor.ProductionRailWithAddonSlots.push_back(
        ProductionRailStarportSlotValue);

    FBuildPlacementSlot FallbackFactorySlotValue;
    FallbackFactorySlotValue.SlotId.SlotType = EBuildPlacementSlotType::MainFactoryWithAddon;
    FallbackFactorySlotValue.SlotId.Ordinal = 0U;
    FallbackFactorySlotValue.FootprintPolicy = EBuildPlacementFootprintPolicy::RequiresAddonClearance;
    FallbackFactorySlotValue.BuildPoint = Point2D(30.0f, 18.0f);
    ProductionRailGameStateDescriptorValue.MainBaseLayoutDescriptor.FactoryWithAddonSlots.push_back(
        FallbackFactorySlotValue);

    FCommandAuthoritySchedulingState& ProductionRailSchedulingStateValue =
        ProductionRailGameStateDescriptorValue.CommandAuthoritySchedulingState;
    FCommandOrderRecord ProductionRailFactoryEconomyOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::EconomyAndProduction, NullTag, ABILITY_ID::BUILD_FACTORY, 170,
        EIntentDomain::StructureBuild, 5U);
    ProductionRailFactoryEconomyOrderValue.TargetCount = 1U;
    ProductionRailFactoryEconomyOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
    ProductionRailFactoryEconomyOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_FACTORY;
    ProductionRailFactoryEconomyOrderValue.PreferredPlacementSlotType =
        EBuildPlacementSlotType::MainProductionWithAddon;
    ProductionRailFactoryEconomyOrderValue.PreferredPlacementSlotId = ProductionRailFactorySlotValue.SlotId;
    const uint32_t ProductionRailFactoryEconomyOrderIdValue =
        ProductionRailSchedulingStateValue.EnqueueOrder(ProductionRailFactoryEconomyOrderValue);

    const std::vector<Point2D> ProductionRailExpansionLocationsValue =
    {
        Point2D(20.0f, 10.0f),
    };

    IntentBufferValue.Reset();
    EconomyProductionOrderExpanderValue.ExpandEconomyAndProductionOrders(
        ProductionRailFrameValue, ProductionRailAgentStateValue, ProductionRailGameStateDescriptorValue,
        IntentBufferValue, BuildPlacementServiceValue, ProductionRailExpansionLocationsValue);

    size_t ProductionRailChildOrderIndexValue = 0U;
    Check(ProductionRailSchedulingStateValue.TryGetActiveChildOrderIndex(
              ProductionRailFactoryEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution,
              ProductionRailChildOrderIndexValue),
          SuccessValue, "A factory order with an exact preferred rail slot id should create a child on that rail pad.");
    if (ProductionRailSchedulingStateValue.TryGetActiveChildOrderIndex(
            ProductionRailFactoryEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution,
            ProductionRailChildOrderIndexValue))
    {
        const FCommandOrderRecord ProductionRailChildOrderValue =
            ProductionRailSchedulingStateValue.GetOrderRecord(ProductionRailChildOrderIndexValue);
        Check(ProductionRailChildOrderValue.TargetPoint == ProductionRailFactorySlotValue.BuildPoint,
              SuccessValue, "The factory child order should target the authored factory rail pad.");
        Check(ProductionRailChildOrderValue.ReservedPlacementSlotId == ProductionRailFactorySlotValue.SlotId,
              SuccessValue, "The factory child order should preserve the exact reserved rail slot id.");
    }

    FGameStateDescriptor OccupiedRailGameStateDescriptorValue = ProductionRailGameStateDescriptorValue;
    OccupiedRailGameStateDescriptorValue.CommandAuthoritySchedulingState.Reset();
    OccupiedRailGameStateDescriptorValue.CurrentStep = 6U;
    OccupiedRailGameStateDescriptorValue.CurrentGameLoop = 6U;
    OccupiedRailGameStateDescriptorValue.BuildPlanning.AvailableMinerals = 500U;
    OccupiedRailGameStateDescriptorValue.BuildPlanning.AvailableVespene = 100U;
    OccupiedRailGameStateDescriptorValue.BuildPlanning.AvailableSupply = 20U;
    OccupiedRailGameStateDescriptorValue.BuildPlanning.ReservedMinerals = 0U;
    OccupiedRailGameStateDescriptorValue.BuildPlanning.ReservedVespene = 0U;
    OccupiedRailGameStateDescriptorValue.BuildPlanning.ReservedSupply = 0U;

    FCommandAuthoritySchedulingState& OccupiedRailSchedulingStateValue =
        OccupiedRailGameStateDescriptorValue.CommandAuthoritySchedulingState;
    FCommandOrderRecord BlockingRailOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::StrategicDirector, NullTag, ABILITY_ID::BUILD_BARRACKS, 180,
        EIntentDomain::StructureBuild, 6U);
    BlockingRailOrderValue.TargetCount = 1U;
    BlockingRailOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
    BlockingRailOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKS;
    BlockingRailOrderValue.ReservedPlacementSlotId = ProductionRailFactorySlotValue.SlotId;
    const uint32_t BlockingRailOrderIdValue = OccupiedRailSchedulingStateValue.EnqueueOrder(BlockingRailOrderValue);
    (void)BlockingRailOrderIdValue;

    FCommandOrderRecord BlockingLeftRailOrderValue = BlockingRailOrderValue;
    BlockingLeftRailOrderValue.ReservedPlacementSlotId = ProductionRailBarracksSlotValue.SlotId;
    OccupiedRailSchedulingStateValue.EnqueueOrder(BlockingLeftRailOrderValue);

    FCommandOrderRecord BlockingRightRailOrderValue = BlockingRailOrderValue;
    BlockingRightRailOrderValue.ReservedPlacementSlotId = ProductionRailStarportSlotValue.SlotId;
    OccupiedRailSchedulingStateValue.EnqueueOrder(BlockingRightRailOrderValue);

    FCommandOrderRecord OccupiedRailFactoryEconomyOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::EconomyAndProduction, NullTag, ABILITY_ID::BUILD_FACTORY, 170,
        EIntentDomain::StructureBuild, 6U);
    OccupiedRailFactoryEconomyOrderValue.TargetCount = 1U;
    OccupiedRailFactoryEconomyOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
    OccupiedRailFactoryEconomyOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_FACTORY;
    OccupiedRailFactoryEconomyOrderValue.PreferredPlacementSlotType =
        EBuildPlacementSlotType::MainProductionWithAddon;
    OccupiedRailFactoryEconomyOrderValue.PreferredPlacementSlotId = ProductionRailFactorySlotValue.SlotId;
    const uint32_t OccupiedRailFactoryEconomyOrderIdValue =
        OccupiedRailSchedulingStateValue.EnqueueOrder(OccupiedRailFactoryEconomyOrderValue);

    IntentBufferValue.Reset();
    EconomyProductionOrderExpanderValue.ExpandEconomyAndProductionOrders(
        ProductionRailFrameValue, ProductionRailAgentStateValue, OccupiedRailGameStateDescriptorValue,
        IntentBufferValue, BuildPlacementServiceValue, ProductionRailExpansionLocationsValue);

    Check(!OccupiedRailSchedulingStateValue.TryGetActiveChildOrderIndex(
              OccupiedRailFactoryEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution,
              ProductionRailChildOrderIndexValue),
          SuccessValue,
          "An exact preferred rail slot id should defer when its authored pad is claimed instead of drifting to another rail slot.");
    size_t OccupiedRailFactoryEconomyOrderIndexValue = 0U;
    Check(OccupiedRailSchedulingStateValue.TryGetOrderIndex(
              OccupiedRailFactoryEconomyOrderIdValue, OccupiedRailFactoryEconomyOrderIndexValue),
          SuccessValue, "The deferred exact preferred rail order should remain addressable in scheduling state.");
    if (OccupiedRailSchedulingStateValue.TryGetOrderIndex(
            OccupiedRailFactoryEconomyOrderIdValue, OccupiedRailFactoryEconomyOrderIndexValue))
    {
        const FCommandOrderRecord OccupiedRailFactoryEconomyOrderRecordValue =
            OccupiedRailSchedulingStateValue.GetOrderRecord(OccupiedRailFactoryEconomyOrderIndexValue);
        Check(OccupiedRailFactoryEconomyOrderRecordValue.LastDeferralReason ==
                  ECommandOrderDeferralReason::ReservedSlotOccupied,
              SuccessValue,
              "A claimed exact preferred rail slot should defer with ReservedSlotOccupied instead of drifting.");
    }

    FCommandOrderRecord PreferredRailOnlyFactoryEconomyOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::EconomyAndProduction, NullTag, ABILITY_ID::BUILD_FACTORY, 170,
        EIntentDomain::StructureBuild, 6U);
    PreferredRailOnlyFactoryEconomyOrderValue.TargetCount = 1U;
    PreferredRailOnlyFactoryEconomyOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
    PreferredRailOnlyFactoryEconomyOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_FACTORY;
    PreferredRailOnlyFactoryEconomyOrderValue.PreferredPlacementSlotType =
        EBuildPlacementSlotType::MainProductionWithAddon;
    const uint32_t PreferredRailOnlyFactoryEconomyOrderIdValue =
        OccupiedRailSchedulingStateValue.EnqueueOrder(PreferredRailOnlyFactoryEconomyOrderValue);

    IntentBufferValue.Reset();
    EconomyProductionOrderExpanderValue.ExpandEconomyAndProductionOrders(
        ProductionRailFrameValue, ProductionRailAgentStateValue, OccupiedRailGameStateDescriptorValue,
        IntentBufferValue, BuildPlacementServiceValue, ProductionRailExpansionLocationsValue);

    Check(OccupiedRailSchedulingStateValue.TryGetActiveChildOrderIndex(
              PreferredRailOnlyFactoryEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution,
              ProductionRailChildOrderIndexValue),
          SuccessValue,
          "A non-exact preferred production-rail factory order should fall through to later factory slots when the rail is full.");
    if (OccupiedRailSchedulingStateValue.TryGetActiveChildOrderIndex(
            PreferredRailOnlyFactoryEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution,
            ProductionRailChildOrderIndexValue))
    {
        const FCommandOrderRecord PreferredRailOnlyFactoryChildOrderValue =
            OccupiedRailSchedulingStateValue.GetOrderRecord(ProductionRailChildOrderIndexValue);
        Check(PreferredRailOnlyFactoryChildOrderValue.TargetPoint == FallbackFactorySlotValue.BuildPoint,
              SuccessValue,
              "A non-exact preferred production-rail factory order should relocate to the authored factory fallback slot.");
        Check(PreferredRailOnlyFactoryChildOrderValue.ReservedPlacementSlotId == FallbackFactorySlotValue.SlotId,
              SuccessValue,
              "A relocated factory order should reserve the factory fallback slot instead of drifting without a slot id.");
    }

    std::vector<Unit> ProtectedAddonLaneUnitStorageValue;
    ProtectedAddonLaneUnitStorageValue.push_back(
        MakeUnit(711U, UNIT_TYPEID::TERRAN_FACTORY, Unit::Alliance::Self, Point2D(20.0f, 20.0f), true));
    ProtectedAddonLaneUnitStorageValue.push_back(
        MakeUnit(712U, UNIT_TYPEID::TERRAN_SCV, Unit::Alliance::Self, Point2D(14.0f, 10.0f), false));

    Units ProtectedAddonLaneUnitPointersValue;
    AppendUnitPointers(ProtectedAddonLaneUnitStorageValue, ProtectedAddonLaneUnitPointersValue);

    FakeObservation ProtectedAddonLaneObservationValue;
    ProtectedAddonLaneObservationValue.GameLoopValue = 7U;
    ProtectedAddonLaneObservationValue.SetUnits(ProtectedAddonLaneUnitPointersValue);
    FakeQuery ProtectedAddonLaneQueryValue;
    const FFrameContext ProtectedAddonLaneFrameValue =
        FFrameContext::Create(&ProtectedAddonLaneObservationValue, &ProtectedAddonLaneQueryValue, 7U);

    FAgentState ProtectedAddonLaneAgentStateValue;
    ProtectedAddonLaneAgentStateValue.Update(ProtectedAddonLaneFrameValue);

    FGameStateDescriptor ProtectedAddonLaneGameStateDescriptorValue;
    ProtectedAddonLaneGameStateDescriptorValue.CurrentStep = 7U;
    ProtectedAddonLaneGameStateDescriptorValue.CurrentGameLoop = 7U;
    ProtectedAddonLaneGameStateDescriptorValue.BuildPlanning.AvailableMinerals = 500U;
    ProtectedAddonLaneGameStateDescriptorValue.BuildPlanning.AvailableVespene = 100U;
    ProtectedAddonLaneGameStateDescriptorValue.BuildPlanning.AvailableSupply = 20U;
    ProtectedAddonLaneGameStateDescriptorValue.MainBaseLayoutDescriptor.bIsValid = true;

    FBuildPlacementSlot BlockingStarportPlacementSlotValue;
    BlockingStarportPlacementSlotValue.SlotId.SlotType = EBuildPlacementSlotType::MainStarportWithAddon;
    BlockingStarportPlacementSlotValue.SlotId.Ordinal = 0U;
    BlockingStarportPlacementSlotValue.FootprintPolicy = EBuildPlacementFootprintPolicy::RequiresAddonClearance;
    BlockingStarportPlacementSlotValue.BuildPoint = Point2D(21.5f, 20.0f);
    ProtectedAddonLaneGameStateDescriptorValue.MainBaseLayoutDescriptor.StarportWithAddonSlots.push_back(
        BlockingStarportPlacementSlotValue);

    FBuildPlacementSlot SafeStarportPlacementSlotValue;
    SafeStarportPlacementSlotValue.SlotId.SlotType = EBuildPlacementSlotType::MainStarportWithAddon;
    SafeStarportPlacementSlotValue.SlotId.Ordinal = 1U;
    SafeStarportPlacementSlotValue.FootprintPolicy = EBuildPlacementFootprintPolicy::RequiresAddonClearance;
    SafeStarportPlacementSlotValue.BuildPoint = Point2D(28.0f, 24.0f);
    ProtectedAddonLaneGameStateDescriptorValue.MainBaseLayoutDescriptor.StarportWithAddonSlots.push_back(
        SafeStarportPlacementSlotValue);

    FCommandAuthoritySchedulingState& ProtectedAddonLaneSchedulingStateValue =
        ProtectedAddonLaneGameStateDescriptorValue.CommandAuthoritySchedulingState;
    FCommandOrderRecord ProtectedAddonLaneStarportEconomyOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::EconomyAndProduction, NullTag, ABILITY_ID::BUILD_STARPORT, 170,
        EIntentDomain::StructureBuild, 7U);
    ProtectedAddonLaneStarportEconomyOrderValue.TargetCount = 1U;
    ProtectedAddonLaneStarportEconomyOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
    ProtectedAddonLaneStarportEconomyOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_STARPORT;
    const uint32_t ProtectedAddonLaneStarportEconomyOrderIdValue =
        ProtectedAddonLaneSchedulingStateValue.EnqueueOrder(ProtectedAddonLaneStarportEconomyOrderValue);

    IntentBufferValue.Reset();
    EconomyProductionOrderExpanderValue.ExpandEconomyAndProductionOrders(
        ProtectedAddonLaneFrameValue, ProtectedAddonLaneAgentStateValue, ProtectedAddonLaneGameStateDescriptorValue,
        IntentBufferValue, BuildPlacementServiceValue, ProductionRailExpansionLocationsValue);

    size_t ProtectedAddonLaneChildOrderIndexValue = 0U;
    Check(ProtectedAddonLaneSchedulingStateValue.TryGetActiveChildOrderIndex(
              ProtectedAddonLaneStarportEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution,
              ProtectedAddonLaneChildOrderIndexValue),
          SuccessValue,
          "A starport order should still create a child when a later safe slot exists outside an observed factory add-on lane.");
    if (ProtectedAddonLaneSchedulingStateValue.TryGetActiveChildOrderIndex(
            ProtectedAddonLaneStarportEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution,
            ProtectedAddonLaneChildOrderIndexValue))
    {
        const FCommandOrderRecord ProtectedAddonLaneChildOrderValue =
            ProtectedAddonLaneSchedulingStateValue.GetOrderRecord(ProtectedAddonLaneChildOrderIndexValue);
        Check(ProtectedAddonLaneChildOrderValue.TargetPoint == SafeStarportPlacementSlotValue.BuildPoint,
              SuccessValue,
              "A starport order should skip a candidate that overlaps an observed factory add-on lane.");
        Check(ProtectedAddonLaneChildOrderValue.ReservedPlacementSlotId == SafeStarportPlacementSlotValue.SlotId,
              SuccessValue,
              "A starport order should reserve the safe authored starport slot after skipping the blocked add-on lane.");
    }

    std::vector<Unit> ProjectedSelfContributionUnitStorageValue;
    ProjectedSelfContributionUnitStorageValue.push_back(
        MakeUnit(703U, UNIT_TYPEID::TERRAN_SCV, Unit::Alliance::Self, Point2D(16.0f, 12.0f), false));

    Units ProjectedSelfContributionUnitPointersValue;
    AppendUnitPointers(ProjectedSelfContributionUnitStorageValue, ProjectedSelfContributionUnitPointersValue);

    FakeObservation ProjectedSelfContributionObservationValue;
    ProjectedSelfContributionObservationValue.GameLoopValue = 6U;
    ProjectedSelfContributionObservationValue.SetUnits(ProjectedSelfContributionUnitPointersValue);
    FakeQuery ProjectedSelfContributionQueryValue;
    const FFrameContext ProjectedSelfContributionFrameValue =
        FFrameContext::Create(&ProjectedSelfContributionObservationValue, &ProjectedSelfContributionQueryValue, 6U);

    FAgentState ProjectedSelfContributionAgentStateValue;
    ProjectedSelfContributionAgentStateValue.Update(ProjectedSelfContributionFrameValue);

    FGameStateDescriptor ProjectedSelfContributionGameStateDescriptorValue;
    ProjectedSelfContributionGameStateDescriptorValue.CurrentStep = 6U;
    ProjectedSelfContributionGameStateDescriptorValue.CurrentGameLoop = 6U;
    ProjectedSelfContributionGameStateDescriptorValue.BuildPlanning.AvailableMinerals = 500U;
    ProjectedSelfContributionGameStateDescriptorValue.BuildPlanning.AvailableVespene = 0U;
    ProjectedSelfContributionGameStateDescriptorValue.BuildPlanning.AvailableSupply = 20U;
    ProjectedSelfContributionGameStateDescriptorValue.MainBaseLayoutDescriptor.bIsValid = true;
    ProjectedSelfContributionGameStateDescriptorValue.MainBaseLayoutDescriptor.ProductionRailWithAddonSlots.push_back(
        ProductionRailBarracksSlotValue);
    ProjectedSelfContributionGameStateDescriptorValue.ProductionState.ProjectedBuildingCounts[
        GetTerranBuildingTypeIndex(UNIT_TYPEID::TERRAN_BARRACKS)] = 1U;

    FCommandAuthoritySchedulingState& ProjectedSelfContributionSchedulingStateValue =
        ProjectedSelfContributionGameStateDescriptorValue.CommandAuthoritySchedulingState;
    FCommandOrderRecord ProjectedSelfContributionEconomyOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::EconomyAndProduction, NullTag, ABILITY_ID::BUILD_BARRACKS, 170,
        EIntentDomain::StructureBuild, 6U);
    ProjectedSelfContributionEconomyOrderValue.TargetCount = 1U;
    ProjectedSelfContributionEconomyOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
    ProjectedSelfContributionEconomyOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKS;
    ProjectedSelfContributionEconomyOrderValue.PreferredPlacementSlotType =
        EBuildPlacementSlotType::MainProductionWithAddon;
    ProjectedSelfContributionEconomyOrderValue.PreferredPlacementSlotId = ProductionRailBarracksSlotValue.SlotId;
    const uint32_t ProjectedSelfContributionEconomyOrderIdValue =
        ProjectedSelfContributionSchedulingStateValue.EnqueueOrder(ProjectedSelfContributionEconomyOrderValue);

    IntentBufferValue.Reset();
    EconomyProductionOrderExpanderValue.ExpandEconomyAndProductionOrders(
        ProjectedSelfContributionFrameValue, ProjectedSelfContributionAgentStateValue,
        ProjectedSelfContributionGameStateDescriptorValue, IntentBufferValue, BuildPlacementServiceValue,
        ProductionRailExpansionLocationsValue);

    size_t ProjectedSelfContributionChildOrderIndexValue = 0U;
    Check(ProjectedSelfContributionSchedulingStateValue.TryGetActiveChildOrderIndex(
              ProjectedSelfContributionEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution,
              ProjectedSelfContributionChildOrderIndexValue),
          SuccessValue,
          "A queued structure order should ignore its own projected contribution so it can recreate a missing child.");
    if (ProjectedSelfContributionSchedulingStateValue.TryGetActiveChildOrderIndex(
            ProjectedSelfContributionEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution,
            ProjectedSelfContributionChildOrderIndexValue))
    {
        const FCommandOrderRecord ProjectedSelfContributionChildOrderValue =
            ProjectedSelfContributionSchedulingStateValue.GetOrderRecord(ProjectedSelfContributionChildOrderIndexValue);
        Check(ProjectedSelfContributionChildOrderValue.TargetPoint == ProductionRailBarracksSlotValue.BuildPoint,
              SuccessValue,
              "The recreated barracks child order should still target the authored production rail slot.");
    }

    std::vector<Unit> AddonBlockerUnitStorageValue;
    AddonBlockerUnitStorageValue.push_back(
        MakeUnit(901U, UNIT_TYPEID::TERRAN_BARRACKS, Unit::Alliance::Self, Point2D(20.0f, 20.0f), true));
    AddonBlockerUnitStorageValue.push_back(
        MakeUnit(902U, UNIT_TYPEID::TERRAN_MARINE, Unit::Alliance::Self, Point2D(22.5f, 19.5f), false));

    Units AddonBlockerUnitPointersValue;
    AppendUnitPointers(AddonBlockerUnitStorageValue, AddonBlockerUnitPointersValue);

    FakeObservation AddonBlockerObservationValue;
    AddonBlockerObservationValue.GameLoopValue = 700U;
    AddonBlockerObservationValue.SetUnits(AddonBlockerUnitPointersValue);
    FakeQuery AddonBlockerQueryValue;
    const FFrameContext AddonBlockerFrameValue =
        FFrameContext::Create(&AddonBlockerObservationValue, &AddonBlockerQueryValue, 7U);

    FAgentState AddonBlockerAgentStateValue;
    AddonBlockerAgentStateValue.Update(AddonBlockerFrameValue);

    FGameStateDescriptor AddonBlockerGameStateDescriptorValue;
    AddonBlockerGameStateDescriptorValue.CurrentStep = 7U;
    AddonBlockerGameStateDescriptorValue.CurrentGameLoop = 700U;
    AddonBlockerGameStateDescriptorValue.BuildPlanning.AvailableMinerals = 200U;
    AddonBlockerGameStateDescriptorValue.BuildPlanning.AvailableVespene = 100U;
    AddonBlockerGameStateDescriptorValue.BuildPlanning.AvailableSupply = 20U;
    AddonBlockerGameStateDescriptorValue.RampWallDescriptor.bIsValid = true;
    AddonBlockerGameStateDescriptorValue.RampWallDescriptor.BarracksSlot.SlotId.SlotType =
        EBuildPlacementSlotType::MainRampBarracksWithAddon;
    AddonBlockerGameStateDescriptorValue.RampWallDescriptor.BarracksSlot.SlotId.Ordinal = 0U;
    AddonBlockerGameStateDescriptorValue.RampWallDescriptor.BarracksSlot.FootprintPolicy =
        EBuildPlacementFootprintPolicy::RequiresAddonClearance;
    AddonBlockerGameStateDescriptorValue.RampWallDescriptor.BarracksSlot.BuildPoint = Point2D(20.0f, 20.0f);
    AddonBlockerGameStateDescriptorValue.MainBaseLayoutDescriptor.bIsValid = true;
    AddonBlockerGameStateDescriptorValue.MainBaseLayoutDescriptor.ProductionClearanceAnchorPoint =
        Point2D(12.0f, 12.0f);

    FCommandAuthoritySchedulingState& AddonBlockerSchedulingStateValue =
        AddonBlockerGameStateDescriptorValue.CommandAuthoritySchedulingState;
    FCommandOrderRecord AddonBlockerEconomyOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::EconomyAndProduction, NullTag, ABILITY_ID::BUILD_REACTOR_BARRACKS, 180,
        EIntentDomain::StructureBuild, 700U);
    AddonBlockerEconomyOrderValue.TargetCount = 1U;
    AddonBlockerEconomyOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKS;
    AddonBlockerEconomyOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKSREACTOR;
    AddonBlockerEconomyOrderValue.CommitmentClass = ECommandCommitmentClass::MandatoryOpening;
    AddonBlockerEconomyOrderValue.ExecutionGuarantee = ECommandTaskExecutionGuarantee::MustExecute;
    AddonBlockerEconomyOrderValue.PreferredProducerPlacementSlotId =
        AddonBlockerGameStateDescriptorValue.RampWallDescriptor.BarracksSlot.SlotId;
    const uint32_t AddonBlockerEconomyOrderIdValue =
        AddonBlockerSchedulingStateValue.EnqueueOrder(AddonBlockerEconomyOrderValue);

    IntentBufferValue.Reset();
    EconomyProductionOrderExpanderValue.ExpandEconomyAndProductionOrders(
        AddonBlockerFrameValue, AddonBlockerAgentStateValue, AddonBlockerGameStateDescriptorValue,
        IntentBufferValue, BuildPlacementServiceValue, ExpansionLocationsValue);

    Check(!AddonBlockerSchedulingStateValue.TryGetActiveChildOrderIndex(
              AddonBlockerEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution,
              ProductionRailChildOrderIndexValue),
          SuccessValue,
          "An add-on order with a friendly unit on the add-on footprint should defer until the blocker is cleared.");

    size_t AddonBlockerEconomyOrderIndexValue = 0U;
    Check(AddonBlockerSchedulingStateValue.TryGetOrderIndex(
              AddonBlockerEconomyOrderIdValue, AddonBlockerEconomyOrderIndexValue),
          SuccessValue, "The deferred add-on blocker order should remain addressable in scheduling state.");
    if (AddonBlockerSchedulingStateValue.TryGetOrderIndex(
            AddonBlockerEconomyOrderIdValue, AddonBlockerEconomyOrderIndexValue))
    {
        const FCommandOrderRecord AddonBlockerEconomyOrderRecordValue =
            AddonBlockerSchedulingStateValue.GetOrderRecord(AddonBlockerEconomyOrderIndexValue);
        Check(AddonBlockerEconomyOrderRecordValue.LastDeferralReason ==
                  ECommandOrderDeferralReason::ProducerBusy,
              SuccessValue,
              "A friendly unit blocking an add-on footprint should defer as ProducerBusy while blocker relief is attempted.");
    }

    bool HasMarineReliefIntentValue = false;
    for (const FUnitIntent& IntentValue : IntentBufferValue.Intents)
    {
        if (IntentValue.ActorTag != 902U || IntentValue.Ability != ABILITY_ID::GENERAL_MOVE)
        {
            continue;
        }

        HasMarineReliefIntentValue = true;
        Check(IntentValue.TargetPoint ==
                  AddonBlockerGameStateDescriptorValue.MainBaseLayoutDescriptor.ProductionClearanceAnchorPoint,
              SuccessValue,
              "A marine blocking an add-on footprint should be moved toward the main-side production-clearance anchor.");
        break;
    }

    Check(HasMarineReliefIntentValue, SuccessValue,
          "A marine blocking an add-on footprint should receive a recovery move intent to clear the addon space.");

    AddonBlockerObservationValue.GameLoopValue = 710U;
    AddonBlockerGameStateDescriptorValue.CurrentStep = 8U;
    AddonBlockerGameStateDescriptorValue.CurrentGameLoop = 710U;
    const FFrameContext AddonBlockerCooldownFrameValue =
        FFrameContext::Create(&AddonBlockerObservationValue, &AddonBlockerQueryValue, 8U);
    AddonBlockerAgentStateValue.Update(AddonBlockerCooldownFrameValue);
    IntentBufferValue.Reset();
    EconomyProductionOrderExpanderValue.ExpandEconomyAndProductionOrders(
        AddonBlockerCooldownFrameValue, AddonBlockerAgentStateValue, AddonBlockerGameStateDescriptorValue,
        IntentBufferValue, BuildPlacementServiceValue, ExpansionLocationsValue);

    bool HasCooldownReliefIntentValue = false;
    for (const FUnitIntent& IntentValue : IntentBufferValue.Intents)
    {
        if (IntentValue.ActorTag == 902U && IntentValue.Ability == ABILITY_ID::GENERAL_MOVE)
        {
            HasCooldownReliefIntentValue = true;
            break;
        }
    }

    Check(!HasCooldownReliefIntentValue, SuccessValue,
          "An add-on blocker relief cooldown should suppress duplicate recovery move intents before the retry window expires.");

    AddonBlockerObservationValue.GameLoopValue = 732U;
    AddonBlockerGameStateDescriptorValue.CurrentStep = 32U;
    AddonBlockerGameStateDescriptorValue.CurrentGameLoop = 732U;
    const FFrameContext AddonBlockerRetryFrameValue =
        FFrameContext::Create(&AddonBlockerObservationValue, &AddonBlockerQueryValue, 32U);
    AddonBlockerAgentStateValue.Update(AddonBlockerRetryFrameValue);
    IntentBufferValue.Reset();
    EconomyProductionOrderExpanderValue.ExpandEconomyAndProductionOrders(
        AddonBlockerRetryFrameValue, AddonBlockerAgentStateValue, AddonBlockerGameStateDescriptorValue,
        IntentBufferValue, BuildPlacementServiceValue, ExpansionLocationsValue);

    AddonBlockerObservationValue.GameLoopValue = 764U;
    AddonBlockerGameStateDescriptorValue.CurrentStep = 64U;
    AddonBlockerGameStateDescriptorValue.CurrentGameLoop = 764U;
    const FFrameContext AddonBlockerHardBlockFrameValue =
        FFrameContext::Create(&AddonBlockerObservationValue, &AddonBlockerQueryValue, 64U);
    AddonBlockerAgentStateValue.Update(AddonBlockerHardBlockFrameValue);
    IntentBufferValue.Reset();
    EconomyProductionOrderExpanderValue.ExpandEconomyAndProductionOrders(
        AddonBlockerHardBlockFrameValue, AddonBlockerAgentStateValue, AddonBlockerGameStateDescriptorValue,
        IntentBufferValue, BuildPlacementServiceValue, ExpansionLocationsValue);

    if (AddonBlockerSchedulingStateValue.TryGetOrderIndex(
            AddonBlockerEconomyOrderIdValue, AddonBlockerEconomyOrderIndexValue))
    {
        const FCommandOrderRecord AddonBlockerHardBlockOrderValue =
            AddonBlockerSchedulingStateValue.GetOrderRecord(AddonBlockerEconomyOrderIndexValue);
        Check(AddonBlockerHardBlockOrderValue.LastDeferralReason == ECommandOrderDeferralReason::NoValidPlacement,
              SuccessValue,
              "An add-on blocker that persists across three relief windows should promote to a hard placement block.");
    }

    std::vector<Unit> AddonClearUnitStorageValue;
    AddonClearUnitStorageValue.push_back(
        MakeUnit(901U, UNIT_TYPEID::TERRAN_BARRACKS, Unit::Alliance::Self, Point2D(20.0f, 20.0f), true));
    Units AddonClearUnitPointersValue;
    AppendUnitPointers(AddonClearUnitStorageValue, AddonClearUnitPointersValue);

    FakeObservation AddonClearObservationValue;
    AddonClearObservationValue.GameLoopValue = 740U;
    AddonClearObservationValue.SetUnits(AddonClearUnitPointersValue);
    FakeQuery AddonClearQueryValue;
    const FFrameContext AddonClearFrameValue = FFrameContext::Create(&AddonClearObservationValue, &AddonClearQueryValue, 40U);

    FAgentState AddonClearAgentStateValue;
    AddonClearAgentStateValue.Update(AddonClearFrameValue);

    FGameStateDescriptor AddonClearGameStateDescriptorValue;
    AddonClearGameStateDescriptorValue.CurrentStep = 40U;
    AddonClearGameStateDescriptorValue.CurrentGameLoop = 740U;
    AddonClearGameStateDescriptorValue.BuildPlanning.AvailableMinerals = 200U;
    AddonClearGameStateDescriptorValue.BuildPlanning.AvailableVespene = 100U;
    AddonClearGameStateDescriptorValue.BuildPlanning.AvailableSupply = 20U;
    AddonClearGameStateDescriptorValue.RampWallDescriptor =
        AddonBlockerGameStateDescriptorValue.RampWallDescriptor;
    AddonClearGameStateDescriptorValue.MainBaseLayoutDescriptor =
        AddonBlockerGameStateDescriptorValue.MainBaseLayoutDescriptor;
    FCommandOrderRecord AddonClearEconomyOrderValue = AddonBlockerEconomyOrderValue;
    AddonClearEconomyOrderValue.CreationStep = 40U;
    const uint32_t AddonClearEconomyOrderIdValue =
        AddonClearGameStateDescriptorValue.CommandAuthoritySchedulingState.EnqueueOrder(AddonClearEconomyOrderValue);

    FTerranEconomyProductionOrderExpander AddonClearEconomyProductionOrderExpanderValue;
    IntentBufferValue.Reset();
    AddonClearEconomyProductionOrderExpanderValue.ExpandEconomyAndProductionOrders(
        AddonClearFrameValue, AddonClearAgentStateValue, AddonClearGameStateDescriptorValue, IntentBufferValue,
        BuildPlacementServiceValue, ExpansionLocationsValue);

    size_t AddonClearChildOrderIndexValue = 0U;
    Check(AddonClearGameStateDescriptorValue.CommandAuthoritySchedulingState.TryGetActiveChildOrderIndex(
              AddonClearEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution, AddonClearChildOrderIndexValue),
          SuccessValue, "Once the footprint clears, the bound barracks add-on order should create a child order.");
    if (AddonClearGameStateDescriptorValue.CommandAuthoritySchedulingState.TryGetActiveChildOrderIndex(
            AddonClearEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution, AddonClearChildOrderIndexValue))
    {
        const FCommandOrderRecord AddonClearChildOrderValue =
            AddonClearGameStateDescriptorValue.CommandAuthoritySchedulingState.GetOrderRecord(AddonClearChildOrderIndexValue);
        Check(AddonClearChildOrderValue.ActorTag == 901U &&
                  AddonClearChildOrderValue.AbilityId == ABILITY_ID::BUILD_REACTOR_BARRACKS,
              SuccessValue,
              "Once the footprint clears, the add-on child order should dispatch from the bound wall barracks.");
    }

    std::vector<Unit> ReservedBarracksUnitStorageValue;
    ReservedBarracksUnitStorageValue.push_back(
        MakeUnit(951U, UNIT_TYPEID::TERRAN_BARRACKS, Unit::Alliance::Self, Point2D(20.0f, 20.0f), true));
    Units ReservedBarracksUnitPointersValue;
    AppendUnitPointers(ReservedBarracksUnitStorageValue, ReservedBarracksUnitPointersValue);

    FakeObservation ReservedBarracksObservationValue;
    ReservedBarracksObservationValue.GameLoopValue = 800U;
    ReservedBarracksObservationValue.SetUnits(ReservedBarracksUnitPointersValue);
    FakeQuery ReservedBarracksQueryValue;
    const FFrameContext ReservedBarracksFrameValue =
        FFrameContext::Create(&ReservedBarracksObservationValue, &ReservedBarracksQueryValue, 50U);

    FAgentState ReservedBarracksAgentStateValue;
    ReservedBarracksAgentStateValue.Update(ReservedBarracksFrameValue);

    FGameStateDescriptor ReservedBarracksGameStateDescriptorValue;
    ReservedBarracksGameStateDescriptorValue.CurrentStep = 50U;
    ReservedBarracksGameStateDescriptorValue.CurrentGameLoop = 800U;
    ReservedBarracksGameStateDescriptorValue.BuildPlanning.AvailableMinerals = 300U;
    ReservedBarracksGameStateDescriptorValue.BuildPlanning.AvailableVespene = 100U;
    ReservedBarracksGameStateDescriptorValue.BuildPlanning.AvailableSupply = 20U;
    ReservedBarracksGameStateDescriptorValue.RampWallDescriptor =
        AddonBlockerGameStateDescriptorValue.RampWallDescriptor;
    ReservedBarracksGameStateDescriptorValue.MainBaseLayoutDescriptor =
        AddonBlockerGameStateDescriptorValue.MainBaseLayoutDescriptor;
    FCommandAuthoritySchedulingState& ReservedBarracksSchedulingStateValue =
        ReservedBarracksGameStateDescriptorValue.CommandAuthoritySchedulingState;

    FCommandOrderRecord ReservedAddonEconomyOrderValue = AddonBlockerEconomyOrderValue;
    ReservedAddonEconomyOrderValue.CreationStep = 50U;
    ReservedAddonEconomyOrderValue.BasePriorityValue = 200;
    const uint32_t ReservedAddonEconomyOrderIdValue =
        ReservedBarracksSchedulingStateValue.EnqueueOrder(ReservedAddonEconomyOrderValue);

    FCommandOrderRecord ReservedMarineEconomyOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::EconomyAndProduction, NullTag, ABILITY_ID::TRAIN_MARINE, 150,
        EIntentDomain::UnitProduction, 50U);
    ReservedMarineEconomyOrderValue.TaskType = ECommandTaskType::UnitProduction;
    ReservedMarineEconomyOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKS;
    ReservedMarineEconomyOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_MARINE;
    ReservedMarineEconomyOrderValue.TargetCount = 1U;
    const uint32_t ReservedMarineEconomyOrderIdValue =
        ReservedBarracksSchedulingStateValue.EnqueueOrder(ReservedMarineEconomyOrderValue);

    IntentBufferValue.Reset();
    EconomyProductionOrderExpanderValue.ExpandEconomyAndProductionOrders(
        ReservedBarracksFrameValue, ReservedBarracksAgentStateValue, ReservedBarracksGameStateDescriptorValue,
        IntentBufferValue, BuildPlacementServiceValue, ExpansionLocationsValue);

    size_t ReservedAddonChildOrderIndexValue = 0U;
    Check(ReservedBarracksSchedulingStateValue.TryGetActiveChildOrderIndex(
              ReservedAddonEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution,
              ReservedAddonChildOrderIndexValue),
          SuccessValue,
          "The mandatory opening add-on should keep ownership of the wall barracks producer.");
    Check(!ReservedBarracksSchedulingStateValue.TryGetActiveChildOrderIndex(
              ReservedMarineEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution,
              ReservedAddonChildOrderIndexValue),
          SuccessValue,
          "Generic marine production should not consume the wall barracks while its mandatory reactor is still pending.");

    std::vector<Unit> CommandCenterUnitStorageValue;
    CommandCenterUnitStorageValue.push_back(
        MakeUnit(801U, UNIT_TYPEID::TERRAN_SCV, Unit::Alliance::Self, Point2D(12.0f, 10.0f), false));
    CommandCenterUnitStorageValue.push_back(MakeUnit(802U, UNIT_TYPEID::TERRAN_COMMANDCENTER,
                                                     Unit::Alliance::Self, Point2D(10.0f, 10.0f), true));

    Units CommandCenterUnitPointersValue;
    AppendUnitPointers(CommandCenterUnitStorageValue, CommandCenterUnitPointersValue);

    FakeObservation CommandCenterObservationValue;
    CommandCenterObservationValue.GameLoopValue = 500U;
    CommandCenterObservationValue.SetUnits(CommandCenterUnitPointersValue);

    FakeQuery CommandCenterQueryValue;
    CommandCenterQueryValue.RejectWorkerSpecificCommandCenterPlacementValue = true;
    const FFrameContext CommandCenterFrameValue =
        FFrameContext::Create(&CommandCenterObservationValue, &CommandCenterQueryValue, 5U);

    FAgentState CommandCenterAgentStateValue;
    CommandCenterAgentStateValue.Update(CommandCenterFrameValue);

    FGameStateDescriptor CommandCenterGameStateDescriptorValue;
    CommandCenterGameStateDescriptorValue.CurrentStep = 500U;
    CommandCenterGameStateDescriptorValue.CurrentGameLoop = 500U;
    CommandCenterGameStateDescriptorValue.BuildPlanning.AvailableMinerals = 500U;
    CommandCenterGameStateDescriptorValue.BuildPlanning.AvailableVespene = 0U;
    CommandCenterGameStateDescriptorValue.BuildPlanning.AvailableSupply = 20U;
    CommandCenterGameStateDescriptorValue.BuildPlanning.ObservedTownHallCount = 1U;

    FCommandAuthoritySchedulingState& CommandCenterSchedulingStateValue =
        CommandCenterGameStateDescriptorValue.CommandAuthoritySchedulingState;
    FCommandOrderRecord CommandCenterEconomyOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::EconomyAndProduction, NullTag, ABILITY_ID::BUILD_COMMANDCENTER, 150,
        EIntentDomain::StructureBuild, 5U);
    CommandCenterEconomyOrderValue.TargetCount = 2U;
    CommandCenterEconomyOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
    CommandCenterEconomyOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_COMMANDCENTER;
    const uint32_t CommandCenterEconomyOrderIdValue =
        CommandCenterSchedulingStateValue.EnqueueOrder(CommandCenterEconomyOrderValue);

    const std::vector<Point2D> CommandCenterExpansionLocationsValue =
    {
        Point2D(30.0f, 10.0f),
    };

    IntentBufferValue.Reset();
    EconomyProductionOrderExpanderValue.ExpandEconomyAndProductionOrders(
        CommandCenterFrameValue, CommandCenterAgentStateValue, CommandCenterGameStateDescriptorValue,
        IntentBufferValue, BuildPlacementServiceValue, CommandCenterExpansionLocationsValue);

    size_t CommandCenterChildOrderIndexValue = 0U;
    Check(CommandCenterSchedulingStateValue.TryGetActiveChildOrderIndex(
              CommandCenterEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution,
              CommandCenterChildOrderIndexValue),
          SuccessValue,
          "A natural expansion order should create an SCV child order when the expansion point is globally valid.");
    if (CommandCenterSchedulingStateValue.TryGetActiveChildOrderIndex(
            CommandCenterEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution,
            CommandCenterChildOrderIndexValue))
    {
        const FCommandOrderRecord CommandCenterChildOrderValue =
            CommandCenterSchedulingStateValue.GetOrderRecord(CommandCenterChildOrderIndexValue);
        Check(CommandCenterChildOrderValue.TargetPoint == Point2D(30.0f, 10.0f),
              SuccessValue,
              "The natural expansion child order should target the selected natural expansion location.");
    }

    std::vector<Unit> RefineryUnitStorageValue;
    RefineryUnitStorageValue.push_back(
        MakeUnit(401U, UNIT_TYPEID::TERRAN_SCV, Unit::Alliance::Self, Point2D(12.0f, 10.0f), false));
    RefineryUnitStorageValue.push_back(MakeUnit(402U, UNIT_TYPEID::TERRAN_COMMANDCENTER, Unit::Alliance::Self,
                                                Point2D(10.0f, 10.0f), true));
    RefineryUnitStorageValue.push_back(MakeUnit(403U, UNIT_TYPEID::TERRAN_COMMANDCENTER, Unit::Alliance::Self,
                                                Point2D(30.0f, 10.0f), true));
    RefineryUnitStorageValue.push_back(MakeUnit(501U, UNIT_TYPEID::NEUTRAL_VESPENEGEYSER,
                                                Unit::Alliance::Neutral, Point2D(13.0f, 8.0f), true));
    RefineryUnitStorageValue.push_back(MakeUnit(502U, UNIT_TYPEID::NEUTRAL_VESPENEGEYSER,
                                                Unit::Alliance::Neutral, Point2D(7.0f, 12.0f), true));
    RefineryUnitStorageValue.push_back(MakeUnit(503U, UNIT_TYPEID::NEUTRAL_VESPENEGEYSER,
                                                Unit::Alliance::Neutral, Point2D(33.0f, 8.0f), true));
    RefineryUnitStorageValue.push_back(MakeUnit(504U, UNIT_TYPEID::NEUTRAL_VESPENEGEYSER,
                                                Unit::Alliance::Neutral, Point2D(27.0f, 12.0f), true));

    Units RefineryUnitPointersValue;
    AppendUnitPointers(RefineryUnitStorageValue, RefineryUnitPointersValue);

    FakeObservation RefineryObservationValue;
    RefineryObservationValue.SetUnits(RefineryUnitPointersValue);
    FakeQuery RefineryQueryValue;
    const FFrameContext RefineryFrameValue =
        FFrameContext::Create(&RefineryObservationValue, &RefineryQueryValue, 3U);

    FAgentState RefineryAgentStateValue;
    RefineryAgentStateValue.Update(RefineryFrameValue);

    FGameStateDescriptor RefineryGameStateDescriptorValue;
    RefineryGameStateDescriptorValue.CurrentStep = 3U;
    RefineryGameStateDescriptorValue.CurrentGameLoop = 3U;
    RefineryGameStateDescriptorValue.BuildPlanning.AvailableMinerals = 500U;
    RefineryGameStateDescriptorValue.BuildPlanning.AvailableVespene = 0U;
    RefineryGameStateDescriptorValue.BuildPlanning.AvailableSupply = 20U;

    FCommandAuthoritySchedulingState& RefinerySchedulingStateValue =
        RefineryGameStateDescriptorValue.CommandAuthoritySchedulingState;
    FCommandOrderRecord RefineryEconomyOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::EconomyAndProduction, NullTag, ABILITY_ID::BUILD_REFINERY, 150,
        EIntentDomain::StructureBuild, 3U);
    RefineryEconomyOrderValue.TargetCount = 1U;
    RefineryEconomyOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
    RefineryEconomyOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_REFINERY;
    const uint32_t RefineryEconomyOrderIdValue =
        RefinerySchedulingStateValue.EnqueueOrder(RefineryEconomyOrderValue);

    const std::vector<Point2D> RefineryExpansionLocationsValue =
    {
        Point2D(30.0f, 10.0f),
    };

    EconomyProductionOrderExpanderValue.ExpandEconomyAndProductionOrders(
        RefineryFrameValue, RefineryAgentStateValue, RefineryGameStateDescriptorValue, IntentBufferValue,
        BuildPlacementServiceValue, RefineryExpansionLocationsValue);

    size_t RefineryChildOrderIndexValue = 0U;
    Check(RefinerySchedulingStateValue.TryGetActiveChildOrderIndex(
              RefineryEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution, RefineryChildOrderIndexValue),
          SuccessValue, "A refinery order should create a worker child order when main-base geysers are available.");
    if (RefinerySchedulingStateValue.TryGetActiveChildOrderIndex(
            RefineryEconomyOrderIdValue, ECommandAuthorityLayer::UnitExecution, RefineryChildOrderIndexValue))
    {
        const FCommandOrderRecord RefineryChildOrderValue =
            RefinerySchedulingStateValue.GetOrderRecord(RefineryChildOrderIndexValue);
        Check(RefineryChildOrderValue.TargetUnitTag == 501U || RefineryChildOrderValue.TargetUnitTag == 502U,
              SuccessValue, "The first refinery should target one of the main-base geysers before the natural.");
    }

    return SuccessValue;
}

}  // namespace sc2
