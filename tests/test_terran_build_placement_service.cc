#include "test_terran_build_placement_service.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

#include "common/services/EBuildPlacementFootprintPolicy.h"
#include "common/services/EBuildPlacementSlotType.h"
#include "common/services/FBuildPlacementContext.h"
#include "common/services/FBuildPlacementSlot.h"
#include "common/descriptors/FGameStateDescriptor.h"
#include "common/services/FTerranBuildPlacementService.h"
#include "sc2api/sc2_interfaces.h"

namespace sc2
{
namespace
{

struct FFakePlacementQuery : QueryInterface
{
    AvailableAbilities GetAbilitiesForUnit(const Unit* UnitPtrValue, bool IgnoreResourceRequirementsValue = false,
                                           bool UseGeneralizedAbilityValue = true) override
    {
        (void)UnitPtrValue;
        (void)IgnoreResourceRequirementsValue;
        (void)UseGeneralizedAbilityValue;
        return {};
    }

    std::vector<AvailableAbilities> GetAbilitiesForUnits(const Units& UnitsToQueryValue,
                                                         bool IgnoreResourceRequirementsValue = false,
                                                         bool UseGeneralizedAbilityValue = true) override
    {
        (void)IgnoreResourceRequirementsValue;
        (void)UseGeneralizedAbilityValue;
        return std::vector<AvailableAbilities>(UnitsToQueryValue.size());
    }

    float PathingDistance(const Point2D& StartPointValue, const Point2D& EndPointValue) override
    {
        (void)StartPointValue;
        (void)EndPointValue;
        return 1.0f;
    }

    float PathingDistance(const Unit* StartUnitValue, const Point2D& EndPointValue) override
    {
        (void)StartUnitValue;
        (void)EndPointValue;
        return 1.0f;
    }

    std::vector<float> PathingDistance(const std::vector<PathingQuery>& QueriesValue) override
    {
        return std::vector<float>(QueriesValue.size(), 1.0f);
    }

    bool Placement(const AbilityID& AbilityIdValue, const Point2D& TargetPointValue,
                   const Unit* UnitPtrValue = nullptr) override
    {
        (void)AbilityIdValue;
        (void)TargetPointValue;
        (void)UnitPtrValue;
        return true;
    }

    std::vector<bool> Placement(const std::vector<PlacementQuery>& QueriesValue) override
    {
        return std::vector<bool>(QueriesValue.size(), true);
    }
};

struct FSelectivePlacementQuery : QueryInterface
{
    std::vector<Point2D> RejectedPlacementPoints;
    float RejectedPlacementRadiusValue = 0.001f;

    AvailableAbilities GetAbilitiesForUnit(const Unit* UnitPtrValue, bool IgnoreResourceRequirementsValue = false,
                                           bool UseGeneralizedAbilityValue = true) override
    {
        (void)UnitPtrValue;
        (void)IgnoreResourceRequirementsValue;
        (void)UseGeneralizedAbilityValue;
        return {};
    }

    std::vector<AvailableAbilities> GetAbilitiesForUnits(const Units& UnitsToQueryValue,
                                                         bool IgnoreResourceRequirementsValue = false,
                                                         bool UseGeneralizedAbilityValue = true) override
    {
        (void)IgnoreResourceRequirementsValue;
        (void)UseGeneralizedAbilityValue;
        return std::vector<AvailableAbilities>(UnitsToQueryValue.size());
    }

    float PathingDistance(const Point2D& StartPointValue, const Point2D& EndPointValue) override
    {
        (void)StartPointValue;
        (void)EndPointValue;
        return 1.0f;
    }

    float PathingDistance(const Unit* StartUnitValue, const Point2D& EndPointValue) override
    {
        (void)StartUnitValue;
        (void)EndPointValue;
        return 1.0f;
    }

    std::vector<float> PathingDistance(const std::vector<PathingQuery>& QueriesValue) override
    {
        return std::vector<float>(QueriesValue.size(), 1.0f);
    }

    bool Placement(const AbilityID& AbilityIdValue, const Point2D& TargetPointValue,
                   const Unit* UnitPtrValue = nullptr) override
    {
        (void)AbilityIdValue;
        (void)UnitPtrValue;

        for (const Point2D& RejectedPlacementPointValue : RejectedPlacementPoints)
        {
            if (DistanceSquared2D(TargetPointValue, RejectedPlacementPointValue) <=
                (RejectedPlacementRadiusValue * RejectedPlacementRadiusValue))
            {
                return false;
            }
        }

        return true;
    }

