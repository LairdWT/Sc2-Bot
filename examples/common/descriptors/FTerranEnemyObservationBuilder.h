#pragma once

#include "common/descriptors/IEnemyObservationBuilder.h"

namespace sc2
{

class FTerranEnemyObservationBuilder : public IEnemyObservationBuilder
{
public:
    FTerranEnemyObservationBuilder() = default;
    ~FTerranEnemyObservationBuilder() override = default;

    void RebuildEnemyObservation(const ObservationInterface& ObservationValue,
                                 uint64_t CurrentGameLoopValue,
                                 FEnemyObservationDescriptor& EnemyObservationDescriptorValue) const override;
};

}  // namespace sc2
