#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>

#include "math/Mat4.h"
#include "math/Vec2.h"
#include "math/Vec3.h"

#include "render/Color.h"
#include "assets/mesh/MeshData.h"
#include "render/api/RenderFrame.h"
#include "assets/mesh/Vertex.h"

#include "geometry/CubeMeshFactory.h"



namespace
{
    constexpr float epsilon = 0.00001f;



    void require( bool condition, const char* message ) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }



    template <typename Callback>
    void requireInvalidArgument( Callback callback, const char* message ) {
        bool caught = false;

        try {
            callback();
        }
        catch (const std::invalid_argument&) {
            caught = true;
        }

        require(caught, message);
    }



    ve::Vec3 subtract( const ve::Vec3& a, const ve::Vec3& b ) {
        return ve::Vec3{ a.x - b.x, a.y - b.y, a.z - b.z };
    }



    void testColor32() {
        const ve::Color32 color = ve::makeColor(17, 34, 51, 68);

        require(color == 0x44112233u, "Color32 bit layout is incorrect");

        require(ve::getRed(color) == 17, "Red channel is incorrect");
        require(ve::getGreen(color) == 34, "Green channel is incorrect");
        require(ve::getBlue(color) == 51, "Blue channel is incorrect");
        require(ve::getAlpha(color) == 68, "Alpha channel is incorrect");

        require(
            ve::makeColor(255, 255, 255) == 0xFFFFFFFFu,
            "Default alpha must be 255"
        );

        std::cout << "[PASS] Color32 layout and channels\n";
    }



    void testCubeCountsAndIndices() {
        const ve::MeshData cube = ve::CubeMeshFactory::create(1.0f);

        require(cube.vertexCount() == 24, "Cube must contain 24 vertices");
        require(cube.indexCount() == 36, "Cube must contain 36 indices");
        require(cube.triangleCount() == 12, "Cube must contain 12 triangles");


        const auto& indices = cube.indices();

        for (std::uint32_t index : indices) {
            require(
                static_cast<std::size_t>(index) < cube.vertexCount(),
                "Cube contains an out-of-range index"
            );
        }


        for (std::uint32_t face = 0; face < 6; ++face) {
            const std::uint32_t baseVertex = face * 4;
            const std::size_t baseIndex = static_cast<std::size_t>(face) * 6;

            require(indices[baseIndex + 0] == baseVertex + 0, "Invalid first triangle index");
            require(indices[baseIndex + 1] == baseVertex + 1, "Invalid first triangle index");
            require(indices[baseIndex + 2] == baseVertex + 2, "Invalid first triangle index");

            require(indices[baseIndex + 3] == baseVertex + 0, "Invalid second triangle index");
            require(indices[baseIndex + 4] == baseVertex + 2, "Invalid second triangle index");
            require(indices[baseIndex + 5] == baseVertex + 3, "Invalid second triangle index");
        }


        std::cout << "[PASS] Cube vertex and index counts\n";
    }



    void testCubeFaceWinding() {
        const ve::MeshData cube = ve::CubeMeshFactory::create(1.0f);

        const auto& vertices = cube.vertices();


        // Expected outward normals in CubeMeshFactory face order

        const std::array<ve::Vec3, 6> expectedNormals{
            ve::Vec3{  0.0f,  0.0f,  1.0f },  // Front
            ve::Vec3{  0.0f,  0.0f, -1.0f },  // Back
            ve::Vec3{ -1.0f,  0.0f,  0.0f },  // Left
            ve::Vec3{  1.0f,  0.0f,  0.0f },  // Right
            ve::Vec3{  0.0f,  1.0f,  0.0f },  // Top
            ve::Vec3{  0.0f, -1.0f,  0.0f }   // Bottom
        };

        for (std::size_t face = 0; face < 6; ++face) {
            const std::size_t base = face * 4;

            const ve::Vec3& a = vertices[base + 0].position;
            const ve::Vec3& b = vertices[base + 1].position;
            const ve::Vec3& c = vertices[base + 2].position;

            const ve::Vec3 edge1 = subtract(b, a);
            const ve::Vec3 edge2 = subtract(c, a);

            const ve::Vec3 normal = ve::cross(edge1, edge2);

            const float outward = ve::dot(normal, expectedNormals[face]);

            require(
                std::fabs(outward - 4.0f) < epsilon,
                "Cube face winding or outward normal is incorrect"
            );
        }


        std::cout << "[PASS] Cube outward-facing CCW winding\n";
    }



    void testCubeUV() {
        const ve::MeshData cube = ve::CubeMeshFactory::create(1.0f);

        const std::array<ve::Vec2, 4> expectedUV{
            ve::Vec2{ 0.0f, 0.0f },
            ve::Vec2{ 1.0f, 0.0f },
            ve::Vec2{ 1.0f, 1.0f },
            ve::Vec2{ 0.0f, 1.0f }
        };


        for (std::size_t face = 0; face < 6; ++face) {
            for (std::size_t corner = 0; corner < 4; ++corner) {
                const ve::Vec2& actual = cube.vertices()[face * 4 + corner].uv;
                const ve::Vec2& expected = expectedUV[corner];

                require(
                    std::fabs(actual.x - expected.x) < epsilon &&
                    std::fabs(actual.y - expected.y) < epsilon,
                    "Cube UV coordinates are incorrect"
                );
            }
        }


        std::cout << "[PASS] Cube UV coordinates\n";
    }



    void testCubeSize() {
        constexpr std::array<float, 3> halfExtents{ 0.5f, 1.0f, 2.0f };


        for (float halfExtent : halfExtents) {
            const ve::MeshData cube = ve::CubeMeshFactory::create(halfExtent);


            for (const ve::Vertex& vertex : cube.vertices()) {
                const ve::Vec3& p = vertex.position;

                require(
                    std::fabs(std::fabs(p.x) - halfExtent) < epsilon,
                    "Cube X extent is incorrect"
                );

                require(
                    std::fabs(std::fabs(p.y) - halfExtent) < epsilon,
                    "Cube Y extent is incorrect"
                );

                require(
                    std::fabs(std::fabs(p.z) - halfExtent) < epsilon,
                    "Cube Z extent is incorrect"
                );
            }
        }


        std::cout << "[PASS] Cube size generation\n";
    }



    void testInvalidMeshData() {
        requireInvalidArgument(
            [] {
                const ve::MeshData invalid{
                    std::vector<ve::Vertex>{},
                    std::vector<std::uint32_t>{ 0, 1 }
                };
                (void)invalid;
            },
            "MeshData must reject non-triangle index counts"
        );


        requireInvalidArgument(
            [] {
                const ve::MeshData invalid{
                    std::vector<ve::Vertex>(1),
                    std::vector<std::uint32_t>{ 0, 1, 0 }
                };
                (void)invalid;
            },
            "MeshData must reject out-of-range indices"
        );


        std::cout << "[PASS] MeshData validation\n";
    }



    void testInvalidCubeSize() {
        requireInvalidArgument(
            [] { (void)ve::CubeMeshFactory::create(0.0f); },
            "Cube must reject zero extent"
        );

        requireInvalidArgument(
            [] { (void)ve::CubeMeshFactory::create(-1.0f); },
            "Cube must reject negative extent"
        );

        requireInvalidArgument(
            [] {
                const float infinity = std::numeric_limits<float>::infinity();
                (void)ve::CubeMeshFactory::create(infinity);
            },
            "Cube must reject infinite extent"
        );


        std::cout << "[PASS] Cube size validation\n";
    }



    void testSharedRenderFrame() {
        const ve::MeshData cube = ve::CubeMeshFactory::create(1.0f);


        ve::RenderFrame frame{};

        const ve::Mat4 leftModel = ve::Mat4::translation(
            ve::Vec3{ -1.4f, 0.0f, -6.0f }
        );

        const ve::Mat4 rightModel = ve::Mat4::translation(
            ve::Vec3{ 1.4f, 0.0f, -6.0f }
        );


        frame.draws.push_back(ve::RenderDraw{ &cube, leftModel });
        frame.draws.push_back(ve::RenderDraw{ &cube, rightModel });


        require(frame.draws.size() == 2, "RenderFrame must contain two draws");

        require(
            frame.draws[0].mesh == &cube &&
            frame.draws[1].mesh == &cube,
            "Draws must share the same MeshData"
        );


        require(
            std::fabs(frame.draws[0].model(0, 3) + 1.4f) < epsilon &&
            std::fabs(frame.draws[1].model(0, 3) - 1.4f) < epsilon,
            "Draws must preserve independent Model matrices"
        );


        std::cout << "[PASS] Shared MeshData and independent Draw models\n";
    }
}



int main() {
    try {
        testColor32();
        testCubeCountsAndIndices();
        testCubeFaceWinding();
        testCubeUV();
        testCubeSize();
        testInvalidMeshData();
        testInvalidCubeSize();
        testSharedRenderFrame();


        std::cout << "\n";
        std::cout << "All renderer architecture tests passed.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}