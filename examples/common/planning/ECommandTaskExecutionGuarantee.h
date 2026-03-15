#pragma once

#include <cstdint>

namespace sc2
{

enum class ECommandTaskExecutionGuarantee : uint8_t
{
    MustExecute,
    Preferred,
    Skippable,
};

const char* ToString(ECommandTaskExecutionGuarantee CommandTaskExecutionGuaranteeValue);

}  // namespace sc2
