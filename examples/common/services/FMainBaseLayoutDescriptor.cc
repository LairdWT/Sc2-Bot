#include "common/services/FMainBaseLayoutDescriptor.h"

namespace sc2
{

FMainBaseLayoutDescriptor::FMainBaseLayoutDescriptor()
{
    Reset();
}

void FMainBaseLayoutDescriptor::Reset()
{
    bIsValid = false;
    bUsesAuthoredProductionLayout = false;
    LayoutAnchorPoint = Point2D();
    ArmyAssemblyAnchorPoint = Point2D();
    NaturalEntranceArmyRallyAnchorPoint = Point2D();
    ProductionClearanceAnchorPoint = Point2D();
    NaturalEntranceWallDepotSlots.clear();
    NaturalApproachDepotSlots.clear();
    SupportDepotSlots.clear();
    PeripheralDepotSlots.clear();
    ProductionRailWithAddonSlots.clear();
    BarracksWithAddonSlots.clear();
    FactoryWithAddonSlots.clear();
    StarportWithAddonSlots.clear();
}

}  // namespace sc2
