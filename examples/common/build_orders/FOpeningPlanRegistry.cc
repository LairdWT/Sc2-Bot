#include "common/build_orders/FOpeningPlanRegistry.h"

#include <initializer_list>

namespace sc2
{
namespace
{

FBuildPlacementSlotId CreatePlacementSlotId(const EBuildPlacementSlotType BuildPlacementSlotTypeValue,
                                            const uint8_t SlotOrdinalValue)
{
    FBuildPlacementSlotId BuildPlacementSlotIdValue;
    BuildPlacementSlotIdValue.SlotType = BuildPlacementSlotTypeValue;
    BuildPlacementSlotIdValue.Ordinal = SlotOrdinalValue;
    return BuildPlacementSlotIdValue;
}

FOpeningPlanStep CreateOpeningPlanStep(const uint32_t StepIdValue, const uint64_t MinGameLoopValue,
                                       const int PriorityValue, const AbilityID AbilityIdValue,
                                       const UNIT_TYPEID ProducerUnitTypeIdValue,
                                       const UNIT_TYPEID ResultUnitTypeIdValue, const uint32_t TargetCountValue,
                                       const uint32_t RequestedQueueCountValue,
                                       const uint32_t ParallelGroupIdValue,
                                       const std::initializer_list<uint32_t>& RequiredCompletedStepIdsValue,
                                       const EBuildPlacementSlotType PreferredPlacementSlotTypeValue =
                                           EBuildPlacementSlotType::Unknown,
                                       const FBuildPlacementSlotId PreferredPlacementSlotIdValue =
                                           FBuildPlacementSlotId(),
                                       const UpgradeID UpgradeIdValue = UpgradeID(UPGRADE_ID::INVALID))
{
    FOpeningPlanStep OpeningPlanStepValue;
    OpeningPlanStepValue.StepId = StepIdValue;
    OpeningPlanStepValue.MinGameLoop = MinGameLoopValue;
    OpeningPlanStepValue.PriorityValue = PriorityValue;
    OpeningPlanStepValue.AbilityId = AbilityIdValue;
    OpeningPlanStepValue.ProducerUnitTypeId = ProducerUnitTypeIdValue;
    OpeningPlanStepValue.ResultUnitTypeId = ResultUnitTypeIdValue;
    OpeningPlanStepValue.TargetCount = TargetCountValue;
    OpeningPlanStepValue.RequestedQueueCount = RequestedQueueCountValue;
    OpeningPlanStepValue.ParallelGroupId = ParallelGroupIdValue;
    OpeningPlanStepValue.UpgradeId = UpgradeIdValue;
    OpeningPlanStepValue.PreferredPlacementSlotType = PreferredPlacementSlotTypeValue;
    OpeningPlanStepValue.PreferredPlacementSlotId = PreferredPlacementSlotIdValue;
    OpeningPlanStepValue.RequiredCompletedStepIds.assign(RequiredCompletedStepIdsValue.begin(),
                                                         RequiredCompletedStepIdsValue.end());
    return OpeningPlanStepValue;
}

const FOpeningPlanDescriptor& CreateTerranTwoBaseMMMFrameOpeningDescriptor()
{
    static const FOpeningPlanDescriptor DescriptorValue =
        []()
        {
            FOpeningPlanDescriptor OpeningPlanDescriptorValue;
            OpeningPlanDescriptorValue.OpeningPlanId = EOpeningPlanId::TerranTwoBaseMMMFrameOpening;
            OpeningPlanDescriptorValue.BuildOrderId = "Terran.TwoBaseMMM.EightMinutePressure";
            OpeningPlanDescriptorValue.Summary =
                "Two-base MMM pressure package expressed in explicit game-loop steps through the 8:17 follow-up add-on wave.";

            OpeningPlanDescriptorValue.Goals.TargetBaseCount = 3U;
            OpeningPlanDescriptorValue.Goals.TargetOrbitalCommandCount = 2U;
            OpeningPlanDescriptorValue.Goals.TargetWorkerCount = 44U;
            OpeningPlanDescriptorValue.Goals.TargetRefineryCount = 4U;
            OpeningPlanDescriptorValue.Goals.TargetSupplyDepotCount = 14U;
            OpeningPlanDescriptorValue.Goals.TargetBarracksCount = 5U;
            OpeningPlanDescriptorValue.Goals.TargetFactoryCount = 1U;
            OpeningPlanDescriptorValue.Goals.TargetStarportCount = 1U;
            OpeningPlanDescriptorValue.Goals.TargetBarracksReactorCount = 2U;
            OpeningPlanDescriptorValue.Goals.TargetBarracksTechLabCount = 1U;
            OpeningPlanDescriptorValue.Goals.TargetFactoryTechLabCount = 2U;
            OpeningPlanDescriptorValue.Goals.TargetStarportReactorCount = 1U;
            OpeningPlanDescriptorValue.Goals.TargetMarineCount = 36U;
            OpeningPlanDescriptorValue.Goals.TargetMarauderCount = 15U;
            OpeningPlanDescriptorValue.Goals.TargetHellionCount = 1U;
            OpeningPlanDescriptorValue.Goals.TargetCycloneCount = 1U;
            OpeningPlanDescriptorValue.Goals.TargetMedivacCount = 6U;
            OpeningPlanDescriptorValue.Goals.TargetLiberatorCount = 1U;
            OpeningPlanDescriptorValue.Goals.TargetSiegeTankCount = 1U;

            std::vector<FOpeningPlanStep>& StepsValue = OpeningPlanDescriptorValue.Steps;
            StepsValue.reserve(102U);
            StepsValue.push_back(CreateOpeningPlanStep(1U, 358U, 100, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 1U,
                                                       1U, 0U, {}, EBuildPlacementSlotType::MainRampDepotLeft));
            StepsValue.push_back(CreateOpeningPlanStep(2U, 896U, 95, ABILITY_ID::BUILD_BARRACKS,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_BARRACKS, 1U,
                                                       1U, 15U, {1U},
                                                       EBuildPlacementSlotType::MainRampBarracksWithAddon));
            StepsValue.push_back(CreateOpeningPlanStep(3U, 941U, 94, ABILITY_ID::BUILD_REFINERY,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_REFINERY, 1U,
                                                       1U, 15U, {1U}));
            StepsValue.push_back(CreateOpeningPlanStep(4U, 1926U, 90, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 1U,
                                                       1U, 0U, {2U}));
            StepsValue.push_back(CreateOpeningPlanStep(5U, 1949U, 89, ABILITY_ID::MORPH_ORBITALCOMMAND,
                                                       UNIT_TYPEID::TERRAN_COMMANDCENTER,
                                                       UNIT_TYPEID::TERRAN_ORBITALCOMMAND, 1U, 1U, 0U, {2U}));
            StepsValue.push_back(CreateOpeningPlanStep(6U, 2195U, 88, ABILITY_ID::BUILD_COMMANDCENTER,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_COMMANDCENTER, 2U,
                                                       1U, 20U, {3U}));
            StepsValue.push_back(CreateOpeningPlanStep(7U, 2330U, 87, ABILITY_ID::BUILD_REACTOR_BARRACKS,
                                                       UNIT_TYPEID::TERRAN_BARRACKS,
                                                       UNIT_TYPEID::TERRAN_BARRACKSREACTOR, 1U, 1U, 20U, {2U}));
            StepsValue.push_back(CreateOpeningPlanStep(8U, 2464U, 86, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 2U,
                                                       1U, 0U, {}, EBuildPlacementSlotType::MainRampDepotRight));
            StepsValue.push_back(CreateOpeningPlanStep(9U, 2867U, 84, ABILITY_ID::BUILD_FACTORY,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_FACTORY, 1U, 1U,
                                                       22U, {3U}, EBuildPlacementSlotType::MainFactoryWithAddon,
                                                       CreatePlacementSlotId(
                                                           EBuildPlacementSlotType::MainFactoryWithAddon, 0U)));
            StepsValue.push_back(CreateOpeningPlanStep(10U, 3024U, 83, ABILITY_ID::BUILD_REFINERY,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_REFINERY, 2U,
                                                       1U, 22U, {6U}));
            StepsValue.push_back(CreateOpeningPlanStep(11U, 3136U, 82, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 3U,
                                                       2U, 0U, {7U}));
            StepsValue.push_back(CreateOpeningPlanStep(12U, 3699U, 81, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 5U,
                                                       2U, 0U, {11U}));
            StepsValue.push_back(CreateOpeningPlanStep(13U, 3808U, 80, ABILITY_ID::MORPH_ORBITALCOMMAND,
                                                       UNIT_TYPEID::TERRAN_COMMANDCENTER,
                                                       UNIT_TYPEID::TERRAN_ORBITALCOMMAND, 2U, 1U, 0U, {6U}));
            StepsValue.push_back(CreateOpeningPlanStep(14U, 3853U, 79, ABILITY_ID::BUILD_STARPORT,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_STARPORT, 1U,
                                                       1U, 29U, {9U},
                                                       EBuildPlacementSlotType::MainStarportWithAddon,
                                                       CreatePlacementSlotId(
                                                           EBuildPlacementSlotType::MainStarportWithAddon, 0U)));
            StepsValue.push_back(CreateOpeningPlanStep(15U, 3898U, 78, ABILITY_ID::TRAIN_HELLION,
                                                       UNIT_TYPEID::TERRAN_FACTORY, UNIT_TYPEID::TERRAN_HELLION, 1U,
                                                       1U, 29U, {9U}));
            StepsValue.push_back(CreateOpeningPlanStep(16U, 4010U, 77, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 6U,
                                                       1U, 0U, {12U}));
            StepsValue.push_back(CreateOpeningPlanStep(17U, 4077U, 76, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 7U,
                                                       1U, 0U, {16U}));
            StepsValue.push_back(CreateOpeningPlanStep(18U, 4278U, 75, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 3U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(19U, 4390U, 74, ABILITY_ID::BUILD_TECHLAB_FACTORY,
                                                       UNIT_TYPEID::TERRAN_FACTORY,
                                                       UNIT_TYPEID::TERRAN_FACTORYTECHLAB, 1U, 1U, 0U, {9U}));
            StepsValue.push_back(CreateOpeningPlanStep(20U, 4502U, 73, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 8U,
                                                       1U, 36U, {17U}));
            StepsValue.push_back(CreateOpeningPlanStep(21U, 4525U, 72, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 9U,
                                                       1U, 36U, {20U}));
            StepsValue.push_back(CreateOpeningPlanStep(22U, 4659U, 71, ABILITY_ID::TRAIN_MEDIVAC,
                                                       UNIT_TYPEID::TERRAN_STARPORT, UNIT_TYPEID::TERRAN_MEDIVAC, 1U,
                                                       1U, 0U, {14U}));
            StepsValue.push_back(CreateOpeningPlanStep(23U, 4794U, 70, ABILITY_ID::TRAIN_CYCLONE,
                                                       UNIT_TYPEID::TERRAN_FACTORY, UNIT_TYPEID::TERRAN_CYCLONE, 1U,
                                                       1U, 0U, {19U}));
            StepsValue.push_back(CreateOpeningPlanStep(24U, 4950U, 69, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 11U,
                                                       2U, 0U, {21U}));
            StepsValue.push_back(CreateOpeningPlanStep(25U, 5040U, 68, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 4U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(26U, 5264U, 67, ABILITY_ID::BUILD_BARRACKS,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_BARRACKS, 2U, 1U,
                                                       0U, {6U}, EBuildPlacementSlotType::MainBarracksWithAddon,
                                                       CreatePlacementSlotId(
                                                           EBuildPlacementSlotType::MainBarracksWithAddon, 0U)));
            StepsValue.push_back(CreateOpeningPlanStep(27U, 5398U, 66, ABILITY_ID::TRAIN_LIBERATOR,
                                                       UNIT_TYPEID::TERRAN_STARPORT, UNIT_TYPEID::TERRAN_LIBERATOR,
                                                       1U, 1U, 51U, {14U}));
            StepsValue.push_back(CreateOpeningPlanStep(28U, 5421U, 65, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 12U,
                                                       1U, 51U, {24U}));
            StepsValue.push_back(CreateOpeningPlanStep(29U, 5578U, 64, ABILITY_ID::TRAIN_SIEGETANK,
                                                       UNIT_TYPEID::TERRAN_FACTORY, UNIT_TYPEID::TERRAN_SIEGETANK,
                                                       1U, 1U, 0U, {19U}));
            StepsValue.push_back(CreateOpeningPlanStep(30U, 5712U, 63, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 5U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(31U, 5779U, 62, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 13U,
                                                       1U, 0U, {28U}));
            StepsValue.push_back(CreateOpeningPlanStep(32U, 5824U, 61, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 14U,
                                                       1U, 0U, {31U}));
            StepsValue.push_back(CreateOpeningPlanStep(33U, 6070U, 68, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 6U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(34U, 6182U, 67, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 15U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(35U, 6249U, 66, ABILITY_ID::BUILD_BARRACKS,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_BARRACKS, 3U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(36U, 6272U, 65, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 16U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(37U, 6540U, 64, ABILITY_ID::BUILD_ENGINEERINGBAY,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_ENGINEERINGBAY, 1U,
                                                       1U, 37U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(38U, 6540U, 63, ABILITY_ID::RESEARCH_STIMPACK,
                                                       UNIT_TYPEID::TERRAN_BARRACKSTECHLAB, UNIT_TYPEID::INVALID, 1U,
                                                       1U, 37U, {}, EBuildPlacementSlotType::Unknown,
                                                       FBuildPlacementSlotId(), UpgradeID(UPGRADE_ID::STIMPACK)));
            StepsValue.push_back(CreateOpeningPlanStep(39U, 6540U, 62, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 17U,
                                                       1U, 37U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(40U, 6563U, 61, ABILITY_ID::BUILD_REFINERY,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_REFINERY, 3U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(41U, 6608U, 60, ABILITY_ID::BUILD_REACTOR_STARPORT,
                                                       UNIT_TYPEID::TERRAN_STARPORT,
                                                       UNIT_TYPEID::TERRAN_STARPORTREACTOR, 1U, 1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(42U, 6742U, 59, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 7U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(43U, 6787U, 58, ABILITY_ID::BUILD_TECHLAB_FACTORY,
                                                       UNIT_TYPEID::TERRAN_FACTORY,
                                                       UNIT_TYPEID::TERRAN_FACTORYTECHLAB, 2U, 1U, 41U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(44U, 6787U, 57, ABILITY_ID::BUILD_REFINERY,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_REFINERY, 4U,
                                                       1U, 41U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(45U, 6787U, 56, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 18U,
                                                       1U, 41U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(46U, 6809U, 55, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 19U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(47U, 6966U, 54, ABILITY_ID::BUILD_BUNKER,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_BUNKER, 1U, 1U,
                                                       0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(48U, 7056U, 53, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 8U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(49U, 7123U, 52, ABILITY_ID::TRAIN_MARAUDER,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARAUDER, 1U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(50U, 7190U, 51, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 20U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(51U, 7369U, 50, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 9U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(52U, 7414U, 49, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 21U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(53U, 7459U, 48, ABILITY_ID::TRAIN_MARAUDER,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARAUDER, 2U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(54U, 7526U, 47, ABILITY_ID::BUILD_REACTOR_FACTORY,
                                                       UNIT_TYPEID::TERRAN_FACTORY,
                                                       UNIT_TYPEID::TERRAN_FACTORYREACTOR, 1U, 1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(55U, 7571U, 46, ABILITY_ID::RESEARCH_COMBATSHIELD,
                                                       UNIT_TYPEID::TERRAN_BARRACKSTECHLAB, UNIT_TYPEID::INVALID, 1U,
                                                       1U, 0U, {}, EBuildPlacementSlotType::Unknown,
                                                       FBuildPlacementSlotId(), UpgradeID(UPGRADE_ID::SHIELDWALL)));
            StepsValue.push_back(CreateOpeningPlanStep(
                56U, 7593U, 45, ABILITY_ID::RESEARCH_TERRANINFANTRYWEAPONSLEVEL1,
                UNIT_TYPEID::TERRAN_ENGINEERINGBAY, UNIT_TYPEID::INVALID, 1U, 1U, 0U, {},
                EBuildPlacementSlotType::Unknown, FBuildPlacementSlotId(),
                UpgradeID(UPGRADE_ID::TERRANINFANTRYWEAPONSLEVEL1)));
            StepsValue.push_back(CreateOpeningPlanStep(57U, 7638U, 44, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 22U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(58U, 7705U, 43, ABILITY_ID::TRAIN_MARAUDER,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARAUDER, 3U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(59U, 7795U, 42, ABILITY_ID::TRAIN_MEDIVAC,
                                                       UNIT_TYPEID::TERRAN_STARPORT, UNIT_TYPEID::TERRAN_MEDIVAC, 2U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(60U, 7884U, 41, ABILITY_ID::TRAIN_MEDIVAC,
                                                       UNIT_TYPEID::TERRAN_STARPORT, UNIT_TYPEID::TERRAN_MEDIVAC, 3U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(61U, 7974U, 40, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 23U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(62U, 7996U, 39, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 10U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(63U, 8041U, 38, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 24U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(64U, 8063U, 37, ABILITY_ID::TRAIN_MARAUDER,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARAUDER, 4U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(65U, 8175U, 36, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 11U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(66U, 8198U, 35, ABILITY_ID::TRAIN_MARAUDER,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARAUDER, 5U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(67U, 8400U, 34, ABILITY_ID::TRAIN_WIDOWMINE,
                                                       UNIT_TYPEID::TERRAN_FACTORY, UNIT_TYPEID::TERRAN_WIDOWMINE, 2U,
                                                       2U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(68U, 8467U, 33, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 26U,
                                                       2U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(69U, 8534U, 32, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 12U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(70U, 8556U, 31, ABILITY_ID::TRAIN_MARAUDER,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARAUDER, 6U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(71U, 8668U, 30, ABILITY_ID::TRAIN_MARAUDER,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARAUDER, 7U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(72U, 8780U, 29, ABILITY_ID::RESEARCH_CONCUSSIVESHELLS,
                                                       UNIT_TYPEID::TERRAN_BARRACKSTECHLAB, UNIT_TYPEID::INVALID, 1U,
                                                       1U, 0U, {}, EBuildPlacementSlotType::Unknown,
                                                       FBuildPlacementSlotId(),
                                                       UpgradeID(UPGRADE_ID::PUNISHERGRENADES)));
            StepsValue.push_back(CreateOpeningPlanStep(73U, 8870U, 28, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 27U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(74U, 8892U, 27, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 28U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(75U, 8915U, 26, ABILITY_ID::TRAIN_MEDIVAC,
                                                       UNIT_TYPEID::TERRAN_STARPORT, UNIT_TYPEID::TERRAN_MEDIVAC, 4U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(76U, 9094U, 25, ABILITY_ID::TRAIN_WIDOWMINE,
                                                       UNIT_TYPEID::TERRAN_FACTORY, UNIT_TYPEID::TERRAN_WIDOWMINE, 4U,
                                                       2U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(77U, 9184U, 24, ABILITY_ID::TRAIN_MARAUDER,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARAUDER, 8U,
                                                       1U, 73U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(78U, 9184U, 23, ABILITY_ID::BUILD_COMMANDCENTER,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_COMMANDCENTER, 3U,
                                                       1U, 73U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(79U, 9273U, 22, ABILITY_ID::TRAIN_MARAUDER,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARAUDER, 9U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(80U, 9296U, 21, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 13U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(81U, 9340U, 20, ABILITY_ID::TRAIN_MEDIVAC,
                                                       UNIT_TYPEID::TERRAN_STARPORT, UNIT_TYPEID::TERRAN_MEDIVAC, 5U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(82U, 9363U, 19, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 29U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(83U, 9385U, 18, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 30U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(84U, 9587U, 17, ABILITY_ID::TRAIN_MEDIVAC,
                                                       UNIT_TYPEID::TERRAN_STARPORT, UNIT_TYPEID::TERRAN_MEDIVAC, 6U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(85U, 9654U, 16, ABILITY_ID::TRAIN_MARAUDER,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARAUDER, 10U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(86U, 9744U, 15, ABILITY_ID::TRAIN_MARAUDER,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARAUDER, 11U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(87U, 9811U, 14, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 32U,
                                                       2U, 82U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(88U, 9811U, 13, ABILITY_ID::TRAIN_WIDOWMINE,
                                                       UNIT_TYPEID::TERRAN_FACTORY, UNIT_TYPEID::TERRAN_WIDOWMINE, 6U,
                                                       2U, 82U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(89U, 9990U, 12, ABILITY_ID::BUILD_BARRACKS,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_BARRACKS, 4U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(90U, 10012U, 11, ABILITY_ID::BUILD_BARRACKS,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_BARRACKS, 5U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(91U, 10192U, 10, ABILITY_ID::TRAIN_MARAUDER,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARAUDER, 12U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(
                92U, 10214U, 9, ABILITY_ID::TRAIN_VIKINGFIGHTER, UNIT_TYPEID::TERRAN_STARPORT,
                UNIT_TYPEID::TERRAN_VIKINGFIGHTER, 1U, 1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(93U, 10259U, 8, ABILITY_ID::TRAIN_MARAUDER,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARAUDER, 13U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(94U, 10326U, 7, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 14U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(95U, 10348U, 6, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 34U,
                                                       2U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(96U, 10572U, 5, ABILITY_ID::TRAIN_WIDOWMINE,
                                                       UNIT_TYPEID::TERRAN_FACTORY, UNIT_TYPEID::TERRAN_WIDOWMINE, 8U,
                                                       2U, 90U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(
                97U, 10572U, 4, ABILITY_ID::TRAIN_VIKINGFIGHTER, UNIT_TYPEID::TERRAN_STARPORT,
                UNIT_TYPEID::TERRAN_VIKINGFIGHTER, 2U, 1U, 90U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(98U, 10662U, 3, ABILITY_ID::TRAIN_MARAUDER,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARAUDER, 14U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(99U, 10729U, 2, ABILITY_ID::TRAIN_MARAUDER,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARAUDER, 15U,
                                                       1U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(100U, 10886U, 1, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 36U,
                                                       2U, 0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(101U, 11132U, 1, ABILITY_ID::BUILD_TECHLAB_BARRACKS,
                                                       UNIT_TYPEID::TERRAN_BARRACKS,
                                                       UNIT_TYPEID::TERRAN_BARRACKSTECHLAB, 1U, 1U, 94U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(102U, 11132U, 1, ABILITY_ID::BUILD_REACTOR_BARRACKS,
                                                       UNIT_TYPEID::TERRAN_BARRACKS,
                                                       UNIT_TYPEID::TERRAN_BARRACKSREACTOR, 2U, 1U, 94U, {}));
            return OpeningPlanDescriptorValue;
        }();

    return DescriptorValue;
}

}  // namespace

const FOpeningPlanDescriptor& FOpeningPlanRegistry::GetOpeningPlanDescriptor(const EOpeningPlanId OpeningPlanIdValue)
{
    switch (OpeningPlanIdValue)
    {
        case EOpeningPlanId::TerranTwoBaseMMMFrameOpening:
            return CreateTerranTwoBaseMMMFrameOpeningDescriptor();
        case EOpeningPlanId::Unknown:
        default:
            return CreateTerranTwoBaseMMMFrameOpeningDescriptor();
    }
}

}  // namespace sc2
