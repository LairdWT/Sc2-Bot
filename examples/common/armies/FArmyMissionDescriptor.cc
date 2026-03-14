#include "common/armies/FArmyMissionDescriptor.h"

namespace sc2
{

FArmyMissionDescriptor::FArmyMissionDescriptor()
{
    Reset();
}

void FArmyMissionDescriptor::Reset()
{
    MissionType = EArmyMissionType::AssembleAtRally;
    SourceGoalId = 0U;
    ObjectivePoint = Point2D();
    ObjectiveRadius = 0.0f;
    SearchExpansionOrdinal = 0U;
}

}  // namespace sc2