    std::vector<bool> Placement(const std::vector<PlacementQuery>& QueriesValue) override
    {
        std::vector<bool> PlacementResultsValue;
        PlacementResultsValue.reserve(QueriesValue.size());

        for (const PlacementQuery& PlacementQueryValue : QueriesValue)
        {
            PlacementResultsValue.push_back(Placement(PlacementQueryValue.ability,
                                                      PlacementQueryValue.target_pos));
        }

        return PlacementResultsValue;
    }
};

bool Check(const bool ConditionValue, bool& SuccessValue, const std::string& MessageValue)
{
    if (!ConditionValue)
    {
        SuccessValue = false;
        std::cerr << "    " << MessageValue << std::endl;
    }

    return ConditionValue;
}

bool ArePointsEqual(const Point2D& LeftValue, const Point2D& RightValue)
{
    constexpr float CoordinateToleranceValue = 0.001f;
    return std::fabs(LeftValue.x - RightValue.x) <= CoordinateToleranceValue &&
           std::fabs(LeftValue.y - RightValue.y) <= CoordinateToleranceValue;
}

bool IsPointWithinDistance(const Point2D& LeftValue, const Point2D& RightValue, const float DistanceValue)
{
    return DistanceSquared2D(LeftValue, RightValue) <= (DistanceValue * DistanceValue);
}

Point2D GetNormalizedDirectionForTest(const Point2D& DirectionValue)
{
    const float LengthSquaredValue =
        (DirectionValue.x * DirectionValue.x) + (DirectionValue.y * DirectionValue.y);
    if (LengthSquaredValue <= 0.0001f)
    {
        return Point2D(1.0f, 0.0f);
    }

    const float InverseLengthValue = 1.0f / std::sqrt(LengthSquaredValue);
    return Point2D(DirectionValue.x * InverseLengthValue, DirectionValue.y * InverseLengthValue);
}

Point2D GetClockwiseLateralDirectionForTest(const Point2D& ForwardDirectionValue)
{
    return Point2D(ForwardDirectionValue.y, -ForwardDirectionValue.x);
}

Point2D ProjectPlacementOffsetForTest(const Point2D& AnchorPointValue, const Point2D& ForwardDirectionValue,
                                      const Point2D& LateralDirectionValue, const Point2D& OffsetValue)
{
    return Point2D(AnchorPointValue.x + (LateralDirectionValue.x * OffsetValue.x) +
                       (ForwardDirectionValue.x * OffsetValue.y),
                   AnchorPointValue.y + (LateralDirectionValue.y * OffsetValue.x) +
                       (ForwardDirectionValue.y * OffsetValue.y));
}

Point2D GetSpawnVerticalMainBaseStepForTest(const Point2D& BaseLocationValue, const Point2D& PlayableMinValue,
                                            const Point2D& PlayableMaxValue)
{
    const float CenterXValue = (PlayableMinValue.x + PlayableMaxValue.x) * 0.5f;
    const float CenterYValue = (PlayableMinValue.y + PlayableMaxValue.y) * 0.5f;
    const bool IsLeftSideValue = BaseLocationValue.x <= CenterXValue;
    const bool IsUpperSideValue = BaseLocationValue.y >= CenterYValue;

    if (IsLeftSideValue && IsUpperSideValue)
    {
        return Point2D(0.0f, 4.0f);
    }

    if (!IsLeftSideValue && !IsUpperSideValue)
    {
        return Point2D(0.0f, -4.0f);
    }

    if (!IsLeftSideValue && IsUpperSideValue)
    {
        return Point2D(0.0f, 4.0f);
    }

    return Point2D(0.0f, -4.0f);
}

Point2D GetSpawnHorizontalMainBaseStepForTest(const Point2D& BaseLocationValue, const Point2D& PlayableMinValue,
                                              const Point2D& PlayableMaxValue)
{
    const float CenterXValue = (PlayableMinValue.x + PlayableMaxValue.x) * 0.5f;
    const bool IsLeftSideValue = BaseLocationValue.x <= CenterXValue;

    return IsLeftSideValue ? Point2D(-6.0f, 0.0f) : Point2D(6.0f, 0.0f);
}

Point2D GetOneCellStepTowardPointForTest(const Point2D& FromPointValue, const Point2D& ToPointValue)
{
    const float DeltaXValue = ToPointValue.x - FromPointValue.x;
    const float DeltaYValue = ToPointValue.y - FromPointValue.y;
    const float StepXValue = std::abs(DeltaXValue) >= 0.5f ? (DeltaXValue > 0.0f ? 1.0f : -1.0f) : 0.0f;
    const float StepYValue = std::abs(DeltaYValue) >= 0.5f ? (DeltaYValue > 0.0f ? 1.0f : -1.0f) : 0.0f;
    return Point2D(StepXValue, StepYValue);
}

bool ContainsSlot(const std::vector<FBuildPlacementSlot>& BuildPlacementSlotsValue,
                  const EBuildPlacementSlotType BuildPlacementSlotTypeValue,
                  const EBuildPlacementFootprintPolicy BuildPlacementFootprintPolicyValue,
                  const Point2D& BuildPointValue)
{
    for (const FBuildPlacementSlot& BuildPlacementSlotValue : BuildPlacementSlotsValue)
    {
        if (BuildPlacementSlotValue.SlotId.SlotType == BuildPlacementSlotTypeValue &&
            BuildPlacementSlotValue.FootprintPolicy == BuildPlacementFootprintPolicyValue &&
            ArePointsEqual(BuildPlacementSlotValue.BuildPoint, BuildPointValue))
        {
            return true;
        }
    }

    return false;
}

bool TryFindFirstSlotTypeIndex(const std::vector<FBuildPlacementSlot>& BuildPlacementSlotsValue,
                               const EBuildPlacementSlotType BuildPlacementSlotTypeValue,
                               size_t& OutSlotIndexValue)
{
    for (size_t SlotIndexValue = 0U; SlotIndexValue < BuildPlacementSlotsValue.size(); ++SlotIndexValue)
    {
        if (BuildPlacementSlotsValue[SlotIndexValue].SlotId.SlotType != BuildPlacementSlotTypeValue)
        {
            continue;
        }

        OutSlotIndexValue = SlotIndexValue;
        return true;
    }

    return false;
}

size_t GetImageCellIndex(const int WidthValue, const int TileXValue, const int TileYValue)
{
    return static_cast<size_t>(TileXValue + (TileYValue * WidthValue));
}

unsigned char EncodeTerrainHeight(const float TerrainHeightValue)
{
    return static_cast<unsigned char>(std::round((TerrainHeightValue * 8.0f) + 127.0f));
}

void SetImageCellValue(ImageData& ImageDataValue, const int TileXValue, const int TileYValue,
                       const unsigned char EncodedValue)
{
    ImageDataValue.data[GetImageCellIndex(ImageDataValue.width, TileXValue, TileYValue)] =
        static_cast<char>(EncodedValue);
}

void SetPlacementBlockedRadius(GameInfo& GameInfoValue, const Point2D& CenterPointValue, const float RadiusValue)
{
    const int MinimumTileXValue =
        std::max(0, static_cast<int>(std::floor(CenterPointValue.x - RadiusValue - 1.0f)));
    const int MaximumTileXValue =
        std::min(GameInfoValue.width - 1, static_cast<int>(std::ceil(CenterPointValue.x + RadiusValue + 1.0f)));
    const int MinimumTileYValue =
        std::max(0, static_cast<int>(std::floor(CenterPointValue.y - RadiusValue - 1.0f)));
    const int MaximumTileYValue =
        std::min(GameInfoValue.height - 1, static_cast<int>(std::ceil(CenterPointValue.y + RadiusValue + 1.0f)));
    const float RadiusSquaredValue = RadiusValue * RadiusValue;

    for (int TileXValue = MinimumTileXValue; TileXValue <= MaximumTileXValue; ++TileXValue)
    {
        for (int TileYValue = MinimumTileYValue; TileYValue <= MaximumTileYValue; ++TileYValue)
        {
            const Point2D TileCenterPointValue(static_cast<float>(TileXValue) + 0.5f,
                                               static_cast<float>(TileYValue) + 0.5f);
            if (DistanceSquared2D(TileCenterPointValue, CenterPointValue) > RadiusSquaredValue)
            {
                continue;
            }

            SetImageCellValue(GameInfoValue.placement_grid, TileXValue, TileYValue, 0U);
        }
    }
}

GameInfo CreateSyntheticRampGameInfo()
{
    GameInfo GameInfoValue;
    GameInfoValue.width = 64;
    GameInfoValue.height = 64;
    GameInfoValue.playable_min = Point2D(0.0f, 0.0f);
    GameInfoValue.playable_max = Point2D(63.0f, 63.0f);

    GameInfoValue.pathing_grid.width = GameInfoValue.width;
    GameInfoValue.pathing_grid.height = GameInfoValue.height;
    GameInfoValue.pathing_grid.bits_per_pixel = 8;
    GameInfoValue.pathing_grid.data.assign(static_cast<size_t>(GameInfoValue.width * GameInfoValue.height),
                                           static_cast<char>(0));

    GameInfoValue.placement_grid.width = GameInfoValue.width;
    GameInfoValue.placement_grid.height = GameInfoValue.height;
    GameInfoValue.placement_grid.bits_per_pixel = 8;
    GameInfoValue.placement_grid.data.assign(static_cast<size_t>(GameInfoValue.width * GameInfoValue.height),
                                             static_cast<char>(255));

    GameInfoValue.terrain_height.width = GameInfoValue.width;
    GameInfoValue.terrain_height.height = GameInfoValue.height;
    GameInfoValue.terrain_height.bits_per_pixel = 8;
    GameInfoValue.terrain_height.data.assign(static_cast<size_t>(GameInfoValue.width * GameInfoValue.height),
                                             static_cast<char>(EncodeTerrainHeight(2.0f)));

    for (int TileXValue = 0; TileXValue <= 25; ++TileXValue)
    {
        for (int TileYValue = 0; TileYValue < GameInfoValue.height; ++TileYValue)
        {
            SetImageCellValue(GameInfoValue.terrain_height, TileXValue, TileYValue,
                              EncodeTerrainHeight(3.0f));
        }
    }

    const std::vector<Point2DI> UpperRampTilesValue =
    {
        Point2DI(26, 31),
        Point2DI(26, 33),
    };
    const std::vector<Point2DI> UpperMidRampTilesValue =
    {
        Point2DI(27, 31),
        Point2DI(27, 32),
        Point2DI(27, 33),
    };
    const std::vector<Point2DI> MidRampTilesValue =
    {
        Point2DI(28, 30),
        Point2DI(28, 31),
        Point2DI(28, 32),
        Point2DI(28, 33),
        Point2DI(28, 34),
    };
    const std::vector<Point2DI> LowerMidRampTilesValue =
    {
        Point2DI(29, 31),
        Point2DI(29, 32),
        Point2DI(29, 33),
    };
    const std::vector<Point2DI> LowerRampTilesValue =
    {
        Point2DI(30, 31),
        Point2DI(30, 33),
    };

    const std::vector<std::vector<Point2DI>> RampTileBandsValue =
    {
        UpperRampTilesValue,
        UpperMidRampTilesValue,
        MidRampTilesValue,
        LowerMidRampTilesValue,
        LowerRampTilesValue,
    };
    const std::vector<float> RampHeightsValue =
    {
        3.0f,
        2.75f,
        2.5f,
        2.25f,
        2.0f,
    };

    for (size_t BandIndexValue = 0U; BandIndexValue < RampTileBandsValue.size(); ++BandIndexValue)
    {
        for (const Point2DI& RampTilePointValue : RampTileBandsValue[BandIndexValue])
        {
            SetImageCellValue(GameInfoValue.placement_grid, RampTilePointValue.x, RampTilePointValue.y, 0U);
            SetImageCellValue(GameInfoValue.terrain_height, RampTilePointValue.x, RampTilePointValue.y,
                              EncodeTerrainHeight(RampHeightsValue[BandIndexValue]));
        }
    }

    return GameInfoValue;
}

GameInfo CreateAuthoredLayoutGameInfo()
{
    GameInfo GameInfoValue;
    GameInfoValue.map_name = "Ladder/(2)Bel'ShirVestigeLE (Void).SC2Map";
    GameInfoValue.width = 200;
    GameInfoValue.height = 200;
    GameInfoValue.playable_min = Point2D(0.0f, 0.0f);
    GameInfoValue.playable_max = Point2D(199.0f, 199.0f);

    GameInfoValue.pathing_grid.width = GameInfoValue.width;
    GameInfoValue.pathing_grid.height = GameInfoValue.height;
    GameInfoValue.pathing_grid.bits_per_pixel = 8;
    GameInfoValue.pathing_grid.data.assign(static_cast<size_t>(GameInfoValue.width * GameInfoValue.height),
                                           static_cast<char>(255));

    GameInfoValue.placement_grid.width = GameInfoValue.width;
    GameInfoValue.placement_grid.height = GameInfoValue.height;
    GameInfoValue.placement_grid.bits_per_pixel = 8;
    GameInfoValue.placement_grid.data.assign(static_cast<size_t>(GameInfoValue.width * GameInfoValue.height),
                                             static_cast<char>(255));

    GameInfoValue.terrain_height.width = GameInfoValue.width;
    GameInfoValue.terrain_height.height = GameInfoValue.height;
    GameInfoValue.terrain_height.bits_per_pixel = 8;
    GameInfoValue.terrain_height.data.assign(static_cast<size_t>(GameInfoValue.width * GameInfoValue.height),
                                             static_cast<char>(EncodeTerrainHeight(2.0f)));

    return GameInfoValue;
}

Point2D MirrorPointAcrossPlayableBoundsForTest(const Point2D& PointValue, const Point2D& PlayableMinValue,
                                               const Point2D& PlayableMaxValue)
{
    return Point2D(PlayableMinValue.x + PlayableMaxValue.x - PointValue.x,
                   PlayableMinValue.y + PlayableMaxValue.y - PointValue.y);
}

Point2D MirrorPointAcrossDepthAxisForTest(const Point2D& PointValue, const Point2D& AnchorPointValue,
                                          const Point2D& DepthDirectionValue,
                                          const Point2D& LateralDirectionValue)
{
    const Point2D AnchorToPointValue = PointValue - AnchorPointValue;
    const float DepthOffsetValue = Dot2D(AnchorToPointValue, DepthDirectionValue);
    const float LateralOffsetValue = Dot2D(AnchorToPointValue, LateralDirectionValue);
    return AnchorPointValue + (DepthDirectionValue * DepthOffsetValue) -
           (LateralDirectionValue * LateralOffsetValue);
}

}  // namespace

bool TestTerranBuildPlacementService(int ArgC, char** ArgV)
{
    (void)ArgC;
    (void)ArgV;

    bool SuccessValue = true;

    FGameStateDescriptor GameStateDescriptorValue;
    FBuildPlacementContext BuildPlacementContextValue;
    BuildPlacementContextValue.BaseLocation = Point2D(50.0f, 50.0f);
    BuildPlacementContextValue.NaturalLocation = Point2D(70.0f, 50.0f);
    FTerranBuildPlacementService BuildPlacementServiceValue;
    BuildPlacementContextValue.RampWallDescriptor =
        BuildPlacementServiceValue.GetRampWallDescriptor(FFrameContext(), BuildPlacementContextValue);
    BuildPlacementContextValue.MainBaseLayoutDescriptor =
        BuildPlacementServiceValue.GetMainBaseLayoutDescriptor(FFrameContext(), BuildPlacementContextValue);

    const Point2D PrimaryAnchorValue =
        BuildPlacementServiceValue.GetPrimaryStructureAnchor(GameStateDescriptorValue, BuildPlacementContextValue);
    Check(ArePointsEqual(PrimaryAnchorValue, Point2D(56.0f, 50.0f)), SuccessValue,
          "Primary structure anchor should bias forward toward the natural approach.");
    Check(BuildPlacementContextValue.RampWallDescriptor.bIsValid, SuccessValue,
          "Placement context should cache a valid ramp-wall descriptor when the natural direction is known.");
    Check(BuildPlacementContextValue.MainBaseLayoutDescriptor.bIsValid, SuccessValue,
          "Placement context should cache a valid main-base layout descriptor when the natural direction is known.");
    Check(ArePointsEqual(BuildPlacementContextValue.MainBaseLayoutDescriptor.LayoutAnchorPoint, Point2D(52.5f, 50.0f)),
          SuccessValue, "Main-base layout discovery should publish the deterministic layout anchor.");
    Check(ArePointsEqual(BuildPlacementContextValue.RampWallDescriptor.InsideStagingPoint, Point2D(54.5f, 50.0f)),
          SuccessValue, "Ramp-wall discovery should publish the inside staging point.");
    Check(ArePointsEqual(BuildPlacementContextValue.RampWallDescriptor.OutsideStagingPoint, Point2D(64.5f, 50.0f)),
          SuccessValue, "Ramp-wall discovery should publish the outside staging point.");
    const Point2D ArmyAssemblyPointValue =
        BuildPlacementServiceValue.GetArmyAssemblyPoint(GameStateDescriptorValue, BuildPlacementContextValue);
    Check(ArePointsEqual(ArmyAssemblyPointValue, BuildPlacementContextValue.RampWallDescriptor.OutsideStagingPoint),
          SuccessValue, "Army assembly should use the ramp-wall outside staging point when the wall is valid.");

    FBuildPlacementContext DiagonalBuildPlacementContextValue;
    DiagonalBuildPlacementContextValue.BaseLocation = Point2D(50.0f, 50.0f);
    DiagonalBuildPlacementContextValue.NaturalLocation = Point2D(64.0f, 64.0f);
    DiagonalBuildPlacementContextValue.RampWallDescriptor =
        BuildPlacementServiceValue.GetRampWallDescriptor(FFrameContext(), DiagonalBuildPlacementContextValue);
    DiagonalBuildPlacementContextValue.MainBaseLayoutDescriptor =
        BuildPlacementServiceValue.GetMainBaseLayoutDescriptor(FFrameContext(), DiagonalBuildPlacementContextValue);
    Check(DiagonalBuildPlacementContextValue.RampWallDescriptor.bIsValid, SuccessValue,
          "Ramp-wall descriptor should remain valid for diagonal natural directions.");
    Check(DiagonalBuildPlacementContextValue.MainBaseLayoutDescriptor.bIsValid, SuccessValue,
          "Main-base layout descriptor should remain valid for diagonal natural directions.");
    const Point2D DiagonalForwardDirectionValue =
        Point2D(std::sqrt(0.5f), std::sqrt(0.5f));
    const Point2D DiagonalWallToBarracksValue =
        DiagonalBuildPlacementContextValue.RampWallDescriptor.BarracksSlot.BuildPoint -
        DiagonalBuildPlacementContextValue.RampWallDescriptor.WallCenterPoint;
    Check((DiagonalWallToBarracksValue.x * DiagonalForwardDirectionValue.x) +
                  (DiagonalWallToBarracksValue.y * DiagonalForwardDirectionValue.y) >
              0.0f,
          SuccessValue, "Diagonal ramp barracks placement should stay forward of the wall center.");
    const Point2D DiagonalWallToLeftDepotValue =
        DiagonalBuildPlacementContextValue.RampWallDescriptor.LeftDepotSlot.BuildPoint -
        DiagonalBuildPlacementContextValue.RampWallDescriptor.WallCenterPoint;
    const Point2D DiagonalWallToRightDepotValue =
        DiagonalBuildPlacementContextValue.RampWallDescriptor.RightDepotSlot.BuildPoint -
        DiagonalBuildPlacementContextValue.RampWallDescriptor.WallCenterPoint;
    Check(((DiagonalWallToLeftDepotValue.x * DiagonalForwardDirectionValue.x) +
               (DiagonalWallToLeftDepotValue.y * DiagonalForwardDirectionValue.y)) <
                  0.0f &&
              ((DiagonalWallToRightDepotValue.x * DiagonalForwardDirectionValue.x) +
                   (DiagonalWallToRightDepotValue.y * DiagonalForwardDirectionValue.y)) <
                  0.0f,
          SuccessValue, "Diagonal ramp depots should stay on the inside side of the wall center.");

    GameInfo SyntheticRampGameInfoValue = CreateSyntheticRampGameInfo();
    FFakePlacementQuery FakePlacementQueryValue;
    FFrameContext SyntheticFrameContextValue;
    SyntheticFrameContextValue.GameInfo = &SyntheticRampGameInfoValue;
    SyntheticFrameContextValue.Query = &FakePlacementQueryValue;

    FBuildPlacementContext SyntheticBuildPlacementContextValue;
    SyntheticBuildPlacementContextValue.BaseLocation = Point2D(18.0f, 32.0f);
    SyntheticBuildPlacementContextValue.NaturalLocation = Point2D(44.0f, 32.0f);
    SyntheticBuildPlacementContextValue.RampWallDescriptor =
        BuildPlacementServiceValue.GetRampWallDescriptor(SyntheticFrameContextValue,
                                                         SyntheticBuildPlacementContextValue);
    SyntheticBuildPlacementContextValue.MainBaseLayoutDescriptor =
        BuildPlacementServiceValue.GetMainBaseLayoutDescriptor(SyntheticFrameContextValue,
                                                               SyntheticBuildPlacementContextValue);

    Check(SyntheticBuildPlacementContextValue.RampWallDescriptor.bIsValid, SuccessValue,
          "Ramp-wall discovery should produce a valid descriptor from a synthetic ramp grid.");
    Check(SyntheticBuildPlacementContextValue.RampWallDescriptor.WallCenterPoint.x >
                  SyntheticBuildPlacementContextValue.BaseLocation.x &&
              SyntheticBuildPlacementContextValue.RampWallDescriptor.WallCenterPoint.x <
                  SyntheticBuildPlacementContextValue.NaturalLocation.x,
          SuccessValue, "Synthetic ramp wall center should lie between the main and natural bases.");
    Check(SyntheticBuildPlacementContextValue.RampWallDescriptor.InsideStagingPoint.x <
                  SyntheticBuildPlacementContextValue.RampWallDescriptor.WallCenterPoint.x &&
              SyntheticBuildPlacementContextValue.RampWallDescriptor.OutsideStagingPoint.x >
                  SyntheticBuildPlacementContextValue.RampWallDescriptor.WallCenterPoint.x,
          SuccessValue, "Synthetic ramp staging points should lie on opposite sides of the wall center.");
    Check(SyntheticBuildPlacementContextValue.RampWallDescriptor.LeftDepotSlot.BuildPoint.y >
                  SyntheticBuildPlacementContextValue.RampWallDescriptor.WallCenterPoint.y &&
              SyntheticBuildPlacementContextValue.RampWallDescriptor.RightDepotSlot.BuildPoint.y <
                  SyntheticBuildPlacementContextValue.RampWallDescriptor.WallCenterPoint.y,
          SuccessValue, "Synthetic ramp depots should straddle the wall center across the ramp width.");
    Check(SyntheticBuildPlacementContextValue.RampWallDescriptor.BarracksSlot.BuildPoint.x <
              SyntheticBuildPlacementContextValue.RampWallDescriptor.LeftDepotSlot.BuildPoint.x,
          SuccessValue, "Synthetic ramp barracks should sit on the main-base side of the depot pair.");
    Check(!SyntheticBuildPlacementContextValue.MainBaseLayoutDescriptor.BarracksWithAddonSlots.empty() &&
              SyntheticBuildPlacementContextValue.MainBaseLayoutDescriptor.BarracksWithAddonSlots.front().BuildPoint.x <
                  SyntheticBuildPlacementContextValue.RampWallDescriptor.WallCenterPoint.x,
          SuccessValue, "Synthetic main-base barracks slots should stay on the main-base side of the wall.");

    GameInfo AuthoredLayoutGameInfoValue = CreateAuthoredLayoutGameInfo();
    FFrameContext AuthoredLayoutFrameContextValue;
    AuthoredLayoutFrameContextValue.GameInfo = &AuthoredLayoutGameInfoValue;
    AuthoredLayoutFrameContextValue.Query = &FakePlacementQueryValue;

    FBuildPlacementContext AuthoredUpperLeftBuildPlacementContextValue;
    AuthoredUpperLeftBuildPlacementContextValue.MapName = AuthoredLayoutGameInfoValue.map_name;
    AuthoredUpperLeftBuildPlacementContextValue.BaseLocation = Point2D(32.0f, 168.0f);
    AuthoredUpperLeftBuildPlacementContextValue.NaturalLocation = Point2D(56.0f, 144.0f);
    AuthoredUpperLeftBuildPlacementContextValue.PlayableMin = AuthoredLayoutGameInfoValue.playable_min;
    AuthoredUpperLeftBuildPlacementContextValue.PlayableMax = AuthoredLayoutGameInfoValue.playable_max;
    AuthoredUpperLeftBuildPlacementContextValue.RampWallDescriptor =
        BuildPlacementServiceValue.GetRampWallDescriptor(FFrameContext(), AuthoredUpperLeftBuildPlacementContextValue);
    AuthoredUpperLeftBuildPlacementContextValue.MainBaseLayoutDescriptor =
        BuildPlacementServiceValue.GetMainBaseLayoutDescriptor(AuthoredLayoutFrameContextValue,
                                                               AuthoredUpperLeftBuildPlacementContextValue);

    const Point2D UpperLeftDepthDirectionValue = GetNormalizedDirectionForTest(
        AuthoredUpperLeftBuildPlacementContextValue.RampWallDescriptor.InsideStagingPoint -
        AuthoredUpperLeftBuildPlacementContextValue.RampWallDescriptor.WallCenterPoint);
    const Point2D UpperLeftLateralDirectionValue =
        GetClockwiseLateralDirectionForTest(UpperLeftDepthDirectionValue);
    const Point2D ExpectedAuthoredLayoutAnchorPointValue =
        AuthoredUpperLeftBuildPlacementContextValue.RampWallDescriptor.WallCenterPoint +
        (UpperLeftDepthDirectionValue * 6.0f);
    const Point2D ExpectedVerticalMainBaseStepValue = GetSpawnVerticalMainBaseStepForTest(
        AuthoredUpperLeftBuildPlacementContextValue.BaseLocation,
        AuthoredLayoutGameInfoValue.playable_min,
        AuthoredLayoutGameInfoValue.playable_max);
    const Point2D ExpectedHorizontalMainBaseStepValue = GetSpawnHorizontalMainBaseStepForTest(
        AuthoredUpperLeftBuildPlacementContextValue.BaseLocation,
        AuthoredLayoutGameInfoValue.playable_min,
        AuthoredLayoutGameInfoValue.playable_max);
    const Point2D ExpectedCommandCenterPullStepValue = GetOneCellStepTowardPointForTest(
        AuthoredUpperLeftBuildPlacementContextValue.RampWallDescriptor.BarracksSlot.BuildPoint,
        AuthoredUpperLeftBuildPlacementContextValue.BaseLocation);
    const Point2D ExpectedRailBarracksPointValue =
        AuthoredUpperLeftBuildPlacementContextValue.RampWallDescriptor.BarracksSlot.BuildPoint +
        ExpectedHorizontalMainBaseStepValue;
    const Point2D ExpectedRailFactoryPointValue =
        ExpectedRailBarracksPointValue + ExpectedHorizontalMainBaseStepValue;
    const Point2D ExpectedRailStarportPointValue =
        ExpectedRailFactoryPointValue + ExpectedHorizontalMainBaseStepValue;
    const Point2D ExpectedAuthoredFactoryPointValue =
        AuthoredUpperLeftBuildPlacementContextValue.RampWallDescriptor.BarracksSlot.BuildPoint +
        ExpectedVerticalMainBaseStepValue +
        ExpectedCommandCenterPullStepValue;
    const Point2D ExpectedAuthoredStarportPointValue =
        ExpectedAuthoredFactoryPointValue + ExpectedVerticalMainBaseStepValue;
    Check(ArePointsEqual(AuthoredUpperLeftBuildPlacementContextValue.MainBaseLayoutDescriptor.LayoutAnchorPoint,
                         ExpectedAuthoredLayoutAnchorPointValue),
          SuccessValue, "Authored Bel'Shir upper-left layout anchor should stay tied to the ramp-back production frame.");
    Check(AuthoredUpperLeftBuildPlacementContextValue.MainBaseLayoutDescriptor.ProductionRailWithAddonSlots.size() == 3U,
          SuccessValue, "Authored Bel'Shir upper-left layout should expose three shared production rail pads.");
    if (AuthoredUpperLeftBuildPlacementContextValue.MainBaseLayoutDescriptor.ProductionRailWithAddonSlots.size() == 3U)
    {
        Check(AuthoredUpperLeftBuildPlacementContextValue.MainBaseLayoutDescriptor.ProductionRailWithAddonSlots[0]
                          .SlotId.SlotType == EBuildPlacementSlotType::MainProductionWithAddon &&
                  AuthoredUpperLeftBuildPlacementContextValue.MainBaseLayoutDescriptor.ProductionRailWithAddonSlots[0]
                          .SlotId.Ordinal == 0U &&
                  ArePointsEqual(AuthoredUpperLeftBuildPlacementContextValue.MainBaseLayoutDescriptor
                                     .ProductionRailWithAddonSlots[0]
                                     .BuildPoint,
                                 ExpectedRailBarracksPointValue),
              SuccessValue, "Authored Bel'Shir upper-left rail ordinal zero should stay on the curated barracks pad.");
        Check(AuthoredUpperLeftBuildPlacementContextValue.MainBaseLayoutDescriptor.ProductionRailWithAddonSlots[1]
                          .SlotId.SlotType == EBuildPlacementSlotType::MainProductionWithAddon &&
                  AuthoredUpperLeftBuildPlacementContextValue.MainBaseLayoutDescriptor.ProductionRailWithAddonSlots[1]
                          .SlotId.Ordinal == 1U &&
                  ArePointsEqual(AuthoredUpperLeftBuildPlacementContextValue.MainBaseLayoutDescriptor
                                     .ProductionRailWithAddonSlots[1]
                                     .BuildPoint,
                                 ExpectedRailFactoryPointValue),
              SuccessValue, "Authored Bel'Shir upper-left rail ordinal one should stay on the curated factory pad.");
        Check(AuthoredUpperLeftBuildPlacementContextValue.MainBaseLayoutDescriptor.ProductionRailWithAddonSlots[2]
                          .SlotId.SlotType == EBuildPlacementSlotType::MainProductionWithAddon &&
                  AuthoredUpperLeftBuildPlacementContextValue.MainBaseLayoutDescriptor.ProductionRailWithAddonSlots[2]
                          .SlotId.Ordinal == 2U &&
                  ArePointsEqual(AuthoredUpperLeftBuildPlacementContextValue.MainBaseLayoutDescriptor
                                     .ProductionRailWithAddonSlots[2]
                                     .BuildPoint,
                                 ExpectedRailStarportPointValue),
              SuccessValue, "Authored Bel'Shir upper-left rail ordinal two should stay on the curated starport pad.");
    }
    Check(AuthoredUpperLeftBuildPlacementContextValue.MainBaseLayoutDescriptor.BarracksWithAddonSlots.size() >= 1U,
          SuccessValue, "Authored Bel'Shir upper-left layout should expose the curated first main barracks slot.");
    if (AuthoredUpperLeftBuildPlacementContextValue.MainBaseLayoutDescriptor.BarracksWithAddonSlots.size() >= 1U)
    {
        Check(ArePointsEqual(AuthoredUpperLeftBuildPlacementContextValue.MainBaseLayoutDescriptor
                                 .BarracksWithAddonSlots[0]
                                 .BuildPoint,
                             ExpectedRailBarracksPointValue),
              SuccessValue, "The authored first main barracks slot should alias the curated barracks opening pad.");
    }
    Check(AuthoredUpperLeftBuildPlacementContextValue.MainBaseLayoutDescriptor.FactoryWithAddonSlots.size() >= 1U,
          SuccessValue, "Authored Bel'Shir upper-left layout should expose the curated first main factory slot.");
    if (AuthoredUpperLeftBuildPlacementContextValue.MainBaseLayoutDescriptor.FactoryWithAddonSlots.size() >= 1U)
    {
        Check(ArePointsEqual(AuthoredUpperLeftBuildPlacementContextValue.MainBaseLayoutDescriptor
                                 .FactoryWithAddonSlots[0]
                                 .BuildPoint,
                             ExpectedAuthoredFactoryPointValue),
              SuccessValue, "The authored first main factory slot should alias the curated factory opening pad.");
    }
    Check(AuthoredUpperLeftBuildPlacementContextValue.MainBaseLayoutDescriptor.StarportWithAddonSlots.size() >= 1U,
          SuccessValue, "Authored Bel'Shir upper-left layout should expose the curated first main starport slot.");
    if (AuthoredUpperLeftBuildPlacementContextValue.MainBaseLayoutDescriptor.StarportWithAddonSlots.size() >= 1U)
    {
        Check(ArePointsEqual(AuthoredUpperLeftBuildPlacementContextValue.MainBaseLayoutDescriptor
                                 .StarportWithAddonSlots[0]
                                 .BuildPoint,
                             ExpectedAuthoredStarportPointValue),
              SuccessValue, "The authored first main starport slot should alias the curated starport opening pad.");
    }

    FSelectivePlacementQuery SelectivePlacementQueryValue;
    SelectivePlacementQueryValue.RejectedPlacementPoints.push_back(ExpectedRailBarracksPointValue);
    SelectivePlacementQueryValue.RejectedPlacementPoints.push_back(ExpectedRailFactoryPointValue);
    SelectivePlacementQueryValue.RejectedPlacementPoints.push_back(ExpectedRailStarportPointValue);

    FFrameContext SnappedAuthoredLayoutFrameContextValue;
    SnappedAuthoredLayoutFrameContextValue.GameInfo = &AuthoredLayoutGameInfoValue;
    SnappedAuthoredLayoutFrameContextValue.Query = &SelectivePlacementQueryValue;

    FBuildPlacementContext SnappedAuthoredBuildPlacementContextValue = AuthoredUpperLeftBuildPlacementContextValue;
    SnappedAuthoredBuildPlacementContextValue.MainBaseLayoutDescriptor.Reset();
    SnappedAuthoredBuildPlacementContextValue.MainBaseLayoutDescriptor =
        BuildPlacementServiceValue.GetMainBaseLayoutDescriptor(SnappedAuthoredLayoutFrameContextValue,
                                                               SnappedAuthoredBuildPlacementContextValue);
    Check(SnappedAuthoredBuildPlacementContextValue.MainBaseLayoutDescriptor.ProductionRailWithAddonSlots.size() == 3U,
          SuccessValue,
          "Authored production rail should ignore transient query rejection and keep publishing the deterministic opening pads.");
    if (SnappedAuthoredBuildPlacementContextValue.MainBaseLayoutDescriptor.ProductionRailWithAddonSlots.size() == 3U)
    {
        Check(SnappedAuthoredBuildPlacementContextValue.MainBaseLayoutDescriptor.ProductionRailWithAddonSlots[0]
                          .SlotId.Ordinal == 0U &&
                  ArePointsEqual(SnappedAuthoredBuildPlacementContextValue.MainBaseLayoutDescriptor
                                     .ProductionRailWithAddonSlots[0]
                                     .BuildPoint,
                                 ExpectedRailBarracksPointValue),
              SuccessValue,
              "Authored barracks rail ordinal zero should stay on the deterministic pad even when query placement is temporarily rejected.");
        Check(SnappedAuthoredBuildPlacementContextValue.MainBaseLayoutDescriptor.ProductionRailWithAddonSlots[1]
                          .SlotId.Ordinal == 1U &&
                  ArePointsEqual(SnappedAuthoredBuildPlacementContextValue.MainBaseLayoutDescriptor
                                     .ProductionRailWithAddonSlots[1]
                                     .BuildPoint,
                                 ExpectedRailFactoryPointValue),
              SuccessValue,
              "Authored factory rail ordinal one should stay on the deterministic pad even when query placement is temporarily rejected.");
        Check(SnappedAuthoredBuildPlacementContextValue.MainBaseLayoutDescriptor.ProductionRailWithAddonSlots[2]
                          .SlotId.Ordinal == 2U &&
                  ArePointsEqual(SnappedAuthoredBuildPlacementContextValue.MainBaseLayoutDescriptor
                                     .ProductionRailWithAddonSlots[2]
                                     .BuildPoint,
                                 ExpectedRailStarportPointValue),
              SuccessValue,
              "Authored starport rail ordinal two should stay on the deterministic pad even when query placement is temporarily rejected.");
    }

    const Point2D MirroredAuthoredBarracksPointValue = MirrorPointAcrossDepthAxisForTest(
        ExpectedRailBarracksPointValue, ExpectedAuthoredLayoutAnchorPointValue,
        UpperLeftDepthDirectionValue, UpperLeftLateralDirectionValue);
    const Point2D MirroredAuthoredFactoryPointValue = MirrorPointAcrossDepthAxisForTest(
        ExpectedRailFactoryPointValue, ExpectedAuthoredLayoutAnchorPointValue,
        UpperLeftDepthDirectionValue, UpperLeftLateralDirectionValue);
    const Point2D MirroredAuthoredStarportPointValue = MirrorPointAcrossDepthAxisForTest(
        ExpectedRailStarportPointValue, ExpectedAuthoredLayoutAnchorPointValue,
        UpperLeftDepthDirectionValue, UpperLeftLateralDirectionValue);

    GameInfo MirroredRailGameInfoValue = CreateAuthoredLayoutGameInfo();
    SetPlacementBlockedRadius(MirroredRailGameInfoValue, ExpectedRailBarracksPointValue, 4.0f);
    SetPlacementBlockedRadius(MirroredRailGameInfoValue, ExpectedRailFactoryPointValue, 4.0f);
    SetPlacementBlockedRadius(MirroredRailGameInfoValue, ExpectedRailStarportPointValue, 4.0f);

    FFrameContext MirroredRailFrameContextValue;
    MirroredRailFrameContextValue.GameInfo = &MirroredRailGameInfoValue;
    MirroredRailFrameContextValue.Query = &FakePlacementQueryValue;

    FBuildPlacementContext MirroredRailBuildPlacementContextValue = AuthoredUpperLeftBuildPlacementContextValue;
    MirroredRailBuildPlacementContextValue.MainBaseLayoutDescriptor.Reset();
    MirroredRailBuildPlacementContextValue.MainBaseLayoutDescriptor =
        BuildPlacementServiceValue.GetMainBaseLayoutDescriptor(MirroredRailFrameContextValue,
                                                               MirroredRailBuildPlacementContextValue);
    Check(MirroredRailBuildPlacementContextValue.MainBaseLayoutDescriptor.ProductionRailWithAddonSlots.size() == 3U,
          SuccessValue, "Authored main-base layout should switch to the mirrored production rail when the primary side is blocked.");
    if (MirroredRailBuildPlacementContextValue.MainBaseLayoutDescriptor.ProductionRailWithAddonSlots.size() == 3U)
    {
        const Point2D ExpectedFactoryStepOffsetValue =
            ExpectedRailFactoryPointValue - ExpectedRailBarracksPointValue;
        const Point2D ExpectedStarportStepOffsetValue =
            ExpectedRailStarportPointValue - ExpectedRailFactoryPointValue;

        Check(!IsPointWithinDistance(MirroredRailBuildPlacementContextValue.MainBaseLayoutDescriptor
                                         .ProductionRailWithAddonSlots[0]
                                         .BuildPoint,
                                     ExpectedRailBarracksPointValue, 4.0f),
              SuccessValue, "Fallback rail ordinal zero should move away from the blocked primary barracks pad.");
        Check(!IsPointWithinDistance(MirroredRailBuildPlacementContextValue.MainBaseLayoutDescriptor
                                         .ProductionRailWithAddonSlots[1]
                                         .BuildPoint,
                                     ExpectedRailFactoryPointValue, 4.0f),
              SuccessValue, "Fallback rail ordinal one should move away from the blocked primary factory pad.");
        Check(!IsPointWithinDistance(MirroredRailBuildPlacementContextValue.MainBaseLayoutDescriptor
                                         .ProductionRailWithAddonSlots[2]
                                         .BuildPoint,
                                     ExpectedRailStarportPointValue, 4.0f),
              SuccessValue, "Fallback rail ordinal two should move away from the blocked primary starport pad.");
        Check(IsPointWithinDistance(
                  MirroredRailBuildPlacementContextValue.MainBaseLayoutDescriptor
                          .ProductionRailWithAddonSlots[1]
                          .BuildPoint,
                  MirroredRailBuildPlacementContextValue.MainBaseLayoutDescriptor
                          .ProductionRailWithAddonSlots[0]
                          .BuildPoint +
                      ExpectedFactoryStepOffsetValue,
                  2.0f),
              SuccessValue, "Mirrored rail ordinal one should stay chained from the mirrored barracks rail pad.");
        Check(IsPointWithinDistance(
                  MirroredRailBuildPlacementContextValue.MainBaseLayoutDescriptor
                          .ProductionRailWithAddonSlots[2]
                          .BuildPoint,
                  MirroredRailBuildPlacementContextValue.MainBaseLayoutDescriptor
                          .ProductionRailWithAddonSlots[1]
                          .BuildPoint +
                      ExpectedStarportStepOffsetValue,
                  2.0f),
              SuccessValue, "Mirrored rail ordinal two should stay chained from the mirrored factory rail pad.");
    }

    GameInfo InvalidSharedRailGameInfoValue = CreateAuthoredLayoutGameInfo();
    SetPlacementBlockedRadius(InvalidSharedRailGameInfoValue, ExpectedRailFactoryPointValue, 20.0f);
    SetPlacementBlockedRadius(InvalidSharedRailGameInfoValue, MirroredAuthoredFactoryPointValue, 20.0f);

    FFrameContext InvalidSharedRailFrameContextValue;
    InvalidSharedRailFrameContextValue.GameInfo = &InvalidSharedRailGameInfoValue;
    InvalidSharedRailFrameContextValue.Query = &FakePlacementQueryValue;

    FBuildPlacementContext InvalidSharedRailBuildPlacementContextValue =
        AuthoredUpperLeftBuildPlacementContextValue;
    InvalidSharedRailBuildPlacementContextValue.MainBaseLayoutDescriptor.Reset();
    InvalidSharedRailBuildPlacementContextValue.MainBaseLayoutDescriptor =
        BuildPlacementServiceValue.GetMainBaseLayoutDescriptor(InvalidSharedRailFrameContextValue,
                                                               InvalidSharedRailBuildPlacementContextValue);
    Check(InvalidSharedRailBuildPlacementContextValue.MainBaseLayoutDescriptor.ProductionRailWithAddonSlots.empty(),
          SuccessValue,
          "Authored main-base layout should suppress the shared rail entirely when no full three-pad rail candidate resolves.");

    FBuildPlacementContext AuthoredLowerRightBuildPlacementContextValue;
    AuthoredLowerRightBuildPlacementContextValue.MapName = AuthoredLayoutGameInfoValue.map_name;
    AuthoredLowerRightBuildPlacementContextValue.BaseLocation = Point2D(167.0f, 31.0f);
    AuthoredLowerRightBuildPlacementContextValue.NaturalLocation = Point2D(143.0f, 55.0f);
    AuthoredLowerRightBuildPlacementContextValue.PlayableMin = AuthoredLayoutGameInfoValue.playable_min;
    AuthoredLowerRightBuildPlacementContextValue.PlayableMax = AuthoredLayoutGameInfoValue.playable_max;
    AuthoredLowerRightBuildPlacementContextValue.RampWallDescriptor =
        BuildPlacementServiceValue.GetRampWallDescriptor(FFrameContext(), AuthoredLowerRightBuildPlacementContextValue);
    AuthoredLowerRightBuildPlacementContextValue.MainBaseLayoutDescriptor =
        BuildPlacementServiceValue.GetMainBaseLayoutDescriptor(AuthoredLayoutFrameContextValue,
                                                               AuthoredLowerRightBuildPlacementContextValue);

    const Point2D ExpectedMirroredBarracksPointValue = MirrorPointAcrossPlayableBoundsForTest(
        ExpectedRailBarracksPointValue, AuthoredLayoutGameInfoValue.playable_min,
        AuthoredLayoutGameInfoValue.playable_max);
    const Point2D ExpectedMirroredFactoryPointValue = MirrorPointAcrossPlayableBoundsForTest(
        ExpectedRailFactoryPointValue, AuthoredLayoutGameInfoValue.playable_min,
        AuthoredLayoutGameInfoValue.playable_max);
    const Point2D ExpectedMirroredStarportPointValue = MirrorPointAcrossPlayableBoundsForTest(
        ExpectedRailStarportPointValue, AuthoredLayoutGameInfoValue.playable_min,
        AuthoredLayoutGameInfoValue.playable_max);
    Check(AuthoredLowerRightBuildPlacementContextValue.MainBaseLayoutDescriptor.ProductionRailWithAddonSlots.size() ==
              3U,
          SuccessValue, "Authored Bel'Shir lower-right layout should expose three shared production rail pads.");
    if (AuthoredLowerRightBuildPlacementContextValue.MainBaseLayoutDescriptor.ProductionRailWithAddonSlots.size() == 3U)
    {
        Check(ArePointsEqual(AuthoredLowerRightBuildPlacementContextValue.MainBaseLayoutDescriptor
                                 .ProductionRailWithAddonSlots[0]
                                 .BuildPoint,
                             ExpectedMirroredBarracksPointValue),
              SuccessValue, "Authored Bel'Shir lower-right rail ordinal zero should mirror the curated barracks pad.");
        Check(ArePointsEqual(AuthoredLowerRightBuildPlacementContextValue.MainBaseLayoutDescriptor
                                 .ProductionRailWithAddonSlots[1]
                                 .BuildPoint,
                             ExpectedMirroredFactoryPointValue),
              SuccessValue, "Authored Bel'Shir lower-right rail ordinal one should mirror the curated factory pad.");
        Check(ArePointsEqual(AuthoredLowerRightBuildPlacementContextValue.MainBaseLayoutDescriptor
                                 .ProductionRailWithAddonSlots[2]
                                 .BuildPoint,
                             ExpectedMirroredStarportPointValue),
              SuccessValue, "Authored Bel'Shir lower-right rail ordinal two should mirror the curated starport pad.");
    }

    const std::vector<FBuildPlacementSlot> AuthoredBarracksSlotsValue =
        BuildPlacementServiceValue.GetStructurePlacementSlots(GameStateDescriptorValue,
                                                              ABILITY_ID::BUILD_BARRACKS,
                                                              AuthoredUpperLeftBuildPlacementContextValue);
    Check(AuthoredBarracksSlotsValue.size() >= 4U, SuccessValue,
          "Authored barracks placement should expose the wall slot plus the curated first main barracks slot.");
    if (AuthoredBarracksSlotsValue.size() >= 4U)
    {
        Check(AuthoredBarracksSlotsValue[0].SlotId.SlotType == EBuildPlacementSlotType::MainRampBarracksWithAddon,
              SuccessValue, "Authored barracks placement should still keep the wall barracks ahead of main production.");
        Check(AuthoredBarracksSlotsValue[1].SlotId.SlotType == EBuildPlacementSlotType::MainBarracksWithAddon &&
                  AuthoredBarracksSlotsValue[1].SlotId.Ordinal == 0U &&
                  ArePointsEqual(AuthoredBarracksSlotsValue[1].BuildPoint, ExpectedRailBarracksPointValue),
              SuccessValue, "Authored barracks placement should expose the curated first main barracks slot immediately after the wall.");
    }

    size_t FirstBarracksRailIndexValue = 0U;
    Check(TryFindFirstSlotTypeIndex(AuthoredBarracksSlotsValue, EBuildPlacementSlotType::MainProductionWithAddon,
                                    FirstBarracksRailIndexValue) &&
              FirstBarracksRailIndexValue > 1U,
          SuccessValue, "Authored barracks placement should keep the shared rail behind the curated first barracks slot.");

    const std::vector<FBuildPlacementSlot> AuthoredFactorySlotsValue =
        BuildPlacementServiceValue.GetStructurePlacementSlots(GameStateDescriptorValue,
                                                              ABILITY_ID::BUILD_FACTORY,
                                                              AuthoredUpperLeftBuildPlacementContextValue);
    Check(AuthoredFactorySlotsValue.size() >= 3U, SuccessValue,
          "Authored factory placement should expose the curated first main factory slot and the shared rail.");
    if (AuthoredFactorySlotsValue.size() >= 3U)
    {
        Check(AuthoredFactorySlotsValue[0].SlotId.SlotType == EBuildPlacementSlotType::MainFactoryWithAddon &&
                  AuthoredFactorySlotsValue[0].SlotId.Ordinal == 0U,
              SuccessValue, "Authored factory placement should expose the curated first main factory slot first.");
        Check(ArePointsEqual(AuthoredFactorySlotsValue[0].BuildPoint, ExpectedAuthoredFactoryPointValue),
              SuccessValue, "Authored factory placement should target the curated factory opening pad first.");
    }

    size_t FirstFactoryRailIndexValue = 0U;
    Check(TryFindFirstSlotTypeIndex(AuthoredFactorySlotsValue, EBuildPlacementSlotType::MainProductionWithAddon,
                                    FirstFactoryRailIndexValue) &&
              FirstFactoryRailIndexValue > 0U,
          SuccessValue, "Authored factory placement should keep the shared rail behind the curated first factory slot.");

    const std::vector<FBuildPlacementSlot> AuthoredStarportSlotsValue =
        BuildPlacementServiceValue.GetStructurePlacementSlots(GameStateDescriptorValue,
                                                              ABILITY_ID::BUILD_STARPORT,
                                                              AuthoredUpperLeftBuildPlacementContextValue);
    Check(AuthoredStarportSlotsValue.size() >= 3U, SuccessValue,
          "Authored starport placement should expose the curated first main starport slot and the shared rail.");
    if (AuthoredStarportSlotsValue.size() >= 3U)
    {
        Check(AuthoredStarportSlotsValue[0].SlotId.SlotType == EBuildPlacementSlotType::MainStarportWithAddon &&
                  AuthoredStarportSlotsValue[0].SlotId.Ordinal == 0U,
              SuccessValue, "Authored starport placement should expose the curated first main starport slot first.");
        Check(ArePointsEqual(AuthoredStarportSlotsValue[0].BuildPoint, ExpectedAuthoredStarportPointValue),
              SuccessValue, "Authored starport placement should target the curated starport opening pad first.");
    }

    size_t FirstStarportRailIndexValue = 0U;
    Check(TryFindFirstSlotTypeIndex(AuthoredStarportSlotsValue, EBuildPlacementSlotType::MainProductionWithAddon,
                                    FirstStarportRailIndexValue) &&
              FirstStarportRailIndexValue > 0U,
          SuccessValue, "Authored starport placement should keep the shared rail behind the curated first starport slot.");

    const std::vector<FBuildPlacementSlot> SupplyDepotSlotsValue =
        BuildPlacementServiceValue.GetStructurePlacementSlots(GameStateDescriptorValue,
                                                              ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                              BuildPlacementContextValue);
    Check(SupplyDepotSlotsValue.size() == 10U, SuccessValue,
          "Supply depot placement should expose ramp, natural-approach, and support slots.");
    Check(!SupplyDepotSlotsValue.empty() &&
              SupplyDepotSlotsValue.front().SlotId.SlotType == EBuildPlacementSlotType::MainRampDepotLeft,
          SuccessValue, "The first supply depot should prefer the main-ramp depot slot.");
    Check(!SupplyDepotSlotsValue.empty() &&
              SupplyDepotSlotsValue.front().FootprintPolicy == EBuildPlacementFootprintPolicy::StructureOnly,
          SuccessValue, "Supply depot slots should not reserve addon clearance.");
    Check(ContainsSlot(SupplyDepotSlotsValue, EBuildPlacementSlotType::MainRampDepotLeft,
                       EBuildPlacementFootprintPolicy::StructureOnly, Point2D(57.0f, 53.0f)),
          SuccessValue, "Supply depot placement should expose the left ramp wall slot from the descriptor.");
    Check(ContainsSlot(SupplyDepotSlotsValue, EBuildPlacementSlotType::MainRampDepotRight,
                       EBuildPlacementFootprintPolicy::StructureOnly, Point2D(57.0f, 47.0f)),
          SuccessValue, "Supply depot placement should expose the opposite ramp depot slot for the wall.");
    Check(ContainsSlot(SupplyDepotSlotsValue, EBuildPlacementSlotType::NaturalApproachDepot,
                       EBuildPlacementFootprintPolicy::StructureOnly, Point2D(56.5f, 62.0f)),
          SuccessValue, "Supply depot placement should include a natural-approach depot slot.");

    GameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
        UNIT_TYPEID::TERRAN_SUPPLYDEPOT)] = 1U;
    const std::vector<FBuildPlacementSlot> SecondSupplyDepotSlotsValue =
        BuildPlacementServiceValue.GetStructurePlacementSlots(GameStateDescriptorValue,
                                                              ABILITY_ID::BUILD_SUPPLYDEPOT,
                                                              BuildPlacementContextValue);
    Check(!SecondSupplyDepotSlotsValue.empty() &&
              SecondSupplyDepotSlotsValue.front().SlotId.SlotType == EBuildPlacementSlotType::MainRampDepotLeft,
          SuccessValue, "Observed depot counts should not reorder descriptor-backed wall slots.");
    Check(SecondSupplyDepotSlotsValue.size() >= 2U &&
              SecondSupplyDepotSlotsValue[1].SlotId.SlotType == EBuildPlacementSlotType::MainRampDepotRight,
          SuccessValue, "Supply depot placement should keep both ramp-wall slots ordered ahead of support slots.");
    GameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
        UNIT_TYPEID::TERRAN_SUPPLYDEPOT)] = 0U;

    const std::vector<FBuildPlacementSlot> BarracksSlotsValue =
        BuildPlacementServiceValue.GetStructurePlacementSlots(GameStateDescriptorValue,
                                                              ABILITY_ID::BUILD_BARRACKS,
                                                              BuildPlacementContextValue);
    Check(BarracksSlotsValue.size() >= 4U, SuccessValue,
          "The first barracks should expose the wall barracks plus the shared production rail.");
    Check(!BarracksSlotsValue.empty() &&
              BarracksSlotsValue.front().SlotId.SlotType == EBuildPlacementSlotType::MainRampBarracksWithAddon,
          SuccessValue, "The first barracks should prefer the ramp-facing barracks slot.");
    Check(!BarracksSlotsValue.empty() &&
              BarracksSlotsValue.front().FootprintPolicy ==
                  EBuildPlacementFootprintPolicy::RequiresAddonClearance,
          SuccessValue, "Barracks slots should require addon clearance.");
    Check(ContainsSlot(BarracksSlotsValue, EBuildPlacementSlotType::MainRampBarracksWithAddon,
                       EBuildPlacementFootprintPolicy::RequiresAddonClearance, Point2D(61.0f, 50.0f)),
          SuccessValue, "Barracks placement should expose the center wall slot.");
    Check(ContainsSlot(BarracksSlotsValue, EBuildPlacementSlotType::MainProductionWithAddon,
                       EBuildPlacementFootprintPolicy::RequiresAddonClearance, Point2D(52.5f, 58.0f)),
          SuccessValue, "Barracks placement should include the shared main-production rail barracks pad.");
    Check(ContainsSlot(BarracksSlotsValue, EBuildPlacementSlotType::MainProductionWithAddon,
                       EBuildPlacementFootprintPolicy::RequiresAddonClearance, Point2D(52.5f, 58.0f)) &&
              BarracksSlotsValue[1].BuildPoint.x <
                  BuildPlacementContextValue.RampWallDescriptor.WallCenterPoint.x,
          SuccessValue, "Barracks production-rail slots should stay inside the wall instead of drifting forward.");

    GameStateDescriptorValue.BuildPlanning.ObservedBuildingCounts[GetTerranBuildingTypeIndex(
        UNIT_TYPEID::TERRAN_BARRACKS)] = 1U;
    const std::vector<FBuildPlacementSlot> FollowUpBarracksSlotsValue =
        BuildPlacementServiceValue.GetStructurePlacementSlots(GameStateDescriptorValue,
                                                              ABILITY_ID::BUILD_BARRACKS,
                                                              BuildPlacementContextValue);
    Check(FollowUpBarracksSlotsValue.size() >= 4U, SuccessValue,
          "Barracks placement should keep the descriptor-backed wall slot in the ordered placement list.");
    Check(!FollowUpBarracksSlotsValue.empty() &&
              FollowUpBarracksSlotsValue.front().SlotId.SlotType ==
                  EBuildPlacementSlotType::MainRampBarracksWithAddon,
          SuccessValue, "Observed barracks counts should not hide the descriptor-backed ramp slot.");

    const std::vector<FBuildPlacementSlot> FactorySlotsValue =
        BuildPlacementServiceValue.GetStructurePlacementSlots(GameStateDescriptorValue,
                                                              ABILITY_ID::BUILD_FACTORY,
                                                              BuildPlacementContextValue);
    Check(FactorySlotsValue.size() >= 4U, SuccessValue,
          "Factory placement should expose the shared production rail before dedicated factory fallback slots.");
    Check(!FactorySlotsValue.empty() &&
              FactorySlotsValue.front().FootprintPolicy ==
                  EBuildPlacementFootprintPolicy::RequiresAddonClearance,
          SuccessValue, "Factory slots should require addon clearance.");
    Check(!FactorySlotsValue.empty() &&
              FactorySlotsValue.front().SlotId.SlotType == EBuildPlacementSlotType::MainProductionWithAddon,
          SuccessValue, "Factory placement should expose the shared main-production rail first.");
    Check(!ContainsSlot(FactorySlotsValue, EBuildPlacementSlotType::MainRampBarracksWithAddon,
                        EBuildPlacementFootprintPolicy::RequiresAddonClearance, Point2D(61.0f, 50.0f)),
          SuccessValue, "Factory placement should exclude the dedicated wall barracks slot.");
    Check(!FactorySlotsValue.empty() &&
              FactorySlotsValue.front().BuildPoint.x <
                  BuildPlacementContextValue.RampWallDescriptor.WallCenterPoint.x,
          SuccessValue, "Factory placement should prefer main-base addon slots behind the wall.");
    Check(ContainsSlot(FactorySlotsValue, EBuildPlacementSlotType::MainProductionWithAddon,
                       EBuildPlacementFootprintPolicy::RequiresAddonClearance, Point2D(44.0f, 66.0f)),
          SuccessValue, "Factory placement should include the shared production-rail factory slot.");
    Check(ContainsSlot(FactorySlotsValue, EBuildPlacementSlotType::MainFactoryWithAddon,
                       EBuildPlacementFootprintPolicy::RequiresAddonClearance, Point2D(44.5f, 54.0f)),
          SuccessValue, "Factory placement should include the deterministic forward factory slot.");

    const std::vector<FBuildPlacementSlot> StarportSlotsValue =
        BuildPlacementServiceValue.GetStructurePlacementSlots(GameStateDescriptorValue,
                                                              ABILITY_ID::BUILD_STARPORT,
                                                              BuildPlacementContextValue);
    Check(StarportSlotsValue.size() >= 4U, SuccessValue,
          "Starport placement should expose the shared production rail before dedicated starport fallback slots.");
    Check(!StarportSlotsValue.empty() &&
              StarportSlotsValue.front().SlotId.SlotType == EBuildPlacementSlotType::MainProductionWithAddon,
          SuccessValue, "Starport placement should expose the shared main-production rail first.");
    Check(ContainsSlot(StarportSlotsValue, EBuildPlacementSlotType::MainProductionWithAddon,
                       EBuildPlacementFootprintPolicy::RequiresAddonClearance, Point2D(35.5f, 74.0f)),
          SuccessValue, "Starport placement should include the shared production-rail starport slot.");
    Check(ContainsSlot(StarportSlotsValue, EBuildPlacementSlotType::MainStarportWithAddon,
                       EBuildPlacementFootprintPolicy::RequiresAddonClearance, Point2D(36.5f, 50.0f)),
          SuccessValue, "Starport placement should include the deterministic forward starport slot.");

    const std::vector<FBuildPlacementSlot> FallbackSlotsValue =
        BuildPlacementServiceValue.GetStructurePlacementSlots(GameStateDescriptorValue,
                                                              ABILITY_ID::INVALID,
                                                              BuildPlacementContextValue);
    Check(FallbackSlotsValue.size() == 1U, SuccessValue,
          "Unsupported structures should still produce one deterministic support slot.");
    Check(!FallbackSlotsValue.empty() &&
              FallbackSlotsValue.front().SlotId.SlotType == EBuildPlacementSlotType::MainSupportStructure,
          SuccessValue, "Unsupported structures should map to the support-structure slot type.");
    Check(!FallbackSlotsValue.empty() &&
              ArePointsEqual(FallbackSlotsValue.front().BuildPoint, PrimaryAnchorValue),
          SuccessValue, "Unsupported structures should fall back to the primary anchor.");

    return SuccessValue;
}

}  // namespace sc2
