#include "common/services/FTerranBuildPlacementService.h"

#include <array>

namespace sc2
{
namespace
{

template <size_t CandidateCount>
void AppendOffsetCandidates(const Point2D& AnchorValue, const std::array<Point2D, CandidateCount>& OffsetValues,
                            std::vector<Point2D>& OutCandidates)
{
    OutCandidates.reserve(OffsetValues.size());
    for (const Point2D& OffsetValue : OffsetValues)
    {
        OutCandidates.push_back(Point2D(AnchorValue.x + OffsetValue.x, AnchorValue.y + OffsetValue.y));
    }
}

}  // namespace

Point2D FTerranBuildPlacementService::GetPrimaryStructureAnchor(const FGameStateDescriptor& GameStateDescriptorValue,
                                                                const Point2D& BaseLocationValue) const
{
    (void)GameStateDescriptorValue;
    return BaseLocationValue;
}

std::vector<Point2D> FTerranBuildPlacementService::GetStructurePlacementCandidates(
    const FGameStateDescriptor& GameStateDescriptorValue, const ABILITY_ID StructureAbilityId,
    const Point2D& BaseLocationValue) const
{
    const Point2D PrimaryAnchorValue = GetPrimaryStructureAnchor(GameStateDescriptorValue, BaseLocationValue);
    std::vector<Point2D> CandidateValues;

    static const std::array<Point2D, 12> SupplyDepotOffsetValues =
    {
        Point2D(6.0f, 4.0f),
        Point2D(8.0f, 4.0f),
        Point2D(6.0f, -4.0f),
        Point2D(8.0f, -4.0f),
        Point2D(-6.0f, 4.0f),
        Point2D(-8.0f, 4.0f),
        Point2D(-6.0f, -4.0f),
        Point2D(-8.0f, -4.0f),
        Point2D(4.0f, 8.0f),
        Point2D(-4.0f, 8.0f),
        Point2D(4.0f, -8.0f),
        Point2D(-4.0f, -8.0f),
    };

    static const std::array<Point2D, 12> BarracksOffsetValues =
    {
        Point2D(10.0f, 8.0f),
        Point2D(14.0f, 8.0f),
        Point2D(10.0f, -8.0f),
        Point2D(14.0f, -8.0f),
        Point2D(-10.0f, 8.0f),
        Point2D(-14.0f, 8.0f),
        Point2D(-10.0f, -8.0f),
        Point2D(-14.0f, -8.0f),
        Point2D(8.0f, 12.0f),
        Point2D(-8.0f, 12.0f),
        Point2D(8.0f, -12.0f),
        Point2D(-8.0f, -12.0f),
    };

    switch (StructureAbilityId)
    {
        case ABILITY_ID::BUILD_SUPPLYDEPOT:
            AppendOffsetCandidates(PrimaryAnchorValue, SupplyDepotOffsetValues, CandidateValues);
            break;
        case ABILITY_ID::BUILD_BARRACKS:
            AppendOffsetCandidates(PrimaryAnchorValue, BarracksOffsetValues, CandidateValues);
            break;
        default:
            CandidateValues.push_back(PrimaryAnchorValue);
            break;
    }

    return CandidateValues;
}

}  // namespace sc2
