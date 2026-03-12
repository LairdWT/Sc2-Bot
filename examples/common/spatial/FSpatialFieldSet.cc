#include "common/spatial/FSpatialFieldSet.h"

#include <cstddef>

namespace sc2
{

FSpatialFieldSet::FSpatialFieldSet()
{
    Reset();
}

void FSpatialFieldSet::Reset()
{
    Width = 0;
    Height = 0;
    EnemyGroundThreat.clear();
    FriendlyGroundInfluence.clear();
    RetreatSafety.clear();
    EngageOpportunity.clear();
}

void FSpatialFieldSet::Resize(const uint32_t WidthValue, const uint32_t HeightValue)
{
    Width = WidthValue;
    Height = HeightValue;

    const size_t FieldCount = static_cast<size_t>(WidthValue) * static_cast<size_t>(HeightValue);

    EnemyGroundThreat.assign(FieldCount, 0.0f);
    FriendlyGroundInfluence.assign(FieldCount, 0.0f);
    RetreatSafety.assign(FieldCount, 0.0f);
    EngageOpportunity.assign(FieldCount, 0.0f);
}

bool FSpatialFieldSet::IsValid() const
{
    if (Width == 0 || Height == 0)
    {
        return false;
    }

    const size_t ExpectedFieldCount = static_cast<size_t>(Width) * static_cast<size_t>(Height);

    return EnemyGroundThreat.size() == ExpectedFieldCount &&
           FriendlyGroundInfluence.size() == ExpectedFieldCount &&
           RetreatSafety.size() == ExpectedFieldCount &&
           EngageOpportunity.size() == ExpectedFieldCount;
}

}  // namespace sc2
