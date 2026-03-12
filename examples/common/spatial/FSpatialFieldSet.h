#pragma once

#include <cstdint>
#include <vector>

namespace sc2
{

struct FSpatialFieldSet
{
    uint32_t Width;
    uint32_t Height;
    std::vector<float> EnemyGroundThreat;
    std::vector<float> FriendlyGroundInfluence;
    std::vector<float> RetreatSafety;
    std::vector<float> EngageOpportunity;

    FSpatialFieldSet();

    void Reset();
    void Resize(const uint32_t WidthValue, const uint32_t HeightValue);
    bool IsValid() const;
};

}  // namespace sc2
