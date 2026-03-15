#pragma once

#include <cstdint>

namespace sc2
{

enum class ECommandTaskRetentionPolicy : uint8_t
{
    HotMustRun,
    BufferedRetry,
    DiscardableDuplicate,
};

const char* ToString(ECommandTaskRetentionPolicy CommandTaskRetentionPolicyValue);

}  // namespace sc2
