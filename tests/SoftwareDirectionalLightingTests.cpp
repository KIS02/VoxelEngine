#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>

#include "assets/texture/Texture2D.h"
#include "math/Mat4.h"
#include "render/backends/software/DepthBuffer.h"
#include "render/backends/software/FrameBuffer.h"
#include "render/backends/software/Rasterizer.h"
#include "render/backends/software/RasterInterpolation.h"
#include "render/lighting/Lighting.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    bool near( float a, float b, float epsilon = 0.0001f ) {
        return std::fabs(a - b) < epsilon;
    }


    void testWorldSpaceNormalTransformation() {
        const ve::NormalMatrix3 matrix = ve::calculateNormalMatrix(
            ve::Mat4::scale(ve::Vec3{ 2.0f, 1.0f, 4.0f })
        );
        const ve::Vec3 normal = ve::transformNormal(matrix, ve::Vec3{ 1.0f, 1.0f, 0.0f });
        const ve::Vec3 tangent{ 2.0f, -1.0f, 0.0f };

        require(near(normal.x, 0.5f) && near(normal.y, 1.0f),
                "Normal Matrix must reverse non-uniform scale");
        require(near(ve::dot(normal, tangent), 0.0f),
                "Transformed normal must stay perpendicular to transformed tangent");
        std::cout << "[PASS] World-space normal transformation\n";
    }


    void testPerspectiveCorrectNormal() {
        ve::RasterVertex a{};
        ve::RasterVertex b{};
        ve::RasterVertex c{};
        a.inverseW = 1.0f;
        b.inverseW = 0.5f;
        c.inverseW = 0.25f;
        a.normalOverW = ve::Vec3{ 1.0f, 0.0f, 0.0f };
        b.normalOverW = ve::Vec3{ 0.0f, 0.5f, 0.0f };
        c.normalOverW = ve::Vec3{ 0.0f, 0.0f, 0.25f };

        const ve::Vec3 weights{ 0.25f, 0.25f, 0.5f };
        const float inverseW = weights.x * a.inverseW + weights.y * b.inverseW + weights.z * c.inverseW;
        const ve::Vec3 normal = ve::interpolatePerspectiveNormal(a, b, c, weights, inverseW);

        require(near(inverseW, 0.5f), "Interpolated inverse W is incorrect");
        require(near(normal.x, 0.5f) && near(normal.y, 0.25f) && near(normal.z, 0.25f),
                "Perspective-correct normal must divide weighted normal over W by inverse W");
        std::cout << "[PASS] Perspective-correct normal interpolation\n";
    }


    void testLambertTextureColorAndAlpha() {
        const ve::DirectionalLight light{ ve::Vec3{ 0.0f, 1.0f, 0.0f }, 0.2f, 0.8f };
        const ve::Color32 baseColor = ve::makeColor(200, 160, 120, 73);
        const ve::Color32 facing = ve::shadeColorLambert(baseColor, ve::Vec3{ 0.0f, 1.0f, 0.0f }, light);
        const ve::Color32 away = ve::shadeColorLambert(baseColor, ve::Vec3{ 0.0f, -1.0f, 0.0f }, light);

        require(facing == baseColor, "Fully lit texture must retain RGB and alpha");
        require(away == ve::makeColor(95, 75, 55, 73), "Ambient-only texture must attenuate RGB but retain alpha");
        std::cout << "[PASS] Lambert color channels and alpha preservation\n";
    }


    ve::RasterVertex vertex( float x, float y, const ve::Vec3& normal ) {
        ve::RasterVertex result{};
        result.position = ve::Vec2{ x, y };
        result.depth = 0.3f;
        result.inverseW = 1.0f;
        result.normal = normal;
        result.normalOverW = normal;
        result.uv = ve::Vec2{ 0.5f, 0.5f };
        result.uvOverW = result.uv;
        return result;
    }


    void testActualRasterPixels() {
        ve::FrameBuffer frame(16, 16);
        ve::DepthBuffer depth(16, 16);
        ve::Rasterizer rasterizer(frame, depth);
        ve::Texture2D texture(1, 1);
        texture.setPixel(0, 0, ve::makeColor(200, 160, 120, 73));
        const ve::DirectionalLight light{ ve::Vec3{ 0.0f, 1.0f, 0.0f }, 0.2f, 0.8f };
        const ve::RasterVertex a = vertex(2.0f, 2.0f, ve::Vec3{ 0.0f, -1.0f, 0.0f });
        const ve::RasterVertex b = vertex(2.0f, 12.0f, ve::Vec3{ 0.0f, -1.0f, 0.0f });
        const ve::RasterVertex c = vertex(12.0f, 2.0f, ve::Vec3{ 0.0f, -1.0f, 0.0f });

        frame.clear(ve::makeColor(0, 0, 0));
        depth.clear(1.0f);
        rasterizer.drawTriangleTextured(a, b, c, texture, &light);
        require(frame.pixel(4, 4) == ve::makeColor(95, 75, 55, 73),
                "Rasterizer must shade final sampled texture at pixel center");
        require(near(depth.depth(4, 4), 0.3f), "Depth test must remain active");
        require(frame.pixel(14, 14) == ve::makeColor(0, 0, 0), "Outside pixels must remain unchanged");

        frame.clear(ve::makeColor(0, 0, 0));
        depth.clear(1.0f);
        rasterizer.drawTriangleTextured(a, b, c, texture);
        require(frame.pixel(4, 4) == ve::makeColor(200, 160, 120, 73),
                "Legacy 4-argument draw must remain unlit");
        std::cout << "[PASS] Rasterizer lit pixels, depth and legacy compatibility\n";
    }
}


int main() {
    try {
        testWorldSpaceNormalTransformation();
        testPerspectiveCorrectNormal();
        testLambertTextureColorAndAlpha();
        testActualRasterPixels();
        std::cout << "All SoftwareDirectionalLighting unit tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
