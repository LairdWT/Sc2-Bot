#include "common/descriptors/FTerranEnemyObservationBuilder.h"

#include "common/descriptors/FEnemyObservationDescriptor.h"
#include "sc2api/sc2_interfaces.h"

namespace sc2
{

void FTerranEnemyObservationBuilder::RebuildEnemyObservation(
    const ObservationInterface& ObservationValue,
    const uint64_t CurrentGameLoopValue,
    FEnemyObservationDescriptor& EnemyObservationDescriptorValue) const
{
    EnemyObservationDescriptorValue.CurrentGameLoop = CurrentGameLoopValue;

    const Units EnemyUnitsValue = ObservationValue.GetUnits(Unit::Alliance::Enemy);
    for (const Unit* EnemyUnitValue : EnemyUnitsValue)
    {
        if (EnemyUnitValue == nullptr)
        {
            continue;
        }

        EnemyObservationDescriptorValue.AddOrUpdateUnit(*EnemyUnitValue, CurrentGameLoopValue);
    }

    if (!EnemyUnitsValue.empty())
    {
        EnemyObservationDescriptorValue.LastFullObservationGameLoop = CurrentGameLoopValue;
    }

    EnemyObservationDescriptorValue.PruneStaleEntries(
        CurrentGameLoopValue, FEnemyObservationDescriptor::DefaultStaleEntryThresholdGameLoopsValue);
    EnemyObservationDescriptorValue.RebuildCompositionSummary();
}

}  // namespace sc2
