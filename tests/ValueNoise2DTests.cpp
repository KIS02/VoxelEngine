#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "voxel/generation/CoordinateHash.h"
#include "voxel/generation/ValueNoise2D.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    void testLatticePoints() {
        const ve::WorldSeed seed = 1234;

        for (int z = -3; z <= 3; ++z) {
            for (int x = -3; x <= 3; ++x) {
                const double actual = ve::valueNoise2D(seed, x, z);
                const double expected = ve::latticeValue(seed, x, z);

                require(actual == expected, "Integer lattice points must match latticeValue");
            }
        }

        std::cout << "[PASS] Lattice points\n";
    }


    void testMidpointInterpolation() {
        const ve::WorldSeed seed = 1234;
        const double v00 = ve::latticeValue(seed, 0, 0);
        const double v10 = ve::latticeValue(seed, 1, 0);
        const double v01 = ve::latticeValue(seed, 0, 1);
        const double v11 = ve::latticeValue(seed, 1, 1);
        const double expected = (v00 + v10 + v01 + v11) / 4.0;

        require(std::fabs(ve::valueNoise2D(seed, 0.5, 0.5) - expected) < 1e-12,
                "Cell midpoint must be the average of four corners");

        std::cout << "[PASS] Midpoint interpolation\n";
    }


    void testNegativeCoordinates() {
        const ve::WorldSeed seed = 1234;
        const double left = ve::latticeValue(seed, -1, 2);
        const double right = ve::latticeValue(seed, 0, 2);
        const double t = 0.75;
        const double s = t * t * (3.0 - 2.0 * t);
        const double expected = left + (right - left) * s;

        require(std::fabs(ve::valueNoise2D(seed, -0.25, 2.0) - expected) < 1e-12,
                "Negative coordinate interpolation must use floor");

        std::cout << "[PASS] Negative coordinates\n";
    }


    void testDeterminismAndBoundaries() {
        const ve::WorldSeed seed = 777;
        const double original = ve::valueNoise2D(seed, -15.375, 28.625);

        ve::valueNoise2D(seed, 900.125, -8.5);

        require(ve::valueNoise2D(seed, -15.375, 28.625) == original,
                "Sampling order must not change the result");

        const double before = ve::valueNoise2D(seed, 15.999, 7.25);
        const double after = ve::valueNoise2D(seed, 16.001, 7.25);

        require(std::fabs(before - after) < 0.001,
                "Noise must be continuous across a Chunk boundary");

        std::cout << "[PASS] Determinism and Chunk boundary\n";
    }


    void testRangeAndInvalidCoordinates() {
        for (int z = -10; z <= 10; ++z) {
            for (int x = -10; x <= 10; ++x) {
                const double value = ve::valueNoise2D(1234, x + 0.25, z + 0.75);

                require(value >= 0.0 && value < 1.0, "Noise must be in [0, 1)");
            }
        }

        bool nonFiniteRejected = false;

        try {
            ve::valueNoise2D(1234, std::numeric_limits<double>::infinity(), 0.0);
        } catch (const std::invalid_argument&) {
            nonFiniteRejected = true;
        }

        require(nonFiniteRejected, "Non-finite coordinates must be rejected");

        bool outOfRangeRejected = false;

        try {
            ve::valueNoise2D(1234, static_cast<double>(std::numeric_limits<int>::max()), 0.0);
        } catch (const std::out_of_range&) {
            outOfRangeRejected = true;
        }

        require(outOfRangeRejected, "Coordinates beyond safe neighbor range must be rejected");

        std::cout << "[PASS] Range and invalid coordinates\n";
    }
}


int main() {
    try {
        testLatticePoints();
        testMidpointInterpolation();
        testNegativeCoordinates();
        testDeterminismAndBoundaries();
        testRangeAndInvalidCoordinates();

        std::cout << "All ValueNoise2D unit tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}