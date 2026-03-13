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
        (void)AbilityIdValue;
        (void)TargetPointValue;
        (void)UnitPtr;
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

    FCommandAuthoritySchedulingState& OccupiedRailSchedulingStateValue =
        OccupiedRailGameStateDescriptorValue.CommandAuthoritySchedulingState;
    FCommandOrderRecord BlockingRailOrderValue = FCommandOrderRecord::CreateNoTarget(
        ECommandAuthorityLayer::EconomyAndProduction, NullTag, ABILITY_ID::BUILD_BARRACKS, 180,
        EIntentDomain::StructureBuild, 6U);
    BlockingRailOrderValue.TargetCount = 1U;
    BlockingRailOrderValue.ProducerUnitTypeId = UNIT_TYPEID::TERRAN_SCV;
    BlockingRailOrderValue.ResultUnitTypeId = UNIT_TYPEID::TERRAN_BARRACKS;
    BlockingRailOrderValue.ReservedPlacementSlotId = ProductionRailFactorySlotValue.SlotId;
    const uint32_t BlockingRailOrderIdValue = OccupiedRailSchedulingStateValue.EnqueueOrder(BlockingRailOrderValue);
    (void)BlockingRailOrderIdValue;

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
