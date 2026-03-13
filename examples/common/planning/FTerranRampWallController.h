#pragma once

#include "common/planning/IWallGateController.h"

namespace sc2
{

class FTerranRampWallController : public IWallGateController
{
public:
    EWallGateState EvaluateDesiredWallGateState(const Units& SelfUnitsValue, const Units& EnemyUnitsValue,
                                                const FRampWallDescriptor& RampWallDescriptorValue) const final;
    void ProduceWallGateIntents(const Units& SelfUnitsValue, const FRampWallDescriptor& RampWallDescriptorValue,
                                EWallGateState DesiredWallGateStateValue,
                                FIntentBuffer& IntentBufferValue) const final;
};

}  // namespace sc2
