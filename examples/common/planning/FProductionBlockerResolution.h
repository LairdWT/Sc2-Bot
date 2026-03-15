#pragma once

#include <cstdint>

#include "common/planning/EProductionBlockerKind.h"

namespace sc2
{

struct FProductionBlockerResolution
{
public:
    FProductionBlockerResolution();

    void Reset();

public:
    EProductionBlockerKind BlockerKind;
    bool bCanAttemptRelief;
    bool bCanRelocate;
    uint64_t RetryNotBeforeGameLoop;
    uint32_t RelocationAttemptCount;
    uint64_t BlockerFingerprint;
    uint32_t SoftBlockWindowCount;
};

}  // namespace sc2
