#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>

#include "render/backends/software/VertexProcessor.h"


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


    void testClipToRasterAttributes() {
        const ve::ClipVertex clip{
            ve::Vec4{ 0.4f, 0.2f, 0.6f, 2.0f },
            ve::Vec2{ 0.6f, 0.2f },
            ve::makeColor(11, 22, 33, 44),
            ve::Vec3{ 0.0f, 1.0f, 0.0f },
            ve::Vec3{ 4.0f, 8.0f, 12.0f }
        };

        const ve::Viewport viewport(800, 600);

        const ve::RasterVertex raster = ve::VertexProcessor::toRasterSpace(
            clip, viewport
        );

        require(near(raster.inverseW, 0.5f),
                "Inverse W mismatch");

        require(near(raster.position.x, 480.0f) && near(raster.position.y, 270.0f),
                "Screen position mismatch");

        require(near(raster.depth, 0.3f),
                "Depth mismatch");

        require(near(raster.uvOverW.x, 0.3f) && near(raster.uvOverW.y, 0.1f),
                "UV/W mismatch");

        require(nearVector(raster.normalOverW, ve::Vec3{ 0.0f, 0.5f, 0.0f }),
                "Normal/W mismatch");

        require(nearVector(raster.worldPosition, ve::Vec3{ 4.0f, 8.0f, 12.0f }),
                "World position not preserved");

        require(nearVector(raster.worldPositionOverW, ve::Vec3{ 2.0f, 4.0f, 6.0f }),
                "World position/W mismatch");

        std::cout << "[PASS] Clip to Raster world position and attributes\n";
    }


    void testLegacyProcessWorldPosition() {
        ve::Vertex vertex{};
        vertex.position = ve::Vec3{ 0.25f, 0.5f, -0.1f };
        vertex.normal = ve::Vec3{ 0.0f, 1.0f, 0.0f };
        vertex.uv = ve::Vec2{ 0.2f, 0.7f };

        const ve::Mat4 model = ve::Mat4::translation(
            ve::Vec3{ 0.1f, -0.2f, 0.3f }
        );

        const ve::Mat4 identity = ve::Mat4::identity();
        const ve::Viewport viewport(800, 600);

        const ve::RasterVertex raster = ve::VertexProcessor::process(
            vertex, model, identity, identity, viewport
        );

        require(nearVector(raster.worldPosition, ve::Vec3{ 0.35f, 0.3f, 0.2f }),
                "Legacy process Model transform mismatch");

        require(nearVector(raster.worldPositionOverW, raster.worldPosition),
                "Legacy process world position/W mismatch");

        require(nearVector(raster.normalOverW, vertex.normal),
                "Existing Normal pipeline altered");

        std::cout << "[PASS] Legacy process computes world position\n";
    }


    void testPerspectiveReconstructionData() {
        const ve::Viewport viewport(800, 600);

        ve::ClipVertex a{};
        ve::ClipVertex b{};
        ve::ClipVertex c{};

        a.position = ve::Vec4{ -0.5f, -0.5f, 0.0f, 1.0f };
        b.position = ve::Vec4{ 0.5f, -0.5f, 0.0f, 2.0f };
        c.position = ve::Vec4{ 0.0f, 0.5f, 0.0f, 4.0f };

        a.worldPosition = ve::Vec3{ 0.0f, 0.0f, 0.0f };
        b.worldPosition = ve::Vec3{ 4.0f, 0.0f, 0.0f };
        c.worldPosition = ve::Vec3{ 0.0f, 8.0f, 0.0f };

        const ve::RasterVertex ra = ve::VertexProcessor::toRasterSpace(a, viewport);
        const ve::RasterVertex rb = ve::VertexProcessor::toRasterSpace(b, viewport);
        const ve::RasterVertex rc = ve::VertexProcessor::toRasterSpace(c, viewport);

        const ve::Vec3 weights{ 0.25f, 0.25f, 0.5f };

        const float denominator = ra.inverseW * weights.x + rb.inverseW * weights.y +
                                  rc.inverseW * weights.z;

        const ve::Vec3 world = (
            ra.worldPositionOverW * weights.x +
            rb.worldPositionOverW * weights.y +
            rc.worldPositionOverW * weights.z
        ) / denominator;

        require(near(denominator, 0.5f),
                "Perspective denominator mismatch");

        require(nearVector(world, ve::Vec3{ 1.0f, 2.0f, 0.0f }),
                "Perspective-correct World Position mismatch");

        std::cout << "[PASS] Perspective reconstruction data\n";
    }


    void testZeroWLegacyHandling() {
        ve::Vertex vertex{};
        vertex.position = ve::Vec3{ 0.25f, 0.5f, -0.1f };
        vertex.normal = ve::Vec3{ 0.0f, 1.0f, 0.0f };

        const ve::Mat4 model = ve::Mat4::translation(
            ve::Vec3{ 0.1f, -0.2f, 0.3f }
        );

        const ve::Mat4 zeroProjection{};
        const ve::Viewport viewport(800, 600);

        const ve::RasterVertex raster = ve::VertexProcessor::process(
            vertex, model, ve::Mat4::identity(), zeroProjection, viewport
        );

        require(near(raster.inverseW, 0.0f),
                "Zero W not handled");

        require(nearVector(raster.worldPosition, ve::Vec3{ 0.35f, 0.3f, 0.2f }),
                "Zero W must retain World Position");

        require(nearVector(raster.worldPositionOverW, ve::Vec3{}),
                "Zero W must not divide World Position");

        std::cout << "[PASS] Legacy zero-W handling\n";
    }
}


int main() {
    try {
        testClipToRasterAttributes();
        testLegacyProcessWorldPosition();
        testPerspectiveReconstructionData();
        testZeroWLegacyHandling();

        std::cout << "All WorldPositionPipeline tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}