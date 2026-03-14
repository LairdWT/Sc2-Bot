#pragma once

#include <cstdint>

#include "common/planning/FCommandTaskDescriptor.h"

namespace sc2
{

struct FOpeningPlanStep
{
public:
    FOpeningPlanStep();

    void Reset();

public:
    FCommandTaskDescriptor TaskDescriptor;
};

}  // namespace sc2
