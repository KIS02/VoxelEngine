#include "render/backends/software/RasterInterpolation.h"

#include <cmath>
#include <stdexcept>


namespace ve
{
    namespace
    {
        Vec3 interpolateAttribute( const Vec3& aOverW, const Vec3& bOverW, const Vec3& cOverW,
                                   const Vec3& weights, float interpolatedInverseW ) {
            if (!std::isfinite(interpolatedInverseW) || interpolatedInverseW == 0.0f)
                throw std::invalid_argument("Perspective interpolation requires nonzero inverse W");

            const Vec3 numerator = aOverW * weights.x + bOverW * weights.y + cOverW * weights.z;
            return numerator / interpolatedInverseW;
        }
    }


    Vec3 interpolatePerspectiveNormal( const RasterVertex& a, const RasterVertex& b,
                                       const RasterVertex& c, const Vec3& weights, float interpolatedInverseW ) {
        return interpolateAttribute(a.normalOverW, b.normalOverW, c.normalOverW,
                                    weights, interpolatedInverseW);
    }


    Vec3 interpolatePerspectiveWorldPosition( const RasterVertex& a, const RasterVertex& b,
                                               const RasterVertex& c, const Vec3& weights,
                                               float interpolatedInverseW ) {
        return interpolateAttribute(a.worldPositionOverW, b.worldPositionOverW, c.worldPositionOverW,
                                    weights, interpolatedInverseW);
    }
}