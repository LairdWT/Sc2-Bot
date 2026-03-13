#include "test_agent_execution_telemetry.h"

#include <iostream>

#include "common/planning/EIntentDomain.h"
#include "common/telemetry/EAgentExecutionEventType.h"
#include "common/telemetry/EExecutionConditionState.h"
#include "common/telemetry/FAgentExecutionTelemetry.h"

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

bool TestAgentExecutionTelemetry(int ArgC, char** ArgV)
{
    (void)ArgC;
    (void)ArgV;

    bool SuccessValue = true;

    FAgentExecutionTelemetry AgentExecutionTelemetryValue;
    Check(AgentExecutionTelemetryValue.SupplyBlockState == EExecutionConditionState::Inactive, SuccessValue,
          "Execution telemetry should default the supply-block state to inactive.");
    Check(AgentExecutionTelemetryValue.MineralBankState == EExecutionConditionState::Inactive, SuccessValue,
          "Execution telemetry should default the mineral-bank state to inactive.");

    AgentExecutionTelemetryValue.UpdateSupplyBlockState(EExecutionConditionState::Active, 10U, 100U);
    Check(AgentExecutionTelemetryValue.SupplyBlockState == EExecutionConditionState::Active, SuccessValue,
          "Execution telemetry should enter the active supply-block state.");
    Check(!AgentExecutionTelemetryValue.RecentEvents.empty() &&
              AgentExecutionTelemetryValue.RecentEvents.back().EventType ==
                  EAgentExecutionEventType::SupplyBlockedStarted,
          SuccessValue, "Execution telemetry should record the start of a supply block.");

    AgentExecutionTelemetryValue.UpdateSupplyBlockState(EExecutionConditionState::Inactive, 20U, 180U);
    Check(AgentExecutionTelemetryValue.LastSupplyBlockDurationGameLoops == 80U, SuccessValue,
          "Execution telemetry should record supply-block duration in game loops.");

    AgentExecutionTelemetryValue.UpdateMineralBankState(EExecutionConditionState::Active, 30U, 240U, 650U);
    Check(AgentExecutionTelemetryValue.MineralBankState == EExecutionConditionState::Active, SuccessValue,
          "Execution telemetry should enter the active mineral-bank state.");
    Check(AgentExecutionTelemetryValue.LastMineralBankAmount == 650U, SuccessValue,
          "Execution telemetry should remember the latest banked-mineral amount.");

    AgentExecutionTelemetryValue.UpdateMineralBankState(EExecutionConditionState::Inactive, 40U, 360U, 200U);
    Check(AgentExecutionTelemetryValue.LastMineralBankDurationGameLoops == 120U, SuccessValue,
          "Execution telemetry should record mineral-bank duration in game loops.");

    AgentExecutionTelemetryValue.RecordActorIntentConflict(50U, 400U, 101U, ABILITY_ID::BUILD_REFINERY,
                                                           EIntentDomain::StructureBuild);
    AgentExecutionTelemetryValue.RecordActorIntentConflict(51U, 401U, 101U, ABILITY_ID::BUILD_REFINERY,
                                                           EIntentDomain::StructureBuild);
    Check(AgentExecutionTelemetryValue.TotalActorIntentConflictCount == 1U, SuccessValue,
          "Execution telemetry should coalesce repeated actor conflicts inside the cooldown window.");

    AgentExecutionTelemetryValue.RecordIdleProductionConflict(60U, 500U, 202U, UNIT_TYPEID::TERRAN_BARRACKS,
                                                              ABILITY_ID::TRAIN_MARINE);
    AgentExecutionTelemetryValue.RecordIdleProductionConflict(61U, 501U, 202U, UNIT_TYPEID::TERRAN_BARRACKS,
                                                              ABILITY_ID::TRAIN_MARINE);
    Check(AgentExecutionTelemetryValue.TotalIdleProductionConflictCount == 1U, SuccessValue,
          "Execution telemetry should coalesce repeated idle-production conflicts inside the cooldown window.");

    AgentExecutionTelemetryValue.Reset();
    Check(AgentExecutionTelemetryValue.RecentEvents.empty(), SuccessValue,
          "Execution telemetry reset should clear recent events.");
    Check(AgentExecutionTelemetryValue.TotalActorIntentConflictCount == 0U, SuccessValue,
          "Execution telemetry reset should clear actor conflict counts.");
    Check(AgentExecutionTelemetryValue.TotalIdleProductionConflictCount == 0U, SuccessValue,
          "Execution telemetry reset should clear idle-production conflict counts.");

    return SuccessValue;
}

}  // namespace sc2
