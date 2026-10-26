#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>

#include "assets/texture/Texture2D.h"
#include "math/Mat4.h"
#include "render/backends/software/Viewport.h"
#include "render/lighting/NormalMatrix3.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    bool near( float a, float b ) {
        return std::fabs(a - b) < 0.0001f;
    }


    void testSampler() {
        ve::Texture2D texture(4, 4);

        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < 4; ++x) {
                texture.setPixel(
                    x, y,
                    ve::makeColor(
                        static_cast<std::uint8_t>(x * 50 + 10),
                        static_cast<std::uint8_t>(y * 50 + 20),
                        30
                    )
                );
            }
        }

        require(texture.sampleNearest(ve::Vec2{ 0.2f, 0.4f }) == texture.pixel(0, 1),
                "Nearest lower texel mismatch");

        require(texture.sampleNearest(ve::Vec2{ 0.8f, 0.6f }) == texture.pixel(3, 2),
                "Nearest upper texel mismatch");

        require(texture.sampleNearest(ve::Vec2{ -0.2f, 1.2f }) == texture.pixel(0, 3),
                "Clamp-to-edge UV mismatch");

        std::cout << "[PASS] CPU Nearest and Clamp-to-Edge contract\n";
    }


    void testDepthMapping() {
        const ve::Viewport viewport(33, 33);

        const auto transform = [&viewport]( float z ) {
            return viewport.transform(
                ve::Vec3{ 0.0f, 0.0f, z },
                ve::makeColor(255, 255, 255)
            ).depth;
        };

        require(near(transform(-1.0f), 0.0f),
                "Near-plane Depth mismatch");

        require(near(transform(0.0f), 0.5f),
                "Middle Depth mismatch");

        require(near(transform(1.0f), 1.0f),
                "Far-plane Depth mismatch");

        std::cout << "[PASS] OpenGL-compatible Window Depth\n";
    }


    void testModelTransforms() {
        const ve::Mat4 model =
            ve::Mat4::rotationZ(0.6f) *
            ve::Mat4::scale(ve::Vec3{ 2.0f, 0.5f, 1.0f });

        const ve::Vec4 tangent = model * ve::Vec4{
            1.0f, -1.0f, 0.0f, 0.0f
        };

        const ve::Vec4 incorrectNormal = model * ve::Vec4{
            1.0f, 1.0f, 0.0f, 0.0f
        };

        const float incorrectDot =
            tangent.x * incorrectNormal.x +
            tangent.y * incorrectNormal.y +
            tangent.z * incorrectNormal.z;

        require(!near(incorrectDot, 0.0f),
                "Model Matrix unexpectedly preserved the Normal");


        // Calculate the inverse-transpose Normal Matrix

        const ve::NormalMatrix3 normalMatrix = ve::calculateNormalMatrix(model);

        const ve::Vec3 correctNormal{
            normalMatrix.m[0][0] + normalMatrix.m[0][1],
            normalMatrix.m[1][0] + normalMatrix.m[1][1],
            normalMatrix.m[2][0] + normalMatrix.m[2][1]
        };

        const float correctDot =
            tangent.x * correctNormal.x +
            tangent.y * correctNormal.y +
            tangent.z * correctNormal.z;

        require(near(correctDot, 0.0f),
                "Normal Matrix did not preserve perpendicularity");

        std::cout << "[PASS] Rotation and Nonuniform Normal Matrix\n";
    }
}


int main() {
    try {
        testSampler();
        testDepthMapping();
        testModelTransforms();

        std::cout << "All RenderingParityContract tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}