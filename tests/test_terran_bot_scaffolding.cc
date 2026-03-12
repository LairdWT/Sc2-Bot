#include "test_terran_bot_scaffolding.h"

#include <iostream>

#include "common/armies/EArmyGoal.h"
#include "common/armies/FArmyDomainState.h"
#include "common/build_planning/FBuildPlanningState.h"
#include "common/descriptors/EGamePlan.h"
#include "common/descriptors/EMacroPhase.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/descriptors/FMacroStateDescriptor.h"
#include "common/planning/EIntentPlaybackState.h"
#include "common/planning/EPlanningProcessorState.h"
#include "common/spatial/FSpatialFieldSet.h"

namespace sc2
{
namespace
{

bool Check(const bool ConditionValue, bool& SuccessValue, const char* MessageValue)
{
    if (!ConditionValue)
    {
        SuccessValue = false;
        std::cerr << "    " << MessageValue << std::endl;
    }

    return ConditionValue;
}

}  // namespace

bool TestTerranBotScaffolding(int ArgC, char** ArgV)
{
    (void)ArgC;
    (void)ArgV;

    bool SuccessValue = true;

    const FMacroStateDescriptor MacroStateDescriptorValue;
    Check(MacroStateDescriptorValue.ActiveGamePlan == EGamePlan::Unknown, SuccessValue,
          "Macro state default game plan should be Unknown.");
    Check(MacroStateDescriptorValue.ActiveMacroPhase == EMacroPhase::Opening, SuccessValue,
          "Macro state default phase should be Opening.");
    Check(MacroStateDescriptorValue.DesiredBaseCount == 1U, SuccessValue,
          "Macro state should default to one desired base.");
    Check(MacroStateDescriptorValue.DesiredArmyCount == 1U, SuccessValue,
          "Macro state should default to one desired army.");

    FArmyDomainState ArmyDomainStateValue;
    Check(ArmyDomainStateValue.MinimumArmyCount == 1U, SuccessValue,
          "Army domain should default to one minimum army.");
    Check(ArmyDomainStateValue.ActiveArmyCount == 1U, SuccessValue,
          "Army domain should always create the primary army anchor.");
    Check(ArmyDomainStateValue.ArmyGoals.size() == 1U, SuccessValue,
          "Army domain should own exactly one default army goal entry after reset.");
    Check(!ArmyDomainStateValue.ArmyGoals.empty() && ArmyDomainStateValue.ArmyGoals.front() == EArmyGoal::Unknown,
          SuccessValue, "Army domain should default the primary army goal to Unknown.");

    ArmyDomainStateValue.MinimumArmyCount = 3U;
    ArmyDomainStateValue.ArmyGoals.clear();
    ArmyDomainStateValue.ActiveArmyCount = 0U;
    ArmyDomainStateValue.EnsurePrimaryArmyExists();
    Check(ArmyDomainStateValue.ActiveArmyCount == 3U, SuccessValue,
          "Army domain should expand to the configured minimum army count.");
    Check(ArmyDomainStateValue.ArmyGoals.size() == 3U, SuccessValue,
          "Army goal storage should match the configured minimum army count.");

    FBuildPlanningState BuildPlanningStateValue;
    BuildPlanningStateValue.ActivePackageCount = 9U;
    BuildPlanningStateValue.ReservedMinerals = 500U;
    BuildPlanningStateValue.CommittedSupply = 7U;
    BuildPlanningStateValue.Reset();
    Check(BuildPlanningStateValue.ActivePackageCount == 0U, SuccessValue,
          "Build planning reset should clear active package count.");
    Check(BuildPlanningStateValue.ReservedMinerals == 0U, SuccessValue,
          "Build planning reset should clear reserved minerals.");
    Check(BuildPlanningStateValue.CommittedSupply == 0U, SuccessValue,
          "Build planning reset should clear committed supply.");

    FSpatialFieldSet SpatialFieldSetValue;
    Check(!SpatialFieldSetValue.IsValid(), SuccessValue,
          "Spatial field set should be invalid before any resize.");
    SpatialFieldSetValue.Resize(4U, 3U);
    Check(SpatialFieldSetValue.IsValid(), SuccessValue,
          "Spatial field set should become valid after resize.");
    Check(SpatialFieldSetValue.EnemyGroundThreat.size() == 12U, SuccessValue,
          "Spatial field threat layer should match the resized field area.");
    Check(SpatialFieldSetValue.FriendlyGroundInfluence.size() == 12U, SuccessValue,
          "Spatial field friendly influence layer should match the resized field area.");
    Check(SpatialFieldSetValue.RetreatSafety.size() == 12U, SuccessValue,
          "Spatial field retreat safety layer should match the resized field area.");
    Check(SpatialFieldSetValue.EngageOpportunity.size() == 12U, SuccessValue,
          "Spatial field opportunity layer should match the resized field area.");
    SpatialFieldSetValue.Reset();
    Check(!SpatialFieldSetValue.IsValid(), SuccessValue,
          "Spatial field set reset should invalidate the field set.");

    FGameStateDescriptor GameStateDescriptorValue;
    GameStateDescriptorValue.CurrentStep = 42U;
    GameStateDescriptorValue.MacroState.DesiredBaseCount = 3U;
    GameStateDescriptorValue.ArmyState.MinimumArmyCount = 2U;
    GameStateDescriptorValue.ArmyState.ArmyGoals.clear();
    GameStateDescriptorValue.ArmyState.ActiveArmyCount = 0U;
    GameStateDescriptorValue.BuildPlanning.ReservedVespene = 125U;
    GameStateDescriptorValue.SpatialFields.Resize(2U, 2U);
    GameStateDescriptorValue.Reset();
    Check(GameStateDescriptorValue.CurrentStep == 0U, SuccessValue,
          "Game state descriptor reset should clear the current step.");
    Check(GameStateDescriptorValue.MacroState.DesiredBaseCount == 1U, SuccessValue,
          "Game state descriptor reset should restore macro defaults.");
    Check(GameStateDescriptorValue.ArmyState.ActiveArmyCount == 1U, SuccessValue,
          "Game state descriptor reset should restore the primary army anchor.");
    Check(GameStateDescriptorValue.BuildPlanning.ReservedVespene == 0U, SuccessValue,
          "Game state descriptor reset should clear build planning reservations.");
    Check(GameStateDescriptorValue.CommandAuthoritySchedulingState.NextOrderId == 1U, SuccessValue,
          "Game state descriptor reset should restore the scheduling order identifier seed.");
    Check(GameStateDescriptorValue.CommandAuthoritySchedulingState.ProcessorState == EPlanningProcessorState::Idle,
          SuccessValue, "Game state descriptor reset should restore the scheduling processor state.");
    Check(GameStateDescriptorValue.CommandAuthoritySchedulingState.PlaybackState == EIntentPlaybackState::Idle,
          SuccessValue, "Game state descriptor reset should restore the scheduling playback state.");
    Check(!GameStateDescriptorValue.SpatialFields.IsValid(), SuccessValue,
          "Game state descriptor reset should clear spatial fields.");

    Check(std::string(ToString(EGamePlan::TimingAttack)) == "TimingAttack", SuccessValue,
          "Game plan string conversion should expose TimingAttack.");
    Check(std::string(ToString(EMacroPhase::MidGame)) == "MidGame", SuccessValue,
          "Macro phase string conversion should expose MidGame.");
    Check(std::string(ToString(EArmyGoal::HoldBase)) == "HoldBase", SuccessValue,
          "Army goal string conversion should expose HoldBase.");

    return SuccessValue;
}

}  // namespace sc2
