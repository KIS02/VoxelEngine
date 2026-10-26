#pragma once

#include "math/Vec3.h"
#include "render/backends/software/RasterVertex.h"


namespace ve
{
    Vec3 interpolatePerspectiveNormal( const RasterVertex& a, const RasterVertex& b,
                                       const RasterVertex& c, const Vec3& weights, float interpolatedInverseW );

    Vec3 interpolatePerspectiveWorldPosition( const RasterVertex& a, const RasterVertex& b,
                                               const RasterVertex& c, const Vec3& weights,
                                               float interpolatedInverseW );
}