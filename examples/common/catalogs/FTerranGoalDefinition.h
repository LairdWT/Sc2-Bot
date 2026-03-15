#pragma once

#include <cstdint>

#include "common/catalogs/EGoalActivationRuleId.h"
#include "common/catalogs/EGoalTargetRuleId.h"
#include "common/catalogs/ETerranGoalDefinitionId.h"
#include "common/catalogs/ETerranTaskTemplateId.h"
#include "common/goals/EGoalDomain.h"
#include "common/goals/EGoalHorizon.h"
#include "common/goals/EGoalType.h"
#include "sc2api/sc2_typeenums.h"

namespace sc2
{

struct FTerranGoalDefinition
{
public:
    FTerranGoalDefinition();
    FTerranGoalDefinition(ETerranGoalDefinitionId DefinitionIdValue, uint32_t GoalIdValue,
                          const char* DisplayNameValue, EGoalDomain GoalDomainValue,
                          EGoalHorizon GoalHorizonValue, EGoalType GoalTypeValue, int BasePriorityValue,
                          EGoalActivationRuleId ActivationRuleIdValue, EGoalTargetRuleId TargetRuleIdValue,
                          uint32_t DefaultTargetCountValue, UNIT_TYPEID DefaultTargetUnitTypeIdValue,
                          UpgradeID DefaultTargetUpgradeIdValue, ETerranTaskTemplateId TaskTemplateIdValue);

    void Reset();

public:
    ETerranGoalDefinitionId DefinitionId;
    uint32_t GoalId;
    const char* DisplayName;
    EGoalDomain GoalDomain;
    EGoalHorizon GoalHorizon;
    EGoalType GoalType;
    int BasePriorityValue;
    EGoalActivationRuleId ActivationRuleId;
    EGoalTargetRuleId TargetRuleId;
    uint32_t DefaultTargetCount;
    UNIT_TYPEID DefaultTargetUnitTypeId;
    UpgradeID DefaultTargetUpgradeId;
    ETerranTaskTemplateId TaskTemplateId;
};

}  // namespace sc2
