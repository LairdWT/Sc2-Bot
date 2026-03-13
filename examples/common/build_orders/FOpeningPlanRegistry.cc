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
                                       const uint32_t ParallelGroupIdValue,
                                       const std::initializer_list<uint32_t>& RequiredCompletedStepIdsValue,
                                       const EBuildPlacementSlotType PreferredPlacementSlotTypeValue =
                                           EBuildPlacementSlotType::Unknown,
                                       const FBuildPlacementSlotId PreferredPlacementSlotIdValue =
                                           FBuildPlacementSlotId())
{
    FOpeningPlanStep OpeningPlanStepValue;
    OpeningPlanStepValue.StepId = StepIdValue;
    OpeningPlanStepValue.MinGameLoop = MinGameLoopValue;
    OpeningPlanStepValue.PriorityValue = PriorityValue;
    OpeningPlanStepValue.AbilityId = AbilityIdValue;
    OpeningPlanStepValue.ProducerUnitTypeId = ProducerUnitTypeIdValue;
    OpeningPlanStepValue.ResultUnitTypeId = ResultUnitTypeIdValue;
    OpeningPlanStepValue.TargetCount = TargetCountValue;
    OpeningPlanStepValue.ParallelGroupId = ParallelGroupIdValue;
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
            OpeningPlanDescriptorValue.BuildOrderId = "Terran.TwoBaseMMM.FrameOpening";
            OpeningPlanDescriptorValue.Summary =
                "Two-base MMM opening through the first medivac, cyclone, liberator, and siege tank.";

            OpeningPlanDescriptorValue.Goals.TargetBaseCount = 2U;
            OpeningPlanDescriptorValue.Goals.TargetOrbitalCommandCount = 2U;
            OpeningPlanDescriptorValue.Goals.TargetWorkerCount = 36U;
            OpeningPlanDescriptorValue.Goals.TargetRefineryCount = 2U;
            OpeningPlanDescriptorValue.Goals.TargetSupplyDepotCount = 4U;
            OpeningPlanDescriptorValue.Goals.TargetBarracksCount = 2U;
            OpeningPlanDescriptorValue.Goals.TargetFactoryCount = 1U;
            OpeningPlanDescriptorValue.Goals.TargetStarportCount = 1U;
            OpeningPlanDescriptorValue.Goals.TargetBarracksReactorCount = 1U;
            OpeningPlanDescriptorValue.Goals.TargetFactoryTechLabCount = 1U;
            OpeningPlanDescriptorValue.Goals.TargetMarineCount = 14U;
            OpeningPlanDescriptorValue.Goals.TargetHellionCount = 1U;
            OpeningPlanDescriptorValue.Goals.TargetCycloneCount = 1U;
            OpeningPlanDescriptorValue.Goals.TargetMedivacCount = 1U;
            OpeningPlanDescriptorValue.Goals.TargetLiberatorCount = 1U;
            OpeningPlanDescriptorValue.Goals.TargetSiegeTankCount = 1U;

            std::vector<FOpeningPlanStep>& StepsValue = OpeningPlanDescriptorValue.Steps;
            StepsValue.reserve(32U);
            StepsValue.push_back(CreateOpeningPlanStep(1U, 358U, 100, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 1U,
                                                       0U, {}, EBuildPlacementSlotType::MainRampDepotLeft));
            StepsValue.push_back(CreateOpeningPlanStep(2U, 896U, 95, ABILITY_ID::BUILD_BARRACKS,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_BARRACKS, 1U,
                                                       15U, {1U},
                                                       EBuildPlacementSlotType::MainRampBarracksWithAddon));
            StepsValue.push_back(CreateOpeningPlanStep(3U, 941U, 94, ABILITY_ID::BUILD_REFINERY,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_REFINERY, 1U,
                                                       15U, {1U}));
            StepsValue.push_back(CreateOpeningPlanStep(4U, 1926U, 90, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 1U,
                                                       0U, {2U}));
            StepsValue.push_back(CreateOpeningPlanStep(5U, 1949U, 89, ABILITY_ID::MORPH_ORBITALCOMMAND,
                                                       UNIT_TYPEID::TERRAN_COMMANDCENTER,
                                                       UNIT_TYPEID::TERRAN_ORBITALCOMMAND, 1U, 0U, {2U}));
            StepsValue.push_back(CreateOpeningPlanStep(6U, 2195U, 88, ABILITY_ID::BUILD_COMMANDCENTER,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_COMMANDCENTER, 2U,
                                                       20U, {3U}));
            StepsValue.push_back(CreateOpeningPlanStep(7U, 2330U, 87, ABILITY_ID::BUILD_REACTOR_BARRACKS,
                                                       UNIT_TYPEID::TERRAN_BARRACKS,
                                                       UNIT_TYPEID::TERRAN_BARRACKSREACTOR, 1U, 20U, {2U}));
            StepsValue.push_back(CreateOpeningPlanStep(8U, 2464U, 86, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 2U,
                                                       0U, {}, EBuildPlacementSlotType::MainRampDepotRight));
            StepsValue.push_back(CreateOpeningPlanStep(9U, 2867U, 84, ABILITY_ID::BUILD_FACTORY,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_FACTORY, 1U, 22U,
                                                       {3U}, EBuildPlacementSlotType::MainProductionWithAddon,
                                                       CreatePlacementSlotId(
                                                           EBuildPlacementSlotType::MainProductionWithAddon, 1U)));
            StepsValue.push_back(CreateOpeningPlanStep(10U, 3024U, 83, ABILITY_ID::BUILD_REFINERY,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_REFINERY, 2U,
                                                       22U, {6U}));
            StepsValue.push_back(CreateOpeningPlanStep(11U, 3136U, 82, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 3U,
                                                       0U, {7U}));
            StepsValue.push_back(CreateOpeningPlanStep(12U, 3699U, 81, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 5U,
                                                       0U, {11U}));
            StepsValue.push_back(CreateOpeningPlanStep(13U, 3808U, 80, ABILITY_ID::MORPH_ORBITALCOMMAND,
                                                       UNIT_TYPEID::TERRAN_COMMANDCENTER,
                                                       UNIT_TYPEID::TERRAN_ORBITALCOMMAND, 2U, 0U, {6U}));
            StepsValue.push_back(CreateOpeningPlanStep(14U, 3853U, 79, ABILITY_ID::BUILD_STARPORT,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_STARPORT, 1U,
                                                       29U, {9U},
                                                       EBuildPlacementSlotType::MainProductionWithAddon,
                                                       CreatePlacementSlotId(
                                                           EBuildPlacementSlotType::MainProductionWithAddon, 2U)));
            StepsValue.push_back(CreateOpeningPlanStep(15U, 3898U, 78, ABILITY_ID::TRAIN_HELLION,
                                                       UNIT_TYPEID::TERRAN_FACTORY, UNIT_TYPEID::TERRAN_HELLION, 1U,
                                                       29U, {9U}));
            StepsValue.push_back(CreateOpeningPlanStep(16U, 4010U, 77, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 6U,
                                                       0U, {12U}));
            StepsValue.push_back(CreateOpeningPlanStep(17U, 4077U, 76, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 7U,
                                                       0U, {16U}));
            StepsValue.push_back(CreateOpeningPlanStep(18U, 4278U, 75, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 3U,
                                                       0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(19U, 4390U, 74, ABILITY_ID::BUILD_TECHLAB_FACTORY,
                                                       UNIT_TYPEID::TERRAN_FACTORY,
                                                       UNIT_TYPEID::TERRAN_FACTORYTECHLAB, 1U, 0U, {9U}));
            StepsValue.push_back(CreateOpeningPlanStep(20U, 4502U, 73, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 8U,
                                                       36U, {17U}));
            StepsValue.push_back(CreateOpeningPlanStep(21U, 4525U, 72, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 9U,
                                                       36U, {20U}));
            StepsValue.push_back(CreateOpeningPlanStep(22U, 4659U, 71, ABILITY_ID::TRAIN_MEDIVAC,
                                                       UNIT_TYPEID::TERRAN_STARPORT, UNIT_TYPEID::TERRAN_MEDIVAC, 1U,
                                                       0U, {14U}));
            StepsValue.push_back(CreateOpeningPlanStep(23U, 4794U, 70, ABILITY_ID::TRAIN_CYCLONE,
                                                       UNIT_TYPEID::TERRAN_FACTORY, UNIT_TYPEID::TERRAN_CYCLONE, 1U,
                                                       0U, {19U}));
            StepsValue.push_back(CreateOpeningPlanStep(24U, 4950U, 69, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 11U,
                                                       0U, {21U}));
            StepsValue.push_back(CreateOpeningPlanStep(25U, 5040U, 68, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 4U,
                                                       0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(26U, 5264U, 67, ABILITY_ID::BUILD_BARRACKS,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_BARRACKS, 2U, 0U,
                                                       {6U}, EBuildPlacementSlotType::MainProductionWithAddon,
                                                       CreatePlacementSlotId(
                                                           EBuildPlacementSlotType::MainProductionWithAddon, 0U)));
            StepsValue.push_back(CreateOpeningPlanStep(27U, 5398U, 66, ABILITY_ID::TRAIN_LIBERATOR,
                                                       UNIT_TYPEID::TERRAN_STARPORT, UNIT_TYPEID::TERRAN_LIBERATOR,
                                                       1U, 51U, {14U}));
            StepsValue.push_back(CreateOpeningPlanStep(28U, 5421U, 65, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 12U,
                                                       51U, {24U}));
            StepsValue.push_back(CreateOpeningPlanStep(29U, 5578U, 64, ABILITY_ID::TRAIN_SIEGETANK,
                                                       UNIT_TYPEID::TERRAN_FACTORY, UNIT_TYPEID::TERRAN_SIEGETANK,
                                                       1U, 0U, {19U}));
            StepsValue.push_back(CreateOpeningPlanStep(30U, 5712U, 63, ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                       UNIT_TYPEID::TERRAN_SCV, UNIT_TYPEID::TERRAN_SUPPLYDEPOT, 5U,
                                                       0U, {}));
            StepsValue.push_back(CreateOpeningPlanStep(31U, 5779U, 62, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 13U,
                                                       0U, {28U}));
            StepsValue.push_back(CreateOpeningPlanStep(32U, 5824U, 61, ABILITY_ID::TRAIN_MARINE,
                                                       UNIT_TYPEID::TERRAN_BARRACKS, UNIT_TYPEID::TERRAN_MARINE, 14U,
                                                       0U, {31U}));
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
