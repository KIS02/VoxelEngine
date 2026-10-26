#include <array>
#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>

#include "assets/geometry/primitives/CubeMeshGenerator.h"
#include "assets/geometry/primitives/CylinderMeshGenerator.h"
#include "render/backends/software/ClipSpaceClipper.h"
#include "render/backends/software/VertexProcessor.h"
#include "voxel/chunk/ChunkMeshBuilder.h"
#include "voxel/chunk/Chunk.h"


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


    void verifySixFaces( const ve::MeshData& mesh ) {
        const std::array<ve::Vec3, 6> expected{
            ve::Vec3{ 0.0f, 0.0f, 1.0f },
            ve::Vec3{ 0.0f, 0.0f, -1.0f },
            ve::Vec3{ -1.0f, 0.0f, 0.0f },
            ve::Vec3{ 1.0f, 0.0f, 0.0f },
            ve::Vec3{ 0.0f, 1.0f, 0.0f },
            ve::Vec3{ 0.0f, -1.0f, 0.0f }
        };

        require(mesh.vertexCount() == 24, "One exposed cube must have 24 vertices");
        require(mesh.indexCount() == 36, "One exposed cube must have 36 indices");

        for (std::size_t face = 0; face < expected.size(); ++face) {
            for (std::size_t corner = 0; corner < 4; ++corner) {
                const ve::Vec3& normal = mesh.vertices()[face * 4 + corner].normal;

                require(nearVector(normal, expected[face]),
                        "Flat face normal must point outward");
            }
        }
    }


    void testCubeNormals() {
        verifySixFaces(ve::CubeMeshGenerator::create(1.0f));

        std::cout << "[PASS] Cube face normals\n";
    }


    void testVoxelNormals() {
        ve::Chunk chunk{};
        chunk.setBlock(3, 3, 3, ve::BlockId::Stone);

        verifySixFaces(ve::ChunkMeshBuilder::create(chunk));

        std::cout << "[PASS] Voxel face normals\n";
    }


    void testCylinderNormals() {
        constexpr int segments = 8;

        const ve::MeshData cylinder = ve::CylinderMeshGenerator::create(1.0f, 1.0f, segments);
        const auto& vertices = cylinder.vertices();

        require(cylinder.vertexCount() == 4 * segments + 4,
                "Cylinder vertex count changed");

        for (int i = 0; i <= segments; ++i) {
            const ve::Vec3& position = vertices[2 * i].position;
            const ve::Vec3 expected{ position.x, 0.0f, position.z };

            require(nearVector(vertices[2 * i].normal, expected),
                    "Cylinder side normal incorrect");

            require(nearVector(vertices[2 * i + 1].normal, expected),
                    "Cylinder upper side normal incorrect");
        }

        const std::size_t bottomCenter = static_cast<std::size_t>(2 * (segments + 1));
        const std::size_t topCenter = bottomCenter + 1;

        require(nearVector(vertices[bottomCenter].normal, ve::Vec3{ 0.0f, -1.0f, 0.0f }),
                "Cylinder lower cap normal incorrect");

        require(nearVector(vertices[topCenter].normal, ve::Vec3{ 0.0f, 1.0f, 0.0f }),
                "Cylinder upper cap normal incorrect");

        for (std::size_t i = topCenter + 1; i < vertices.size(); ++i) {
            const float expectedY = i < topCenter + 1 + segments ? -1.0f : 1.0f;

            require(nearVector(vertices[i].normal, ve::Vec3{ 0.0f, expectedY, 0.0f }),
                    "Cylinder cap perimeter normal incorrect");
        }

        std::cout << "[PASS] Cylinder side and cap normals\n";
    }


    void testSoftwareNormalTransfer() {
        ve::Vertex input{};

        input.position = ve::Vec3{ 0.2f, 0.2f, 0.5f };
        input.normal = ve::Vec3{ 0.0f, 1.0f, 0.0f };

        const ve::Mat4 transform = ve::Mat4::scale(ve::Vec3{ 1.0f, 1.0f, 2.0f });
        const ve::ClipVertex clip = ve::VertexProcessor::toClipSpace(input, transform);

        require(nearVector(clip.normal, input.normal),
                "Clip vertex must retain local normal");

        const ve::RasterVertex raster =
            ve::VertexProcessor::toRasterSpace(clip, ve::Viewport(800, 600));

        require(nearVector(raster.normal, input.normal),
                "Raster vertex must retain local normal");

        require(nearVector(raster.normalOverW, input.normal),
                "Normal over W must use clip W");

        const ve::ClipVertex perspective{
            ve::Vec4{ 0.5f, 0.5f, 0.5f, 2.0f },
            ve::Vec2{},
            ve::makeColor(255, 255, 255),
            input.normal
        };

        const ve::RasterVertex projected =
            ve::VertexProcessor::toRasterSpace(perspective, ve::Viewport(800, 600));

        require(near(projected.inverseW, 0.5f),
                "Inverse W must be 0.5");

        require(nearVector(projected.normalOverW, ve::Vec3{ 0.0f, 0.5f, 0.0f }),
                "Normal over W must be perspective ready");

        std::cout << "[PASS] Software normal attribute transfer\n";
    }


    void testClippedNormalInterpolation() {
        std::array<ve::ClipVertex, 16> output{};

        const ve::ClipVertex a{
            ve::Vec4{ 0.0f, 0.0f, -2.0f, 1.0f },
            ve::Vec2{},
            ve::makeColor(255, 255, 255),
            ve::Vec3{ 1.0f, 0.0f, 0.0f }
        };

        const ve::ClipVertex b{
            ve::Vec4{ -0.5f, -0.5f, 0.0f, 1.0f },
            ve::Vec2{},
            ve::makeColor(255, 255, 255),
            ve::Vec3{ 0.0f, 1.0f, 0.0f }
        };

        const ve::ClipVertex c{
            ve::Vec4{ 0.5f, -0.5f, 0.0f, 1.0f },
            ve::Vec2{},
            ve::makeColor(255, 255, 255),
            ve::Vec3{ 0.0f, 0.0f, 1.0f }
        };

        const std::size_t count = ve::ClipSpaceClipper::clipTriangle(a, b, c, output);

        require(count == 4,
                "Near-plane crossing must produce four vertices");

        bool foundAB = false;
        bool foundAC = false;

        for (std::size_t i = 0; i < count; ++i) {
            if (!near(output[i].position.z, -1.0f))
                continue;

            if (nearVector(output[i].normal, ve::Vec3{ 0.5f, 0.5f, 0.0f }))
                foundAB = true;

            if (nearVector(output[i].normal, ve::Vec3{ 0.5f, 0.0f, 0.5f }))
                foundAC = true;
        }

        require(foundAB && foundAC,
                "Clipping must interpolate normals on both edges");

        std::cout << "[PASS] Clipped normal interpolation\n";
    }
}


int main() {
    try {
        testCubeNormals();
        testVoxelNormals();
        testCylinderNormals();
        testSoftwareNormalTransfer();
        testClippedNormalInterpolation();

        std::cout << "All VertexNormalPipeline tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}