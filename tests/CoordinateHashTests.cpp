#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>

#include "voxel/generation/CoordinateHash.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    void testDeterminism() {
        const std::uint32_t first = ve::coordinateHash(1234, -32, 19);
        const std::uint32_t second = ve::coordinateHash(1234, -32, 19);

        require(first == second, "Identical inputs must produce identical hashes");
        require(first == 3616887662U, "Hash result must match the reference value");

        std::cout << "[PASS] Determinism\n";
    }


    void testInputSensitivity() {
        const std::uint32_t original = ve::coordinateHash(1234, -32, 19);

        require(original != ve::coordinateHash(1235, -32, 19), "Different seed test failed");
        require(original != ve::coordinateHash(1234, -31, 19), "Different X test failed");
        require(original != ve::coordinateHash(1234, -32, 20), "Different Z test failed");
        require(ve::coordinateHash(1234, -1, 19) != ve::coordinateHash(1234, 1, 19),
                "Negative and positive coordinate test failed");

        std::cout << "[PASS] Input variation\n";
    }


    void testLatticeRange() {
        for (int z = -32; z <= 32; ++z) {
            for (int x = -32; x <= 32; ++x) {
                const float value = ve::latticeValue(1234, x, z);

                require(value >= 0.0f && value < 1.0f,
                        "Lattice value must be within [0, 1)");
            }
        }

        std::cout << "[PASS] Lattice range\n";
    }
}


int main() {
    try {
        testDeterminism();
        testInputSensitivity();
        testLatticeRange();

        std::cout << "All CoordinateHash unit tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}