#pragma once

namespace sc2
{

enum class EExecutionConditionState
{
    Inactive,
    Active,
};

const char* ToString(EExecutionConditionState ExecutionConditionStateValue);

}  // namespace sc2
