#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>

#include "render/lighting/DirectionalLight.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    bool nearlyEqual( float a, float b, float epsilon = 0.00001f ) {
        return std::fabs(a - b) <= epsilon;
    }


    void testNormalWinding() {
        const ve::Vec3 a{ 0.0f, 0.0f, 1.0f };
        const ve::Vec3 b{ 1.0f, 0.0f, 1.0f };
        const ve::Vec3 c{ 1.0f, 1.0f, 1.0f };

        const ve::Vec3 front = ve::triangleNormal(a, b, c);
        const ve::Vec3 back = ve::triangleNormal(a, c, b);

        require(nearlyEqual(front.z, 1.0f),
                "Counterclockwise triangle must face +Z");

        require(nearlyEqual(back.z, -1.0f),
                "Reversed winding must face -Z");

        require(nearlyEqual(front.length(), 1.0f),
                "Normal must be normalized");

        const ve::Vec3 degenerate = ve::triangleNormal(a, a, c);

        require(nearlyEqual(degenerate.lengthSquared(), 0.0f),
                "Degenerate triangle must return zero normal");

        std::cout << "[PASS] Triangle normal and winding\n";
    }


    void testDiffuseFactor() {
        const ve::Vec3 normal{ 0.0f, 0.0f, 2.0f };

        require(nearlyEqual(ve::diffuseFactor(normal, ve::Vec3{ 0.0f, 0.0f, 3.0f }), 1.0f),
                "Light-facing surface must have full diffuse intensity");

        require(nearlyEqual(ve::diffuseFactor(normal, ve::Vec3{ 1.0f, 0.0f, 0.0f }), 0.0f),
                "Side-facing surface must have zero diffuse intensity");

        require(nearlyEqual(ve::diffuseFactor(normal, ve::Vec3{ 0.0f, 0.0f, -1.0f }), 0.0f),
                "Back-facing surface must have zero diffuse intensity");

        require(nearlyEqual(ve::diffuseFactor(ve::Vec3{}, ve::Vec3{ 0.0f, 0.0f, 1.0f }), 0.0f),
                "Zero normal must have zero diffuse intensity");

        std::cout << "[PASS] Normalized Lambert diffuse factor\n";
    }


    void testLightingFactor() {
        ve::DirectionalLight light{};
        light.directionToLight = ve::Vec3{ 0.0f, 0.0f, 1.0f };
        light.ambient = 0.2f;
        light.diffuse = 0.8f;

        require(nearlyEqual(ve::lightingFactor(ve::Vec3{ 0.0f, 0.0f, 1.0f }, light), 1.0f),
                "Fully illuminated surface should have intensity 1");

        require(nearlyEqual(ve::lightingFactor(ve::Vec3{ 0.0f, 0.0f, -1.0f }, light), 0.2f),
                "Back-facing surface should receive only ambient light");

        light.diffuse = 2.0f;

        require(nearlyEqual(ve::lightingFactor(ve::Vec3{ 0.0f, 0.0f, 1.0f }, light), 1.0f),
                "Lighting output should be clamped to 1");

        std::cout << "[PASS] Ambient plus diffuse lighting\n";
    }
}


int main() {
    try {
        testNormalWinding();
        testDiffuseFactor();
        testLightingFactor();

        std::cout << "All DirectionalLight unit tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}