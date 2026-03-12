#include "test_terran_build_placement_service.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "common/descriptors/FGameStateDescriptor.h"
#include "common/services/FTerranBuildPlacementService.h"

namespace sc2
{
namespace
{

bool Check(const bool Condition, bool& Success, const std::string& Message)
{
    if (!Condition)
    {
        Success = false;
        std::cerr << "    " << Message << std::endl;
    }

    return Condition;
}

bool ArePointsEqual(const Point2D& LeftValue, const Point2D& RightValue)
{
    constexpr float CoordinateTolerance = 0.001f;
    return std::fabs(LeftValue.x - RightValue.x) <= CoordinateTolerance &&
           std::fabs(LeftValue.y - RightValue.y) <= CoordinateTolerance;
}

bool ContainsPoint(const std::vector<Point2D>& PointValues, const Point2D& TargetPointValue)
{
    for (const Point2D& PointValue : PointValues)
    {
        if (ArePointsEqual(PointValue, TargetPointValue))
        {
            return true;
        }
    }

    return false;
}

}  // namespace

bool TestTerranBuildPlacementService(int ArgC, char** ArgV)
{
    (void)ArgC;
    (void)ArgV;

    bool Success = true;

    FGameStateDescriptor GameStateDescriptorValue;
    const Point2D BaseLocationValue(50.0f, 50.0f);
    FTerranBuildPlacementService BuildPlacementServiceValue;

    const Point2D PrimaryAnchorValue =
        BuildPlacementServiceValue.GetPrimaryStructureAnchor(GameStateDescriptorValue, BaseLocationValue);
    Check(ArePointsEqual(PrimaryAnchorValue, BaseLocationValue), Success,
          "Primary structure anchor should currently remain base-relative and deterministic.");

    const std::vector<Point2D> SupplyDepotCandidates =
        BuildPlacementServiceValue.GetStructurePlacementCandidates(GameStateDescriptorValue,
                                                                  ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                                  BaseLocationValue);
    Check(SupplyDepotCandidates.size() == 12, Success,
          "Supply depot placement should expose the full deterministic candidate set.");
    Check(!SupplyDepotCandidates.empty() &&
              ArePointsEqual(SupplyDepotCandidates.front(), Point2D(BaseLocationValue.x + 6.0f, BaseLocationValue.y + 4.0f)),
          Success, "Supply depot placement should use the documented first candidate offset.");
    Check(ContainsPoint(SupplyDepotCandidates, Point2D(BaseLocationValue.x - 6.0f, BaseLocationValue.y - 4.0f)), Success,
          "Supply depot placement should search across multiple quadrants instead of only positive offsets.");

    const std::vector<Point2D> BarracksCandidates =
        BuildPlacementServiceValue.GetStructurePlacementCandidates(GameStateDescriptorValue,
                                                                  ABILITY_ID::BUILD_BARRACKS,
                                                                  BaseLocationValue);
    Check(BarracksCandidates.size() == 12, Success,
          "Barracks placement should expose the full deterministic candidate set.");
    Check(!BarracksCandidates.empty() &&
              ArePointsEqual(BarracksCandidates.front(), Point2D(BaseLocationValue.x + 10.0f, BaseLocationValue.y + 8.0f)),
          Success, "Barracks placement should use the documented first candidate offset.");

    const std::vector<Point2D> FallbackCandidates =
        BuildPlacementServiceValue.GetStructurePlacementCandidates(GameStateDescriptorValue,
                                                                  ABILITY_ID::INVALID,
                                                                  BaseLocationValue);
    Check(FallbackCandidates.size() == 1 && ArePointsEqual(FallbackCandidates.front(), BaseLocationValue), Success,
          "Unsupported structure abilities should fall back to the primary anchor without random scatter.");

    return Success;
}

}  // namespace sc2
