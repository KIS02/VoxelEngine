#include <array>
#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>

#include "assets/material/MaterialData.h"
#include "assets/texture/Texture2D.h"

#include "render/backends/software/ClipSpaceClipper.h"
#include "render/backends/software/DepthBuffer.h"
#include "render/backends/software/FrameBuffer.h"
#include "render/backends/software/Rasterizer.h"
#include "render/backends/software/VertexProcessor.h"

#include "render/lighting/SpecularLighting.h"


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


    void testVertexTransfer() {
        ve::ClipVertex clip{};

        clip.position = ve::Vec4{ 0.4f, 0.2f, 0.6f, 2.0f };
        clip.uv = ve::Vec2{ 0.6f, 0.2f };
        clip.normal = ve::Vec3{ 0.0f, 1.0f, 0.0f };
        clip.worldPosition = ve::Vec3{ 4.0f, 8.0f, 12.0f };

        const ve::RasterVertex raster = ve::VertexProcessor::toRasterSpace(
            clip, ve::Viewport(800, 600)
        );

        require(near(raster.inverseW, 0.5f),
                "Inverse W incorrect");

        require(nearVector(raster.worldPosition, ve::Vec3{ 4.0f, 8.0f, 12.0f }),
                "World Position missing");

        require(nearVector(raster.worldPositionOverW, ve::Vec3{ 2.0f, 4.0f, 6.0f }),
                "World Position over W incorrect");

        std::cout << "[PASS] Clip-to-raster World Position\n";
    }


    void testPerspectivePosition() {
        ve::RasterVertex a{};
        ve::RasterVertex b{};
        ve::RasterVertex c{};

        a.inverseW = 1.0f;
        b.inverseW = 0.5f;
        c.inverseW = 0.25f;

        a.worldPositionOverW = ve::Vec3{};
        b.worldPositionOverW = ve::Vec3{ 2.0f, 0.0f, 0.0f };
        c.worldPositionOverW = ve::Vec3{ 0.0f, 2.0f, 0.0f };

        const ve::Vec3 weights{ 0.25f, 0.25f, 0.5f };

        const float inverseW =
            a.inverseW * weights.x +
            b.inverseW * weights.y +
            c.inverseW * weights.z;

        const ve::Vec3 result = ve::interpolatePerspectiveWorldPosition(
            a, b, c, weights, inverseW
        );

        require(
            near(inverseW, 0.5f) &&
            nearVector(result, ve::Vec3{ 1.0f, 2.0f, 0.0f }),
            "Perspective-correct World Position incorrect"
        );

        std::cout << "[PASS] Perspective-correct World Position\n";
    }


    void testWorldPositionClipping() {
        ve::ClipVertex a{};
        ve::ClipVertex b{};
        ve::ClipVertex c{};

        a.position = ve::Vec4{ 0.0f, 0.0f, -2.0f, 1.0f };
        b.position = ve::Vec4{ -0.5f, -0.5f, 0.0f, 1.0f };
        c.position = ve::Vec4{ 0.5f, -0.5f, 0.0f, 1.0f };

        a.worldPosition = ve::Vec3{};
        b.worldPosition = ve::Vec3{ 4.0f, 0.0f, 0.0f };
        c.worldPosition = ve::Vec3{ 0.0f, 6.0f, 0.0f };

        std::array<ve::ClipVertex, 16> output{};

        const std::size_t count = ve::ClipSpaceClipper::clipTriangle(
            a, b, c, output
        );

        require(count == 4,
                "Clipping must produce four vertices");

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

        require(foundAB && foundAC,
                "Clipping must interpolate World Positions");

        std::cout << "[PASS] World Position during Clipping\n";
    }


    void testRasterizedSpecular() {
        ve::FrameBuffer framebuffer(16, 16);
        ve::DepthBuffer depth(16, 16);
        ve::Rasterizer rasterizer(framebuffer, depth);

        ve::Texture2D texture(1, 1);
        texture.setPixel(0, 0, ve::makeColor(100, 120, 150, 73));

        ve::MaterialData material{ &texture, 0.4f, 32.0f };

        const ve::DirectionalLight light{
            ve::Vec3{ 0.0f, 1.0f, 0.0f }, 0.2f, 0.8f
        };

        const ve::Vec3 camera{ 0.0f, 5.0f, 0.0f };
        const ve::Vec3 sideCamera{ 5.0f, 0.0f, 0.0f };


        // Create One Front-facing Triangle

        ve::RasterVertex a{};
        ve::RasterVertex b{};
        ve::RasterVertex c{};

        a.position = ve::Vec2{ 2.0f, 2.0f };
        b.position = ve::Vec2{ 2.0f, 12.0f };
        c.position = ve::Vec2{ 12.0f, 2.0f };

        a.depth = 0.3f;
        b.depth = 0.3f;
        c.depth = 0.3f;

        a.inverseW = 1.0f;
        b.inverseW = 1.0f;
        c.inverseW = 1.0f;

        a.normalOverW = ve::Vec3{ 0.0f, 1.0f, 0.0f };
        b.normalOverW = ve::Vec3{ 0.0f, 1.0f, 0.0f };
        c.normalOverW = ve::Vec3{ 0.0f, 1.0f, 0.0f };

        a.worldPositionOverW = ve::Vec3{};
        b.worldPositionOverW = ve::Vec3{};
        c.worldPositionOverW = ve::Vec3{};


        // Test Full Specular Highlight

        framebuffer.clear(ve::makeColor(0, 0, 0));
        depth.clear(1.0f);

        rasterizer.drawTriangleTextured(
            a, b, c, texture, &light, &material, &camera
        );

        const ve::Color32 frontal = framebuffer.pixel(4, 4);

        require(frontal == ve::makeColor(202, 222, 252, 73),
                "Expected Specular-colored pixel");

        require(near(depth.depth(4, 4), 0.3f),
                "Depth not updated");


        // Test Equal-depth Rejection

        rasterizer.drawTriangleTextured(
            a, b, c, texture, &light, &material, &sideCamera
        );

        require(framebuffer.pixel(4, 4) == frontal,
                "Equal-depth fragment must not overwrite pixel");


        // Test Camera Direction Change

        framebuffer.clear(ve::makeColor(0, 0, 0));
        depth.clear(1.0f);

        rasterizer.drawTriangleTextured(
            a, b, c, texture, &light, &material, &sideCamera
        );

        const ve::Color32 side = framebuffer.pixel(4, 4);

        require(ve::getRed(side) < ve::getRed(frontal),
                "Side Camera should reduce Specular");

        require(ve::getAlpha(side) == 73,
                "Alpha must be preserved");


        // Test Lambert Compatibility

        framebuffer.clear(ve::makeColor(0, 0, 0));
        depth.clear(1.0f);

        rasterizer.drawTriangleTextured(
            a, b, c, texture, &light
        );

        require(
            framebuffer.pixel(4, 4) == ve::makeColor(100, 120, 150, 73),
            "Lambert fallback must remain available"
        );

        std::cout << "[PASS] Specular pixels, Camera movement, Alpha and Depth\n";
    }
}


int main() {
    try {
        testVertexTransfer();
        testPerspectivePosition();
        testWorldPositionClipping();
        testRasterizedSpecular();

        std::cout << "All SoftwareSpecularPipeline tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}