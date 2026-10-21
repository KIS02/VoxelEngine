#include <cstddef>
#include <exception>
#include <iostream>
#include <stdexcept>

#include "scene/demo/DemoChunkGenerator.h"

#include "assets/mesh/MeshData.h"
#include "voxel/chunk/ChunkMeshBuilder.h"

#include "voxel/block/BlockId.h"
#include "voxel/chunk/Chunk.h"
#include "voxel/chunk/ChunkCoordinates.h"
#include "voxel/chunk/VoxelStorage.h"



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
        const ve::VoxelStorage world{};


        require(world.chunkCount() == 0, "New VoxelStorage must contain no Chunks");

        require(
            world.findChunk(ve::ChunkPosition{ 0, 0, 0 }) == nullptr,
            "Missing Chunk must return nullptr"
        );

        require(
            world.blockAt(ve::WorldBlockPosition{ -1, 0, 0 }) == ve::BlockId::Air,
            "Missing VoxelStorage block must be Air"
        );

        require(world.chunkCount() == 0, "Reading must not create Chunks");


        std::cout << "[PASS] Empty VoxelStorage and missing Chunk lookup\n";
    }



    void testWorldBlockAccess() {
        ve::VoxelStorage world{};


        world.setBlock(ve::WorldBlockPosition{ -1, 2, 5 }, ve::BlockId::Stone);
        world.setBlock(ve::WorldBlockPosition{ 0, 2, 5 }, ve::BlockId::Grass);


        require(world.chunkCount() == 2, "Blocks must be stored in two Chunks");


        const ve::Chunk* left = world.findChunk(ve::ChunkPosition{ -1, 0, 0 });
        const ve::Chunk* right = world.findChunk(ve::ChunkPosition{ 0, 0, 0 });


        require(left != nullptr, "Negative Chunk must exist");
        require(right != nullptr, "Origin Chunk must exist");


        require(
            left->blockAt(15, 2, 5) == ve::BlockId::Stone,
            "Negative VoxelStorage block must map to Local X=15"
        );

        require(
            right->blockAt(0, 2, 5) == ve::BlockId::Grass,
            "VoxelStorage X=0 must map to Local X=0"
        );


        require(
            world.blockAt(ve::WorldBlockPosition{ -1, 2, 5 }) == ve::BlockId::Stone,
            "VoxelStorage block lookup failed"
        );


        std::cout << "[PASS] Positive and negative VoxelStorage block access\n";
    }



    void testChunkReferenceStability() {
        ve::VoxelStorage world{};


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
        ve::VoxelStorage world{};


        world.setBlock(ve::WorldBlockPosition{ 15, 2, 5 }, ve::BlockId::Stone);
        world.setBlock(ve::WorldBlockPosition{ 16, 2, 5 }, ve::BlockId::Stone);


        const ve::MeshData leftMesh =
            ve::ChunkMeshBuilder::create(world, ve::ChunkPosition{ 0, 0, 0 });

        const ve::MeshData rightMesh =
            ve::ChunkMeshBuilder::create(world, ve::ChunkPosition{ 1, 0, 0 });


        requireFaceCount(leftMesh, 5);
        requireFaceCount(rightMesh, 5);


        std::cout << "[PASS] Positive X Chunk boundary face removal\n";
    }



    void testNegativeXBoundary() {
        ve::VoxelStorage world{};


        world.setBlock(ve::WorldBlockPosition{ -1, 2, 5 }, ve::BlockId::Stone);
        world.setBlock(ve::WorldBlockPosition{ 0, 2, 5 }, ve::BlockId::Stone);


        const ve::MeshData leftMesh =
            ve::ChunkMeshBuilder::create(world, ve::ChunkPosition{ -1, 0, 0 });

        const ve::MeshData rightMesh =
            ve::ChunkMeshBuilder::create(world, ve::ChunkPosition{ 0, 0, 0 });


        requireFaceCount(leftMesh, 5);
        requireFaceCount(rightMesh, 5);


        std::cout << "[PASS] Negative X Chunk boundary face removal\n";
    }



    void testYBoundary() {
        ve::VoxelStorage world{};


        world.setBlock(ve::WorldBlockPosition{ 3, 15, 5 }, ve::BlockId::Stone);
        world.setBlock(ve::WorldBlockPosition{ 3, 16, 5 }, ve::BlockId::Stone);


        const ve::MeshData lowerMesh =
            ve::ChunkMeshBuilder::create(world, ve::ChunkPosition{ 0, 0, 0 });

        const ve::MeshData upperMesh =
            ve::ChunkMeshBuilder::create(world, ve::ChunkPosition{ 0, 1, 0 });


        requireFaceCount(lowerMesh, 5);
        requireFaceCount(upperMesh, 5);


        std::cout << "[PASS] Y Chunk boundary face removal\n";
    }



    void testZBoundary() {
        ve::VoxelStorage world{};


        world.setBlock(ve::WorldBlockPosition{ 3, 2, -1 }, ve::BlockId::Stone);
        world.setBlock(ve::WorldBlockPosition{ 3, 2, 0 }, ve::BlockId::Stone);


        const ve::MeshData backMesh =
            ve::ChunkMeshBuilder::create(world, ve::ChunkPosition{ 0, 0, -1 });

        const ve::MeshData frontMesh =
            ve::ChunkMeshBuilder::create(world, ve::ChunkPosition{ 0, 0, 0 });


        requireFaceCount(backMesh, 5);
        requireFaceCount(frontMesh, 5);


        std::cout << "[PASS] Z Chunk boundary face removal\n";
    }



    void testMissingNeighbor() {
        ve::VoxelStorage world{};


        world.setBlock(ve::WorldBlockPosition{ 15, 2, 5 }, ve::BlockId::Stone);


        const ve::MeshData mesh =
            ve::ChunkMeshBuilder::create(world, ve::ChunkPosition{ 0, 0, 0 });


        requireFaceCount(mesh, 6);


        std::cout << "[PASS] Missing neighbor Chunk exposes boundary face\n";
    }



    void testLegacyChunkMeshing() {
        ve::VoxelStorage world{};


        world.setBlock(ve::WorldBlockPosition{ 15, 2, 5 }, ve::BlockId::Stone);
        world.setBlock(ve::WorldBlockPosition{ 16, 2, 5 }, ve::BlockId::Stone);


        const ve::Chunk* chunk = world.findChunk(ve::ChunkPosition{ 0, 0, 0 });

        require(chunk != nullptr, "Test Chunk must exist");


        const ve::MeshData isolated = ve::ChunkMeshBuilder::create(*chunk);
        const ve::MeshData connected =
            ve::ChunkMeshBuilder::create(world, ve::ChunkPosition{ 0, 0, 0 });


        requireFaceCount(isolated, 6);
        requireFaceCount(connected, 5);


        std::cout << "[PASS] Legacy and VoxelStorage-aware meshing compatibility\n";
    }



    void testFourChunkWorld() {
        ve::VoxelStorage world{};


        for (int z = -1; z <= 0; ++z) {
            for (int x = -1; x <= 0; ++x) {
                const ve::ChunkPosition position{ x, 0, z };

                world.getOrCreateChunk(position) = ve::createDemoChunk();
            }
        }


        require(world.chunkCount() == 4, "Demo VoxelStorage must contain four Chunks");


        std::size_t isolatedFaces = 0;
        std::size_t connectedFaces = 0;


        for (const auto& [position, chunk] : world.chunks()) {
            const ve::MeshData isolated = ve::ChunkMeshBuilder::create(chunk);
            const ve::MeshData connected = ve::ChunkMeshBuilder::create(world, position);


            isolatedFaces += isolated.vertexCount() / 4;
            connectedFaces += connected.vertexCount() / 4;
        }


        require(
            connectedFaces < isolatedFaces,
            "Connected VoxelStorage must remove some boundary faces"
        );


        std::cout << "[PASS] Four-Chunk VoxelStorage boundary optimization\n";

        std::cout << "[INFO] Isolated Faces: " << isolatedFaces << '\n';
        std::cout << "[INFO] Connected Faces: " << connectedFaces << '\n';
        std::cout << "[INFO] Removed Faces: " << isolatedFaces - connectedFaces << '\n';
    }



    void testMissingTargetChunk() {
        const ve::VoxelStorage world{};

        bool caught = false;


        try {
            (void)ve::ChunkMeshBuilder::create(
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


        std::cout << "\nAll VoxelStorage tests passed.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}