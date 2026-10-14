#include <cstddef>
#include <exception>
#include <iostream>
#include <stdexcept>

#include "demo/ChunkDemoFactory.h"

#include "render/MeshData.h"
#include "render/mesh/ChunkMeshFactory.h"

#include "world/BlockType.h"
#include "world/Chunk.h"
#include "world/ChunkCoordinates.h"
#include "world/World.h"



namespace
{
    void require( bool condition, const char* message ) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }



    void requireFaceCount( const ve::MeshData& mesh, std::size_t count ) {
        require(mesh.vertexCount() == count * 4, "Incorrect face vertex count");
        require(mesh.indexCount() == count * 6, "Incorrect face index count");
        require(mesh.triangleCount() == count * 2, "Incorrect face triangle count");
    }



    void testEmptyWorld() {
        const ve::World world{};


        require(world.chunkCount() == 0, "New World must contain no Chunks");

        require(
            world.findChunk(ve::ChunkPosition{ 0, 0, 0 }) == nullptr,
            "Missing Chunk must return nullptr"
        );

        require(
            world.blockAt(ve::WorldBlockPosition{ -1, 0, 0 }) == ve::BlockType::Air,
            "Missing World block must be Air"
        );

        require(world.chunkCount() == 0, "Reading must not create Chunks");


        std::cout << "[PASS] Empty World and missing Chunk lookup\n";
    }



    void testWorldBlockAccess() {
        ve::World world{};


        world.setBlock(ve::WorldBlockPosition{ -1, 2, 5 }, ve::BlockType::Stone);
        world.setBlock(ve::WorldBlockPosition{ 0, 2, 5 }, ve::BlockType::Grass);


        require(world.chunkCount() == 2, "Blocks must be stored in two Chunks");


        const ve::Chunk* left = world.findChunk(ve::ChunkPosition{ -1, 0, 0 });
        const ve::Chunk* right = world.findChunk(ve::ChunkPosition{ 0, 0, 0 });


        require(left != nullptr, "Negative Chunk must exist");
        require(right != nullptr, "Origin Chunk must exist");


        require(
            left->blockAt(15, 2, 5) == ve::BlockType::Stone,
            "Negative World block must map to Local X=15"
        );

        require(
            right->blockAt(0, 2, 5) == ve::BlockType::Grass,
            "World X=0 must map to Local X=0"
        );


        require(
            world.blockAt(ve::WorldBlockPosition{ -1, 2, 5 }) == ve::BlockType::Stone,
            "World block lookup failed"
        );


        std::cout << "[PASS] Positive and negative World block access\n";
    }



    void testChunkReferenceStability() {
        ve::World world{};


        ve::Chunk& first = world.getOrCreateChunk(ve::ChunkPosition{ 0, 0, 0 });
        ve::Chunk* originalAddress = &first;


        for (int x = -16; x <= 16; ++x) {
            world.getOrCreateChunk(ve::ChunkPosition{ x, 0, 0 });
        }


        ve::Chunk* current = world.findChunk(ve::ChunkPosition{ 0, 0, 0 });


        require(current == originalAddress, "Adding Chunks must preserve Chunk addresses");


        std::cout << "[PASS] Stable Chunk addresses\n";
    }



    void testPositiveXBoundary() {
        ve::World world{};


        world.setBlock(ve::WorldBlockPosition{ 15, 2, 5 }, ve::BlockType::Stone);
        world.setBlock(ve::WorldBlockPosition{ 16, 2, 5 }, ve::BlockType::Stone);


        const ve::MeshData leftMesh =
            ve::ChunkMeshFactory::create(world, ve::ChunkPosition{ 0, 0, 0 });

        const ve::MeshData rightMesh =
            ve::ChunkMeshFactory::create(world, ve::ChunkPosition{ 1, 0, 0 });


        requireFaceCount(leftMesh, 5);
        requireFaceCount(rightMesh, 5);


        std::cout << "[PASS] Positive X Chunk boundary face removal\n";
    }



    void testNegativeXBoundary() {
        ve::World world{};


        world.setBlock(ve::WorldBlockPosition{ -1, 2, 5 }, ve::BlockType::Stone);
        world.setBlock(ve::WorldBlockPosition{ 0, 2, 5 }, ve::BlockType::Stone);


        const ve::MeshData leftMesh =
            ve::ChunkMeshFactory::create(world, ve::ChunkPosition{ -1, 0, 0 });

        const ve::MeshData rightMesh =
            ve::ChunkMeshFactory::create(world, ve::ChunkPosition{ 0, 0, 0 });


        requireFaceCount(leftMesh, 5);
        requireFaceCount(rightMesh, 5);


        std::cout << "[PASS] Negative X Chunk boundary face removal\n";
    }



    void testYBoundary() {
        ve::World world{};


        world.setBlock(ve::WorldBlockPosition{ 3, 15, 5 }, ve::BlockType::Stone);
        world.setBlock(ve::WorldBlockPosition{ 3, 16, 5 }, ve::BlockType::Stone);


        const ve::MeshData lowerMesh =
            ve::ChunkMeshFactory::create(world, ve::ChunkPosition{ 0, 0, 0 });

        const ve::MeshData upperMesh =
            ve::ChunkMeshFactory::create(world, ve::ChunkPosition{ 0, 1, 0 });


        requireFaceCount(lowerMesh, 5);
        requireFaceCount(upperMesh, 5);


        std::cout << "[PASS] Y Chunk boundary face removal\n";
    }



    void testZBoundary() {
        ve::World world{};


        world.setBlock(ve::WorldBlockPosition{ 3, 2, -1 }, ve::BlockType::Stone);
        world.setBlock(ve::WorldBlockPosition{ 3, 2, 0 }, ve::BlockType::Stone);


        const ve::MeshData backMesh =
            ve::ChunkMeshFactory::create(world, ve::ChunkPosition{ 0, 0, -1 });

        const ve::MeshData frontMesh =
            ve::ChunkMeshFactory::create(world, ve::ChunkPosition{ 0, 0, 0 });


        requireFaceCount(backMesh, 5);
        requireFaceCount(frontMesh, 5);


        std::cout << "[PASS] Z Chunk boundary face removal\n";
    }



    void testMissingNeighbor() {
        ve::World world{};


        world.setBlock(ve::WorldBlockPosition{ 15, 2, 5 }, ve::BlockType::Stone);


        const ve::MeshData mesh =
            ve::ChunkMeshFactory::create(world, ve::ChunkPosition{ 0, 0, 0 });


        requireFaceCount(mesh, 6);


        std::cout << "[PASS] Missing neighbor Chunk exposes boundary face\n";
    }



    void testLegacyChunkMeshing() {
        ve::World world{};


        world.setBlock(ve::WorldBlockPosition{ 15, 2, 5 }, ve::BlockType::Stone);
        world.setBlock(ve::WorldBlockPosition{ 16, 2, 5 }, ve::BlockType::Stone);


        const ve::Chunk* chunk = world.findChunk(ve::ChunkPosition{ 0, 0, 0 });

        require(chunk != nullptr, "Test Chunk must exist");


        const ve::MeshData isolated = ve::ChunkMeshFactory::create(*chunk);
        const ve::MeshData connected =
            ve::ChunkMeshFactory::create(world, ve::ChunkPosition{ 0, 0, 0 });


        requireFaceCount(isolated, 6);
        requireFaceCount(connected, 5);


        std::cout << "[PASS] Legacy and World-aware meshing compatibility\n";
    }



    void testFourChunkWorld() {
        ve::World world{};


        for (int z = -1; z <= 0; ++z) {
            for (int x = -1; x <= 0; ++x) {
                const ve::ChunkPosition position{ x, 0, z };

                world.getOrCreateChunk(position) = ve::createDemoChunk();
            }
        }


        require(world.chunkCount() == 4, "Demo World must contain four Chunks");


        std::size_t isolatedFaces = 0;
        std::size_t connectedFaces = 0;


        for (const auto& [position, chunk] : world.chunks()) {
            const ve::MeshData isolated = ve::ChunkMeshFactory::create(chunk);
            const ve::MeshData connected = ve::ChunkMeshFactory::create(world, position);


            isolatedFaces += isolated.vertexCount() / 4;
            connectedFaces += connected.vertexCount() / 4;
        }


        require(
            connectedFaces < isolatedFaces,
            "Connected World must remove some boundary faces"
        );


        std::cout << "[PASS] Four-Chunk World boundary optimization\n";

        std::cout << "[INFO] Isolated Faces: " << isolatedFaces << '\n';
        std::cout << "[INFO] Connected Faces: " << connectedFaces << '\n';
        std::cout << "[INFO] Removed Faces: " << isolatedFaces - connectedFaces << '\n';
    }



    void testMissingTargetChunk() {
        const ve::World world{};

        bool caught = false;


        try {
            (void)ve::ChunkMeshFactory::create(
                world,
                ve::ChunkPosition{ 0, 0, 0 }
            );
        }
        catch (const std::invalid_argument&) {
            caught = true;
        }


        require(caught, "Meshing a missing Chunk must throw");


        std::cout << "[PASS] Missing target Chunk validation\n";
    }
}



int main() {
    try {
        testEmptyWorld();
        testWorldBlockAccess();
        testChunkReferenceStability();
        testPositiveXBoundary();
        testNegativeXBoundary();
        testYBoundary();
        testZBoundary();
        testMissingNeighbor();
        testLegacyChunkMeshing();
        testFourChunkWorld();
        testMissingTargetChunk();


        std::cout << "\nAll World tests passed.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}