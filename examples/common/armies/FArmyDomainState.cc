#include "common/armies/FArmyDomainState.h"

namespace sc2
{

FArmyDomainState::FArmyDomainState()
{
    Reset();
}

void FArmyDomainState::Reset()
{
    MinimumArmyCount = 1;
    ActiveArmyCount = 0;
    ActiveSquadCount = 0;
    ReserveUnitCount = 0;
    ArmyGoals.clear();
    EnsurePrimaryArmyExists();
}

void FArmyDomainState::EnsurePrimaryArmyExists()
{
    while (ArmyGoals.size() < MinimumArmyCount)
    {
        ArmyGoals.push_back(EArmyGoal::Unknown);
    }

    ActiveArmyCount = static_cast<uint32_t>(ArmyGoals.size());
}

}  // namespace sc2
