#pragma once

#include <vector>

#include "common/descriptors/FGameStateDescriptor.h"
#include "sc2api/sc2_common.h"

namespace sc2
{

class IExpansionSelectionService
{
public:
    virtual ~IExpansionSelectionService();

    virtual Point2D SelectNextExpansionLocation(const FGameStateDescriptor& GameStateDescriptorValue,
                                                const std::vector<Point2D>& CandidateExpansionLocationsValue) const = 0;
};

}  // namespace sc2
