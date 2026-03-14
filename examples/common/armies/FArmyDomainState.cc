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
    PrimaryArmyAttackSupplyThreshold = 40;
    PrimaryArmyDisengageSupplyThreshold = 20;
    ArmyGoals.clear();
    ArmyPostures.clear();
    ArmyMissions.clear();
    EnsurePrimaryArmyExists();
}

void FArmyDomainState::EnsurePrimaryArmyExists()
{
    while (ArmyGoals.size() < MinimumArmyCount)
    {
        ArmyGoals.push_back(EArmyGoal::Unknown);
    }
    while (ArmyPostures.size() < MinimumArmyCount)
    {
        ArmyPostures.push_back(EArmyPosture::Assemble);
    }
    while (ArmyMissions.size() < MinimumArmyCount)
    {
        ArmyMissions.push_back(FArmyMissionDescriptor());
    }

    ActiveArmyCount = static_cast<uint32_t>(ArmyGoals.size());
}

}  // namespace sc2
