#include "common/catalogs/FTerranGoalDefinition.h"

namespace sc2
{

FTerranGoalDefinition::FTerranGoalDefinition()
{
    Reset();
}

FTerranGoalDefinition::FTerranGoalDefinition(const ETerranGoalDefinitionId DefinitionIdValue,
                                             const uint32_t GoalIdValue, const char* DisplayNameValue,
                                             const EGoalDomain GoalDomainValue,
                                             const EGoalHorizon GoalHorizonValue,
                                             const EGoalType GoalTypeValue, const int BasePriorityValueValue,
                                             const EGoalActivationRuleId ActivationRuleIdValue,
                                             const EGoalTargetRuleId TargetRuleIdValue,
                                             const uint32_t DefaultTargetCountValue,
                                             const UNIT_TYPEID DefaultTargetUnitTypeIdValue,
                                             const UpgradeID DefaultTargetUpgradeIdValue,
                                             const ETerranTaskTemplateId TaskTemplateIdValue)
{
    DefinitionId = DefinitionIdValue;
    GoalId = GoalIdValue;
    DisplayName = DisplayNameValue;
    GoalDomain = GoalDomainValue;
    GoalHorizon = GoalHorizonValue;
    GoalType = GoalTypeValue;
    BasePriorityValue = BasePriorityValueValue;
    ActivationRuleId = ActivationRuleIdValue;
    TargetRuleId = TargetRuleIdValue;
    DefaultTargetCount = DefaultTargetCountValue;
    DefaultTargetUnitTypeId = DefaultTargetUnitTypeIdValue;
    DefaultTargetUpgradeId = DefaultTargetUpgradeIdValue;
    TaskTemplateId = TaskTemplateIdValue;
}

void FTerranGoalDefinition::Reset()
{
    DefinitionId = ETerranGoalDefinitionId::Invalid;
    GoalId = 0U;
    DisplayName = "";
    GoalDomain = EGoalDomain::Economy;
    GoalHorizon = EGoalHorizon::Immediate;
    GoalType = EGoalType::HoldOwnedBase;
    BasePriorityValue = 0;
    ActivationRuleId = EGoalActivationRuleId::Invalid;
    TargetRuleId = EGoalTargetRuleId::Invalid;
    DefaultTargetCount = 0U;
    DefaultTargetUnitTypeId = UNIT_TYPEID::INVALID;
    DefaultTargetUpgradeId = UpgradeID(UPGRADE_ID::INVALID);
    TaskTemplateId = ETerranTaskTemplateId::Invalid;
}

}  // namespace sc2
