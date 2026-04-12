#pragma once

#include <cstdint>

namespace sc2
{

class ObservationInterface;
struct FEnemyObservationDescriptor;

class IEnemyObservationBuilder
{
public:
    virtual ~IEnemyObservationBuilder() = default;

    virtual void RebuildEnemyObservation(const ObservationInterface& ObservationValue,
                                         uint64_t CurrentGameLoopValue,
                                         FEnemyObservationDescriptor& EnemyObservationDescriptorValue) const = 0;
};

}  // namespace sc2
