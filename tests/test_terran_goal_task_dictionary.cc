#include "test_terran_goal_task_dictionary.h"

#include <iostream>
#include <string>

#include "common/build_orders/FOpeningPlanRegistry.h"
#include "common/catalogs/EGoalTargetRuleId.h"
#include "common/catalogs/ETerranGoalDefinitionId.h"
#include "common/catalogs/ETerranTaskTemplateId.h"
#include "common/catalogs/FTerranGoalDefinition.h"
#include "common/catalogs/FTerranGoalDictionary.h"
#include "common/catalogs/FTerranTaskTemplateDefinition.h"
#include "common/catalogs/FTerranTaskTemplateDictionary.h"
#include "common/planning/ECommandCommitmentClass.h"
#include "common/planning/ECommandTaskExecutionGuarantee.h"
#include "common/planning/ECommandTaskOrigin.h"
#include "common/planning/ECommandTaskRetentionPolicy.h"

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

}  // namespace

bool TestTerranGoalTaskDictionary(int ArgC, char** ArgV)
{
    (void)ArgC;
    (void)ArgV;

    bool SuccessValue = true;

    Check(FTerranGoalDictionary::GetDefinitionCount() == 28U, SuccessValue,
          "Terran goal dictionary should expose the authored twenty-eight goal definitions.");
    Check(FTerranTaskTemplateDictionary::GetDefinitionCount() == 29U, SuccessValue,
          "Terran task-template dictionary should expose the authored twenty-nine task templates.");

    const FTerranGoalDefinition* SaturateWorkersDefinitionValue = FTerranGoalDictionary::TryGetByGoalId(110U);
    Check(SaturateWorkersDefinitionValue != nullptr, SuccessValue,
          "Terran goal dictionary should resolve the SaturateWorkers goal by numeric goal id.");
    if (SaturateWorkersDefinitionValue != nullptr)
    {
        Check(SaturateWorkersDefinitionValue->DefinitionId == ETerranGoalDefinitionId::SaturateWorkers,
              SuccessValue, "SaturateWorkers goal lookup should preserve the generated definition id.");
        Check(SaturateWorkersDefinitionValue->TaskTemplateId == ETerranTaskTemplateId::TrainScv, SuccessValue,
              "SaturateWorkers should bind to the TrainScv task template.");
        Check(SaturateWorkersDefinitionValue->TargetRuleId == EGoalTargetRuleId::DesiredWorkerCount, SuccessValue,
              "SaturateWorkers should preserve the generated worker-target rule.");
    }

    const FTerranGoalDefinition* FactoryTechLabDefinitionValue =
        FTerranGoalDictionary::TryGetByDefinitionId(ETerranGoalDefinitionId::UnlockFactoryTechLab);
    Check(FactoryTechLabDefinitionValue != nullptr, SuccessValue,
          "Terran goal dictionary should resolve UnlockFactoryTechLab by generated definition id.");
    if (FactoryTechLabDefinitionValue != nullptr)
    {
        Check(FactoryTechLabDefinitionValue->GoalId == 231U, SuccessValue,
              "UnlockFactoryTechLab should preserve the authored numeric goal id.");
        Check(FactoryTechLabDefinitionValue->DefaultTargetUnitTypeId == UNIT_TYPEID::TERRAN_FACTORYTECHLAB,
              SuccessValue, "UnlockFactoryTechLab should preserve the authored factory-tech-lab target type.");
    }

    const FTerranGoalDefinition* BarracksTechLabDefinitionValue =
        FTerranGoalDictionary::TryGetByDefinitionId(ETerranGoalDefinitionId::UnlockBarracksTechLab);
    Check(BarracksTechLabDefinitionValue != nullptr, SuccessValue,
          "Terran goal dictionary should resolve UnlockBarracksTechLab by generated definition id.");
    if (BarracksTechLabDefinitionValue != nullptr)
    {
        Check(BarracksTechLabDefinitionValue->GoalId == 234U, SuccessValue,
              "UnlockBarracksTechLab should preserve the authored numeric goal id.");
        Check(BarracksTechLabDefinitionValue->DefaultTargetUnitTypeId == UNIT_TYPEID::TERRAN_BARRACKSTECHLAB,
              SuccessValue, "UnlockBarracksTechLab should preserve the authored barracks-tech-lab target type.");
    }

    const FTerranGoalDefinition* SecondEngineeringBayDefinitionValue =
        FTerranGoalDictionary::TryGetByDefinitionId(ETerranGoalDefinitionId::UnlockSecondEngineeringBay);
    Check(SecondEngineeringBayDefinitionValue != nullptr, SuccessValue,
          "Terran goal dictionary should resolve UnlockSecondEngineeringBay by generated definition id.");
    if (SecondEngineeringBayDefinitionValue != nullptr)
    {
        Check(SecondEngineeringBayDefinitionValue->DefaultTargetCount == 2U, SuccessValue,
              "UnlockSecondEngineeringBay should preserve the authored second-bay target count.");
    }

    const FTerranTaskTemplateDefinition* SupplyDepotTemplateValue =
        FTerranTaskTemplateDictionary::TryGetByTemplateId(ETerranTaskTemplateId::BuildSupplyDepot);
    Check(SupplyDepotTemplateValue != nullptr, SuccessValue,
          "Terran task-template dictionary should resolve BuildSupplyDepot by template id.");
    if (SupplyDepotTemplateValue != nullptr)
    {
        Check(SupplyDepotTemplateValue->ActionAbilityId == ABILITY_ID::BUILD_SUPPLYDEPOT, SuccessValue,
              "BuildSupplyDepot template should preserve the authored ability id.");
        Check(SupplyDepotTemplateValue->ActionProducerUnitTypeId == UNIT_TYPEID::TERRAN_SCV, SuccessValue,
              "BuildSupplyDepot template should preserve the authored producer type.");
        Check(SupplyDepotTemplateValue->TaskType == ECommandTaskType::Supply, SuccessValue,
              "BuildSupplyDepot template should preserve the authored supply task type.");
    }

    const FTerranTaskTemplateDefinition* StimpackTemplateValue =
        FTerranTaskTemplateDictionary::TryFindByAction(ABILITY_ID::RESEARCH_STIMPACK, UNIT_TYPEID::INVALID,
                                                       UpgradeID(UPGRADE_ID::STIMPACK));
    Check(StimpackTemplateValue != nullptr, SuccessValue,
          "Terran task-template dictionary should resolve ResearchStimpack by action tuple.");
    if (StimpackTemplateValue != nullptr)
    {
        Check(StimpackTemplateValue->TemplateId == ETerranTaskTemplateId::ResearchStimpack, SuccessValue,
              "ResearchStimpack action lookup should resolve the generated task-template id.");
    }

    const FTerranTaskTemplateDefinition* InfantryArmorTemplateValue =
        FTerranTaskTemplateDictionary::TryGetByTemplateId(ETerranTaskTemplateId::ResearchTerranInfantryArmorLevel1);
    Check(InfantryArmorTemplateValue != nullptr, SuccessValue,
          "Terran task-template dictionary should resolve ResearchTerranInfantryArmorLevel1 by template id.");
    if (InfantryArmorTemplateValue != nullptr)
    {
        Check(InfantryArmorTemplateValue->ActionAbilityId == ABILITY_ID::RESEARCH_TERRANINFANTRYARMORLEVEL1,
              SuccessValue, "ResearchTerranInfantryArmorLevel1 should preserve the authored armor-research ability.");
        Check(InfantryArmorTemplateValue->ActionUpgradeId == UpgradeID(UPGRADE_ID::TERRANINFANTRYARMORSLEVEL1),
              SuccessValue, "ResearchTerranInfantryArmorLevel1 should preserve the authored upgrade id.");
    }

    FCommandTaskDescriptor FactoryTechLabTaskDescriptorValue;
    const bool CreatedFactoryTechLabTaskDescriptorValue =
        FTerranTaskTemplateDictionary::TryCreateTaskDescriptor(ETerranTaskTemplateId::BuildFactoryTechLab,
                                                               FactoryTechLabTaskDescriptorValue);
    Check(CreatedFactoryTechLabTaskDescriptorValue, SuccessValue,
          "Terran task-template dictionary should materialize a BuildFactoryTechLab task descriptor.");
    if (CreatedFactoryTechLabTaskDescriptorValue)
    {
        Check(FactoryTechLabTaskDescriptorValue.ActionAbilityId == ABILITY_ID::BUILD_TECHLAB_FACTORY,
              SuccessValue, "BuildFactoryTechLab task descriptor should preserve the template ability.");
        Check(FactoryTechLabTaskDescriptorValue.TaskType == ECommandTaskType::AddOn, SuccessValue,
              "BuildFactoryTechLab task descriptor should preserve the template task type.");
        Check(FactoryTechLabTaskDescriptorValue.CommitmentClass == ECommandCommitmentClass::FlexibleMacro,
              SuccessValue, "Task-template materialization should preserve default commitment metadata.");
    }

    const FOpeningPlanDescriptor& OpeningPlanDescriptorValue =
        FOpeningPlanRegistry::GetOpeningPlanDescriptor(EOpeningPlanId::TerranTwoBaseMMMFrameOpening);
    const FCommandTaskDescriptor& WallBarracksTaskDescriptorValue = OpeningPlanDescriptorValue.Steps[1].TaskDescriptor;
    Check(WallBarracksTaskDescriptorValue.ActionAbilityId == ABILITY_ID::BUILD_BARRACKS, SuccessValue,
          "Template-backed opening step 2 should preserve the wall barracks build ability.");
    Check(WallBarracksTaskDescriptorValue.TaskType == ECommandTaskType::ProductionStructure, SuccessValue,
          "Template-backed opening step 2 should preserve the production-structure task type.");
    Check(WallBarracksTaskDescriptorValue.Origin == ECommandTaskOrigin::Opening, SuccessValue,
          "Opening step 2 should still override template origin to Opening.");
    Check(WallBarracksTaskDescriptorValue.CommitmentClass == ECommandCommitmentClass::MandatoryOpening,
          SuccessValue, "Exact-slot opening wall barracks should remain mandatory after template refactor.");
    Check(WallBarracksTaskDescriptorValue.ExecutionGuarantee == ECommandTaskExecutionGuarantee::MustExecute,
          SuccessValue, "Exact-slot opening wall barracks should remain must-execute after template refactor.");
    Check(WallBarracksTaskDescriptorValue.RetentionPolicy == ECommandTaskRetentionPolicy::HotMustRun, SuccessValue,
          "Exact-slot opening wall barracks should remain hot-must-run after template refactor.");

    const FCommandTaskDescriptor& FactoryTechLabOpeningTaskDescriptorValue =
        OpeningPlanDescriptorValue.Steps[18].TaskDescriptor;
    Check(FactoryTechLabOpeningTaskDescriptorValue.ActionAbilityId == ABILITY_ID::BUILD_TECHLAB_FACTORY,
          SuccessValue, "Template-backed opening step 19 should preserve the factory-tech-lab ability.");
    Check(FactoryTechLabOpeningTaskDescriptorValue.TaskType == ECommandTaskType::AddOn, SuccessValue,
          "Template-backed opening step 19 should preserve the add-on task type.");
    Check(FactoryTechLabOpeningTaskDescriptorValue.Origin == ECommandTaskOrigin::Opening, SuccessValue,
          "Template-backed opening step 19 should still report opening origin metadata.");

    return SuccessValue;
}

}  // namespace sc2
