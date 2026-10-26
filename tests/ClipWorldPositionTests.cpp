#include <array>
#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>

#include "render/backends/software/ClipSpaceClipper.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    bool near( float a, float b ) {
        return std::fabs(a - b) < 0.0001f;
    }


    bool nearVector( const ve::Vec3& a, const ve::Vec3& b ) {
        return near(a.x, b.x) && near(a.y, b.y) && near(a.z, b.z);
    }


    void testWorldPositionInterpolation() {
        const ve::ClipVertex a{
            ve::Vec4{ 0.0f, 0.0f, -2.0f, 1.0f },
            ve::Vec2{},
            ve::makeColor(255, 255, 255),
            ve::Vec3{ 0.0f, 0.0f, 1.0f },
            ve::Vec3{ 0.0f, 0.0f, 0.0f }
        };

        const ve::ClipVertex b{
            ve::Vec4{ -0.5f, -0.5f, 0.0f, 1.0f },
            ve::Vec2{},
            ve::makeColor(255, 255, 255),
            ve::Vec3{ 0.0f, 0.0f, 1.0f },
            ve::Vec3{ 4.0f, 0.0f, 0.0f }
        };

        const ve::ClipVertex c{
            ve::Vec4{ 0.5f, -0.5f, 0.0f, 1.0f },
            ve::Vec2{},
            ve::makeColor(255, 255, 255),
            ve::Vec3{ 0.0f, 0.0f, 1.0f },
            ve::Vec3{ 0.0f, 6.0f, 0.0f }
        };

        std::array<ve::ClipVertex, 16> output{};

        const std::size_t count = ve::ClipSpaceClipper::clipTriangle(a, b, c, output);

        require(count == 4, "Near-plane clipping must produce four vertices");

        bool foundAB = false;
        bool foundAC = false;

        for (std::size_t i = 0; i < count; ++i) {
            if (!near(output[i].position.z, -1.0f))
                continue;

            if (nearVector(output[i].worldPosition, ve::Vec3{ 2.0f, 0.0f, 0.0f }))
                foundAB = true;

            if (nearVector(output[i].worldPosition, ve::Vec3{ 0.0f, 3.0f, 0.0f }))
                foundAC = true;
        }

        require(foundAB, "World Position on edge AB was not interpolated correctly");
        require(foundAC, "World Position on edge AC was not interpolated correctly");

        std::cout << "[PASS] Clipped World Position interpolation\n";
    }
}


int main() {
    try {
        testWorldPositionInterpolation();

        std::cout << "All ClipWorldPosition tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}