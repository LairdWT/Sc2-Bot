#pragma once

#include "common/services/FBuildPlacementContext.h"
#include "common/services/FMainBaseLayoutDescriptor.h"

namespace sc2
{

struct GameInfo;

class FTerranMainBaseLayoutRegistry
{
public:
    bool TryGetAuthoredMainBaseLayout(const GameInfo* GameInfoPtrValue,
                                      const FBuildPlacementContext& BuildPlacementContextValue,
                                      FMainBaseLayoutDescriptor& OutMainBaseLayoutDescriptorValue) const;
};

}  // namespace sc2
