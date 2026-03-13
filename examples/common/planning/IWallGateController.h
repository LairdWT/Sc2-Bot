#pragma once

#include "common/agent_framework.h"
#include "common/planning/EWallGateState.h"
#include "common/services/FRampWallDescriptor.h"

namespace sc2
{

class IWallGateController
{
public:
    virtual ~IWallGateController();

    virtual EWallGateState EvaluateDesiredWallGateState(const Units& SelfUnitsValue, const Units& EnemyUnitsValue,
                                                        const FRampWallDescriptor& RampWallDescriptorValue) const = 0;
    virtual void ProduceWallGateIntents(const Units& SelfUnitsValue, const FRampWallDescriptor& RampWallDescriptorValue,
                                        EWallGateState DesiredWallGateStateValue,
                                        FIntentBuffer& IntentBufferValue) const = 0;
};

}  // namespace sc2
