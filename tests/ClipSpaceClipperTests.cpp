#include <array>
#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>

#include "render/backends/software/ClipSpaceClipper.h"



namespace
{
    void require( bool condition, const char* message ) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }



    bool near( float a, float b ) {
        return std::fabs(a - b) < 0.0001f;
    }



    ve::ClipVertex vertex( float x, float y, float z, float w,
                           float u = 0.0f, float v = 0.0f ) {
        return ve::ClipVertex{
            ve::Vec4{ x, y, z, w },
            ve::Vec2{ u, v },
            ve::makeColor(255, 255, 255)
        };
    }



    void checkInside( const std::array<ve::ClipVertex, 16>& output, std::size_t count ) {
        for (std::size_t i = 0; i < count; ++i) {
            const ve::Vec4& p = output[i].position;

            constexpr float epsilon = 0.0001f;

            require(p.w > 0.0f, "Clipped vertex must have positive w");

            require(p.x >= -p.w - epsilon && p.x <= p.w + epsilon, "X outside clip volume");
            require(p.y >= -p.w - epsilon && p.y <= p.w + epsilon, "Y outside clip volume");
            require(p.z >= -p.w - epsilon && p.z <= p.w + epsilon, "Z outside clip volume");
        }
    }



    void testVisibleTriangle() {
        std::array<ve::ClipVertex, 16> output{};

        const auto count = ve::ClipSpaceClipper::clipTriangle(
            vertex(-0.5f, -0.5f, 0.0f, 1.0f),
            vertex(0.5f, -0.5f, 0.0f, 1.0f),
            vertex(0.0f, 0.5f, 0.0f, 1.0f),
            output
        );

        require(count == 3, "Visible triangle should remain intact");

        checkInside(output, count);

        std::cout << "[PASS] Fully visible triangle preserved\n";
    }



    void testNearPlaneCrossing() {
        std::array<ve::ClipVertex, 16> output{};

        const auto count = ve::ClipSpaceClipper::clipTriangle(
            vertex(0.0f, 0.0f, -2.0f, 1.0f),
            vertex(0.5f, -0.5f, 0.0f, 1.0f),
            vertex(-0.5f, 0.5f, 0.0f, 1.0f),
            output
        );

        require(count == 4, "Crossing Near Plane should form a quad");

        checkInside(output, count);

        std::cout << "[PASS] Near Plane triangle clipping\n";
    }



    void testBehindCamera() {
        std::array<ve::ClipVertex, 16> output{};

        const auto count = ve::ClipSpaceClipper::clipTriangle(
            vertex(-0.2f, -0.2f, 0.0f, -1.0f),
            vertex(0.2f, -0.2f, 0.0f, -1.0f),
            vertex(0.0f, 0.2f, 0.0f, -1.0f),
            output
        );

        require(count == 0, "Triangle behind the camera must be removed");

        std::cout << "[PASS] Behind-camera triangle rejected\n";
    }



    void testLeftAndRightPlanes() {
        std::array<ve::ClipVertex, 16> output{};

        auto count = ve::ClipSpaceClipper::clipTriangle(
            vertex(-2.0f, 0.0f, 0.0f, 1.0f),
            vertex(0.0f, -0.5f, 0.0f, 1.0f),
            vertex(0.0f, 0.5f, 0.0f, 1.0f),
            output
        );

        require(count == 4, "Left Plane should split triangle into quad");

        checkInside(output, count);



        count = ve::ClipSpaceClipper::clipTriangle(
            vertex(2.0f, 0.0f, 0.0f, 1.0f),
            vertex(0.0f, -0.5f, 0.0f, 1.0f),
            vertex(0.0f, 0.5f, 0.0f, 1.0f),
            output
        );

        require(count == 4, "Right Plane should split triangle into quad");

        checkInside(output, count);

        std::cout << "[PASS] Left and Right Plane clipping\n";
    }



    void testTopBottomAndFarPlanes() {
        std::array<ve::ClipVertex, 16> output{};

        for (int dimension = 0; dimension < 3; ++dimension) {
            ve::ClipVertex a = vertex(0.0f, 0.0f, 0.0f, 1.0f);

            if (dimension == 0) a.position.y = 2.0f;
            if (dimension == 1) a.position.y = -2.0f;
            if (dimension == 2) a.position.z = 2.0f;



            const auto count = ve::ClipSpaceClipper::clipTriangle(
                a,
                vertex(-0.5f, -0.5f, 0.0f, 1.0f),
                vertex(0.5f, 0.5f, 0.0f, 1.0f),
                output
            );

            require(count >= 3, "Triangle must survive plane clipping");

            checkInside(output, count);
        }

        std::cout << "[PASS] Top, Bottom and Far Plane clipping\n";
    }



    void testUvInterpolation() {
        std::array<ve::ClipVertex, 16> output{};

        const auto count = ve::ClipSpaceClipper::clipTriangle(
            vertex(0.0f, 0.0f, -2.0f, 1.0f, 1.0f, 1.0f),
            vertex(-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f),
            vertex(0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f),
            output
        );

        require(count == 4, "Expected 4 vertices after UV clipping");



        bool found = false;

        for (std::size_t i = 0; i < count; ++i) {
            if (near(output[i].position.z, -1.0f) &&
                near(output[i].uv.x, 0.5f) &&
                near(output[i].uv.y, 0.5f)) {
                found = true;
            }
        }

        require(found, "UV must be interpolated at Near Plane intersections");

        std::cout << "[PASS] UV interpolation at clipping intersection\n";
    }
}



int main() {
    try {
        testVisibleTriangle();
        testNearPlaneCrossing();
        testBehindCamera();
        testLeftAndRightPlanes();
        testTopBottomAndFarPlanes();
        testUvInterpolation();

        std::cout << "\nAll Clip Space Clipper tests passed.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}