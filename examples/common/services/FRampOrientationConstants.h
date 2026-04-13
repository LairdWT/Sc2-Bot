#pragma once

#include "sc2api/sc2_common.h"

namespace sc2
{

// SC2 main-to-natural ramps only face 4 diagonal directions.
// The ramp direction is the facing from the base interior toward the natural expansion.
// Production buildings extend INTO the base (opposite of ramp direction) along grid axes.
//
// Coordinate system:
//   X increases to the right (east)
//   Y increases upward (north)
//   Addon footprint: always at (+2.5, -0.5) from structure center
//   Structure grid: 3x3 structures snap to 0.5-unit grid
//   "Touching" structures: centers 3 units apart along one axis
//
// Ramp orientation determination:
//   Compare ramp WallCenterPoint to InsideStagingPoint (4 units into base).
//   The delta vector snaps to one of 4 diagonal quadrants.
//
// Production column layout:
//   Column extends along the Y-axis direction that moves into the base.
//   Lateral expansion goes in -X direction (away from addon at +2.5 X).
//
//   Ramp faces NE → column goes -Y (south into base), lateral -X
//   Ramp faces SE → column goes +Y (north into base), lateral -X
//   Ramp faces SW → column goes +Y (north into base), lateral -X
//   Ramp faces NW → column goes -Y (south into base), lateral -X
//
// Grid offsets from ramp barracks center:
//   Factory:   (0, DepthStep)        where DepthStep = +3 or -3
//   Starport:  (0, DepthStep * 2)
//   2nd Rax:   (-6, 0)
//   3rd Rax:   (-6, DepthStep)
//   4th Prod:  (-12, 0)
//   5th Prod:  (-12, DepthStep)
//   EngBay 1:  (0, DepthStep * 3)
//   EngBay 2:  (-6, DepthStep * 3)

enum class ERampOrientation : uint8_t
{
    NorthEast,
    SouthEast,
    SouthWest,
    NorthWest,
    Unknown
};

// Determine ramp orientation from wall center and inside staging point.
// InsideStagingPoint is 4 units behind the wall toward the base interior.
inline ERampOrientation DetermineRampOrientation(const Point2D& WallCenterPointValue,
                                                  const Point2D& InsideStagingPointValue)
{
    const float DeltaXValue = InsideStagingPointValue.x - WallCenterPointValue.x;
    const float DeltaYValue = InsideStagingPointValue.y - WallCenterPointValue.y;

    // The inside staging point is BEHIND the wall (toward base).
    // Ramp faces the OPPOSITE direction of the inside staging offset.
    // InsideStaging in +X,+Y quadrant → base is NE of wall → ramp faces SW
    if (DeltaXValue < 0.0f && DeltaYValue > 0.0f)
    {
        // Inside staging is NW of wall → ramp faces SE
        return ERampOrientation::SouthEast;
    }
    if (DeltaXValue > 0.0f && DeltaYValue > 0.0f)
    {
        // Inside staging is NE of wall → ramp faces SW
        return ERampOrientation::SouthWest;
    }
    if (DeltaXValue > 0.0f && DeltaYValue < 0.0f)
    {
        // Inside staging is SE of wall → ramp faces NW
        return ERampOrientation::NorthWest;
    }
    if (DeltaXValue < 0.0f && DeltaYValue < 0.0f)
    {
        // Inside staging is SW of wall → ramp faces NE
        return ERampOrientation::NorthEast;
    }

    return ERampOrientation::Unknown;
}

// Get the Y-axis depth step for production column placement.
// Positive = column extends upward (north), negative = downward (south).
inline float GetProductionColumnDepthStep(const ERampOrientation RampOrientationValue)
{
    switch (RampOrientationValue)
    {
        case ERampOrientation::NorthEast:
        case ERampOrientation::NorthWest:
            // Ramp faces north → base is south → column extends south (-Y)
            return -3.0f;
        case ERampOrientation::SouthEast:
        case ERampOrientation::SouthWest:
            // Ramp faces south → base is north → column extends north (+Y)
            return 3.0f;
        case ERampOrientation::Unknown:
        default:
            return 3.0f;
    }
}

// Lateral step for additional production columns. Always -X (away from addon at +2.5 X).
constexpr float ProductionColumnLateralStepValue = -6.0f;

// Per-row lateral stagger: each building in the column offsets 1 grid unit
// AWAY from the ramp on the X-axis. This prevents a straight vertical line
// and fits the natural diagonal of the ramp terrain.
inline float GetProductionColumnStaggerStep(const ERampOrientation RampOrientationValue)
{
    switch (RampOrientationValue)
    {
        case ERampOrientation::NorthWest:
        case ERampOrientation::SouthWest:
            // Ramp faces west side → away from ramp = +X (east)
            return 1.0f;
        case ERampOrientation::NorthEast:
        case ERampOrientation::SouthEast:
            // Ramp faces east side → away from ramp = -X (west)
            return -1.0f;
        case ERampOrientation::Unknown:
        default:
            return 1.0f;
    }
}

}  // namespace sc2
