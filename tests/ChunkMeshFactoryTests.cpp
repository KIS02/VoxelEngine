#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>

#include "math/Vec3.h"

#include "render/Color.h"
#include "assets/mesh/MeshData.h"
#include "assets/mesh/Vertex.h"
#include "world/meshing/ChunkMeshFactory.h"

#include "world/BlockType.h"
#include "world/Chunk.h"



namespace
{
    constexpr float epsilon = 0.00001f;



    void require( bool condition, const char* message ) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }



    void requireFaceCount( const ve::MeshData& mesh, std::size_t faceCount ) {
        require(mesh.vertexCount() == faceCount * 4, "Incorrect Mesh vertex count");
        require(mesh.indexCount() == faceCount * 6, "Incorrect Mesh index count");
        require(mesh.triangleCount() == faceCount * 2, "Incorrect Mesh triangle count");
    }



    void testEmptyChunk() {
        const ve::Chunk chunk{};
        const ve::MeshData mesh = ve::ChunkMeshFactory::create(chunk);


        requireFaceCount(mesh, 0);

        require(mesh.vertices().empty(), "Empty Chunk must have no vertices");
        require(mesh.indices().empty(), "Empty Chunk must have no indices");


        std::cout << "[PASS] Empty Chunk Mesh\n";
    }



    void testSingleBlock() {
        ve::Chunk chunk{};

        chunk.setBlock(3, 2, 5, ve::BlockType::Stone);


        const ve::MeshData mesh = ve::ChunkMeshFactory::create(chunk);


        requireFaceCount(mesh, 6);



        // Block occupies [3,4] x [2,3] x [5,6]

        for (const ve::Vertex& vertex : mesh.vertices()) {
            const ve::Vec3& p = vertex.position;

            require(p.x >= 3.0f && p.x <= 4.0f, "Single block X position is incorrect");
            require(p.y >= 2.0f && p.y <= 3.0f, "Single block Y position is incorrect");
            require(p.z >= 5.0f && p.z <= 6.0f, "Single block Z position is incorrect");
        }


        std::cout << "[PASS] Single Block Mesh\n";
    }



    void testFaceWinding() {
        ve::Chunk chunk{};

        chunk.setBlock(3, 2, 5, ve::BlockType::Stone);


        const ve::MeshData mesh = ve::ChunkMeshFactory::create(chunk);


        const std::array<ve::Vec3, 6> expectedNormals{
            ve::Vec3{  0.0f,  0.0f,  1.0f },
            ve::Vec3{  0.0f,  0.0f, -1.0f },
            ve::Vec3{ -1.0f,  0.0f,  0.0f },
            ve::Vec3{  1.0f,  0.0f,  0.0f },
            ve::Vec3{  0.0f,  1.0f,  0.0f },
            ve::Vec3{  0.0f, -1.0f,  0.0f }
        };



        for (std::size_t face = 0; face < 6; ++face) {
            const std::size_t base = face * 4;

            const ve::Vec3& a = mesh.vertices()[base + 0].position;
            const ve::Vec3& b = mesh.vertices()[base + 1].position;
            const ve::Vec3& c = mesh.vertices()[base + 2].position;


            const ve::Vec3 edge1{
                b.x - a.x,
                b.y - a.y,
                b.z - a.z
            };

            const ve::Vec3 edge2{
                c.x - a.x,
                c.y - a.y,
                c.z - a.z
            };


            const ve::Vec3 normal = ve::cross(edge1, edge2);

            const float outward = ve::dot(normal, expectedNormals[face]);


            require(
                std::fabs(outward - 1.0f) < epsilon,
                "Chunk Mesh face winding is incorrect"
            );
        }


        std::cout << "[PASS] Chunk Mesh outward-facing CCW winding\n";
    }



    void testAdjacentBlocks() {
        ve::Chunk chunk{};

        chunk.setBlock(1, 1, 1, ve::BlockType::Stone);
        chunk.setBlock(2, 1, 1, ve::BlockType::Stone);


        const ve::MeshData mesh = ve::ChunkMeshFactory::create(chunk);


        // 12 total faces - 2 touching faces = 10 faces

        requireFaceCount(mesh, 10);


        std::cout << "[PASS] Adjacent Block internal face removal\n";
    }



    void testSeparatedBlocks() {
        ve::Chunk chunk{};

        chunk.setBlock(1, 1, 1, ve::BlockType::Stone);
        chunk.setBlock(3, 1, 1, ve::BlockType::Stone);


        const ve::MeshData mesh = ve::ChunkMeshFactory::create(chunk);


        // No shared faces: 6 + 6 = 12 faces

        requireFaceCount(mesh, 12);


        std::cout << "[PASS] Separated Blocks preserve all faces\n";
    }



    void testTwoByTwoByTwo() {
        ve::Chunk chunk{};


        for (int y = 0; y < 2; ++y) {
            for (int z = 0; z < 2; ++z) {
                for (int x = 0; x < 2; ++x) {
                    chunk.setBlock(x, y, z, ve::BlockType::Dirt);
                }
            }
        }


        const ve::MeshData mesh = ve::ChunkMeshFactory::create(chunk);


        // Six outer sides, four unit faces per side

        requireFaceCount(mesh, 24);


        std::cout << "[PASS] 2x2x2 Solid Block Region\n";
    }



    void testFullChunk() {
        ve::Chunk chunk{};

        chunk.fill(ve::BlockType::Stone);


        const ve::MeshData mesh = ve::ChunkMeshFactory::create(chunk);


        constexpr std::size_t expectedFaces =
            6 * ve::Chunk::Size * ve::Chunk::Size;


        requireFaceCount(mesh, expectedFaces);



        // Every Index Must Refer to an Existing Vertex

        for (std::uint32_t index : mesh.indices()) {
            require(
                static_cast<std::size_t>(index) < mesh.vertexCount(),
                "Full Chunk contains an invalid Mesh index"
            );
        }



        // 4096 Blocks, But Only the Outer Surface is Generated

        require(chunk.solidBlockCount() == 4096, "Chunk must contain 4096 solid blocks");


        std::cout << "[PASS] Full 16x16x16 Chunk surface generation\n";
    }



    void testChunkBoundary() {
        ve::Chunk chunk{};

        chunk.setBlock(15, 7, 7, ve::BlockType::Grass);


        const ve::MeshData mesh = ve::ChunkMeshFactory::create(chunk);


        requireFaceCount(mesh, 6);


        bool foundRightBoundary = false;


        for (const ve::Vertex& vertex : mesh.vertices()) {
            if (std::fabs(vertex.position.x - 16.0f) < epsilon) {
                foundRightBoundary = true;
                break;
            }
        }


        require(foundRightBoundary, "Chunk boundary must extend to local X=16");


        std::cout << "[PASS] Chunk boundary face generation\n";
    }



    void testBlockColors() {
        constexpr std::array<ve::BlockType, 3> types{
            ve::BlockType::Grass,
            ve::BlockType::Dirt,
            ve::BlockType::Stone
        };


        constexpr std::array<ve::Color32, 3> expectedColors{
            ve::makeColor(80, 180, 70),
            ve::makeColor(135, 90, 50),
            ve::makeColor(150, 150, 150)
        };


        for (std::size_t i = 0; i < types.size(); ++i) {
            ve::Chunk chunk{};

            chunk.setBlock(1, 1, 1, types[i]);


            const ve::MeshData mesh = ve::ChunkMeshFactory::create(chunk);


            requireFaceCount(mesh, 6);


            for (const ve::Vertex& vertex : mesh.vertices()) {
                require(vertex.color == expectedColors[i], "Block vertex color is incorrect");
            }
        }


        std::cout << "[PASS] Block type vertex colors\n";
    }



    void testSourceChunkPreserved() {
        ve::Chunk chunk{};

        chunk.setBlock(4, 5, 6, ve::BlockType::Grass);


        const ve::MeshData mesh = ve::ChunkMeshFactory::create(chunk);


        requireFaceCount(mesh, 6);

        require(
            chunk.blockAt(4, 5, 6) == ve::BlockType::Grass,
            "Mesh generation must not modify source Chunk"
        );

        require(
            chunk.solidBlockCount() == 1,
            "Mesh generation must preserve the number of solid blocks"
        );


        std::cout << "[PASS] Source Chunk remains unchanged\n";
    }
}



int main() {
    try {
        testEmptyChunk();
        testSingleBlock();
        testFaceWinding();
        testAdjacentBlocks();
        testSeparatedBlocks();
        testTwoByTwoByTwo();
        testFullChunk();
        testChunkBoundary();
        testBlockColors();
        testSourceChunkPreserved();


        std::cout << "\nAll Chunk Mesh Factory tests passed.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}