#include "common/planning/FProductionBlockerResolution.h"

namespace sc2
{

FProductionBlockerResolution::FProductionBlockerResolution()
{
    Reset();
}

void FProductionBlockerResolution::Reset()
{
    BlockerKind = EProductionBlockerKind::None;
    bCanAttemptRelief = false;
    bCanRelocate = false;
    RetryNotBeforeGameLoop = 0U;
    RelocationAttemptCount = 0U;
    BlockerFingerprint = 0U;
    SoftBlockWindowCount = 0U;
}

}  // namespace sc2
