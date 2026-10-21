#include <cstddef>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>

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



    bool containsPosition( const std::vector<ve::ChunkPosition>& positions,
                           const ve::ChunkPosition& expected ) {
        for (const ve::ChunkPosition& position : positions) {
            if (position.x == expected.x &&
                position.y == expected.y &&
                position.z == expected.z) {
                return true;
            }
        }


        return false;
    }



    void requireDirtyPosition( const std::vector<ve::ChunkPosition>& positions,
                               const ve::ChunkPosition& expected ) {
        require(
            containsPosition(positions, expected),
            "Expected Chunk was not marked Dirty"
        );
    }



    void testNewChunkDirty() {
        ve::VoxelStorage world{};


        world.getOrCreateChunk(ve::ChunkPosition{ 0, 0, 0 });


        require(world.dirtyChunkCount() == 1, "New Chunk must be Dirty");


        const auto dirty = world.consumeDirtyChunks();


        require(dirty.size() == 1, "One new Chunk must produce one Dirty entry");
        requireDirtyPosition(dirty, ve::ChunkPosition{ 0, 0, 0 });

        require(world.dirtyChunkCount() == 0, "Consuming Dirty Chunks must clear the set");


        std::cout << "[PASS] New Chunk becomes Dirty\n";
    }



    void testInteriorBlockEdit() {
        ve::VoxelStorage world{};


        world.getOrCreateChunk(ve::ChunkPosition{ 0, 0, 0 });
        world.getOrCreateChunk(ve::ChunkPosition{ 1, 0, 0 });

        world.consumeDirtyChunks();



        world.setBlock(
            ve::WorldBlockPosition{ 5, 4, 6 },
            ve::BlockId::Stone
        );


        const auto dirty = world.consumeDirtyChunks();


        require(dirty.size() == 1, "Interior edit must dirty only one Chunk");
        requireDirtyPosition(dirty, ve::ChunkPosition{ 0, 0, 0 });


        std::cout << "[PASS] Interior Block Dirty tracking\n";
    }



    void testPositiveXBoundaryEdit() {
        ve::VoxelStorage world{};


        world.getOrCreateChunk(ve::ChunkPosition{ 0, 0, 0 });
        world.getOrCreateChunk(ve::ChunkPosition{ 1, 0, 0 });

        world.consumeDirtyChunks();



        world.setBlock(
            ve::WorldBlockPosition{ 15, 3, 5 },
            ve::BlockId::Stone
        );


        const auto dirty = world.consumeDirtyChunks();


        require(dirty.size() == 2, "X boundary edit must dirty two Chunks");

        requireDirtyPosition(dirty, ve::ChunkPosition{ 0, 0, 0 });
        requireDirtyPosition(dirty, ve::ChunkPosition{ 1, 0, 0 });


        std::cout << "[PASS] Positive X boundary Dirty tracking\n";
    }



    void testNegativeXBoundaryEdit() {
        ve::VoxelStorage world{};


        world.getOrCreateChunk(ve::ChunkPosition{ -1, 0, 0 });
        world.getOrCreateChunk(ve::ChunkPosition{ 0, 0, 0 });

        world.consumeDirtyChunks();



        world.setBlock(
            ve::WorldBlockPosition{ -1, 3, 5 },
            ve::BlockId::Grass
        );


        const auto dirty = world.consumeDirtyChunks();


        require(dirty.size() == 2, "Negative X boundary edit must dirty two Chunks");

        requireDirtyPosition(dirty, ve::ChunkPosition{ -1, 0, 0 });
        requireDirtyPosition(dirty, ve::ChunkPosition{ 0, 0, 0 });


        std::cout << "[PASS] Negative X boundary Dirty tracking\n";
    }



    void testYAndZBoundaryEdit() {
        ve::VoxelStorage world{};


        world.getOrCreateChunk(ve::ChunkPosition{ 0, 0, 0 });
        world.getOrCreateChunk(ve::ChunkPosition{ 0, -1, 0 });
        world.getOrCreateChunk(ve::ChunkPosition{ 0, 0, 1 });

        world.consumeDirtyChunks();



        world.setBlock(
            ve::WorldBlockPosition{ 5, 0, 15 },
            ve::BlockId::Stone
        );


        const auto dirty = world.consumeDirtyChunks();


        require(dirty.size() == 3, "Y/Z corner edit must dirty three Chunks");

        requireDirtyPosition(dirty, ve::ChunkPosition{ 0, 0, 0 });
        requireDirtyPosition(dirty, ve::ChunkPosition{ 0, -1, 0 });
        requireDirtyPosition(dirty, ve::ChunkPosition{ 0, 0, 1 });


        std::cout << "[PASS] Y and Z boundary Dirty tracking\n";
    }



    void testUnchangedBlock() {
        ve::VoxelStorage world{};


        const ve::WorldBlockPosition position{ 3, 4, 5 };


        world.setBlock(position, ve::BlockId::Stone);
        world.consumeDirtyChunks();



        world.setBlock(position, ve::BlockId::Stone);


        require(world.dirtyChunkCount() == 0, "Unchanged Block must not become Dirty");


        std::cout << "[PASS] Unchanged Block avoids unnecessary rebuild\n";
    }



    void testDirtyDeduplication() {
        ve::VoxelStorage world{};


        world.getOrCreateChunk(ve::ChunkPosition{ 0, 0, 0 });
        world.consumeDirtyChunks();



        for (int x = 1; x <= 10; ++x) {
            world.setBlock(
                ve::WorldBlockPosition{ x, 4, 4 },
                ve::BlockId::Stone
            );
        }


        const auto dirty = world.consumeDirtyChunks();


        require(dirty.size() == 1, "Multiple edits must not duplicate Dirty Chunk entries");


        std::cout << "[PASS] Multiple Block edits are deduplicated\n";
    }



    void testNewNeighborInvalidation() {
        ve::VoxelStorage world{};


        world.setBlock(
            ve::WorldBlockPosition{ 15, 5, 5 },
            ve::BlockId::Stone
        );

        world.consumeDirtyChunks();



        // Creating an Adjacent Chunk Must Invalidate Existing Boundary Meshes

        world.getOrCreateChunk(ve::ChunkPosition{ 1, 0, 0 });


        const auto dirty = world.consumeDirtyChunks();


        require(dirty.size() == 2, "New neighbor must dirty both Chunks");

        requireDirtyPosition(dirty, ve::ChunkPosition{ 0, 0, 0 });
        requireDirtyPosition(dirty, ve::ChunkPosition{ 1, 0, 0 });


        std::cout << "[PASS] Newly created Neighbor invalidates boundary Mesh\n";
    }



    void testManualChunkInvalidation() {
        ve::VoxelStorage world{};


        world.getOrCreateChunk(ve::ChunkPosition{ -1, 0, 0 });
        world.getOrCreateChunk(ve::ChunkPosition{ 0, 0, 0 });

        world.consumeDirtyChunks();



        ve::Chunk& chunk = world.getOrCreateChunk(ve::ChunkPosition{ 0, 0, 0 });

        chunk.fill(ve::BlockId::Stone);


        world.markChunkDirty(ve::ChunkPosition{ 0, 0, 0 });


        const auto dirty = world.consumeDirtyChunks();


        require(dirty.size() == 2, "Manual invalidation must include existing neighbors");

        requireDirtyPosition(dirty, ve::ChunkPosition{ -1, 0, 0 });
        requireDirtyPosition(dirty, ve::ChunkPosition{ 0, 0, 0 });


        std::cout << "[PASS] Manual Chunk invalidation\n";
    }



    void testBoundaryMeshUpdate() {
        ve::VoxelStorage world{};


        const ve::WorldBlockPosition leftBlock{ -1, 5, 5 };
        const ve::WorldBlockPosition rightBlock{ 0, 5, 5 };


        world.setBlock(leftBlock, ve::BlockId::Stone);
        world.setBlock(rightBlock, ve::BlockId::Stone);

        world.consumeDirtyChunks();



        const ve::ChunkPosition leftPosition{ -1, 0, 0 };
        const ve::ChunkPosition rightPosition{ 0, 0, 0 };


        const ve::MeshData initialLeft =
            ve::ChunkMeshBuilder::create(world, leftPosition);

        const ve::MeshData initialRight =
            ve::ChunkMeshBuilder::create(world, rightPosition);



        // Both Blocks Are Solid, So Each Has Five Exposed Faces

        require(initialLeft.vertexCount() == 20, "Initial left Block must have five faces");
        require(initialRight.vertexCount() == 20, "Initial right Block must have five faces");



        // Remove the Right Block

        world.setBlock(rightBlock, ve::BlockId::Air);


        const auto dirty = world.consumeDirtyChunks();


        require(dirty.size() == 2, "Deleting a boundary Block must dirty both Chunks");

        requireDirtyPosition(dirty, leftPosition);
        requireDirtyPosition(dirty, rightPosition);



        // Regenerate Both Meshes

        const ve::MeshData updatedLeft =
            ve::ChunkMeshBuilder::create(world, leftPosition);

        const ve::MeshData updatedRight =
            ve::ChunkMeshBuilder::create(world, rightPosition);



        // The Left Block Regains Its Right Face, While the Right Mesh Is Empty

        require(updatedLeft.vertexCount() == 24, "Left Block must regain its exposed face");
        require(updatedRight.vertexCount() == 0, "Removed Block must produce an empty Mesh");


        std::cout << "[PASS] Boundary Block deletion regenerates correct Meshes\n";
    }
}



int main() {
    try {
        testNewChunkDirty();
        testInteriorBlockEdit();
        testPositiveXBoundaryEdit();
        testNegativeXBoundaryEdit();
        testYAndZBoundaryEdit();
        testUnchangedBlock();
        testDirtyDeduplication();
        testNewNeighborInvalidation();
        testManualChunkInvalidation();
        testBoundaryMeshUpdate();


        std::cout << "\nAll VoxelStorage Dirty tests passed.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}