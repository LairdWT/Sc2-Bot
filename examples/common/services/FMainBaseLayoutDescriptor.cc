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
    LayoutAnchorPoint = Point2D();
    NaturalApproachDepotSlots.clear();
    SupportDepotSlots.clear();
    BarracksWithAddonSlots.clear();
    FactoryWithAddonSlots.clear();
    StarportWithAddonSlots.clear();
}

}  // namespace sc2
