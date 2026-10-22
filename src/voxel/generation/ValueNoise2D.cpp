#include "voxel/generation/ValueNoise2D.h"

#include <cmath>
#include <limits>
#include <stdexcept>

#include "voxel/generation/CoordinateHash.h"


namespace ve
{
    namespace
    {
        double lerp( double a, double b, double t ) {
            return a + (b - a) * t;
        }

        double smoothstep( double t ) {
            return t * t * (3.0 - 2.0 * t);
        }
    }


    double valueNoise2D( WorldSeed seed, double worldX, double worldZ ) {
        if (!std::isfinite(worldX) || !std::isfinite(worldZ))
            throw std::invalid_argument("Noise coordinates must be finite");

        const double minCoordinate = static_cast<double>(std::numeric_limits<int>::min());
        const double maxCoordinate = static_cast<double>(std::numeric_limits<int>::max());

        if (worldX < minCoordinate || worldX >= maxCoordinate ||
            worldZ < minCoordinate || worldZ >= maxCoordinate)
            throw std::out_of_range("Noise coordinates exceed supported range");

        const int x0 = static_cast<int>(std::floor(worldX));
        const int z0 = static_cast<int>(std::floor(worldZ));
        const int x1 = x0 + 1;
        const int z1 = z0 + 1;

        const double tx = worldX - x0;
        const double tz = worldZ - z0;
        const double sx = smoothstep(tx);
        const double sz = smoothstep(tz);

        const double v00 = latticeValue(seed, x0, z0);
        const double v10 = latticeValue(seed, x1, z0);
        const double v01 = latticeValue(seed, x0, z1);
        const double v11 = latticeValue(seed, x1, z1);

        const double bottom = lerp(v00, v10, sx);
        const double top = lerp(v01, v11, sx);

        return lerp(bottom, top, sz);
    }
}