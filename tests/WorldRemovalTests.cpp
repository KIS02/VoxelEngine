#include <array>
#include <cstddef>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>

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



    bool samePosition( const ve::ChunkPosition& a, const ve::ChunkPosition& b ) {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }



    bool containsPosition( const std::vector<ve::ChunkPosition>& positions,
                           const ve::ChunkPosition& expected ) {
        for (const ve::ChunkPosition& position : positions) {
            if (samePosition(position, expected)) {
                return true;
            }
        }

        return false;
    }



    void requireFaceCount( const ve::MeshData& mesh, std::size_t expected ) {
        require(mesh.vertexCount() == expected * 4, "Incorrect Mesh vertex count");
        require(mesh.indexCount() == expected * 6, "Incorrect Mesh index count");
        require(mesh.triangleCount() == expected * 2, "Incorrect Mesh triangle count");
    }



    void testRemoveExistingChunk() {
        ve::World world{};

        const ve::ChunkPosition target{ 0, 0, 0 };
        const ve::ChunkPosition other{ 2, 0, 0 };


        world.setBlock(
            ve::WorldBlockPosition{ 1, 2, 3 },
            ve::BlockType::Stone
        );

        world.getOrCreateChunk(other);

        world.consumeDirtyChunks();


        require(world.chunkCount() == 2, "Expected two initial Chunks");
        require(world.removeChunk(target), "Existing Chunk must be removable");

        require(world.chunkCount() == 1, "Chunk count must decrease");
        require(world.findChunk(target) == nullptr, "Removed Chunk must not be found");
        require(world.findChunk(other) != nullptr, "Unrelated Chunk must remain");

        require(
            world.blockAt(ve::WorldBlockPosition{ 1, 2, 3 }) == ve::BlockType::Air,
            "Blocks in removed Chunk must no longer be accessible"
        );


        std::cout << "[PASS] Existing Chunk removal\n";
    }



    void testMissingChunkRemoval() {
        ve::World world{};

        const ve::ChunkPosition existing{ 0, 0, 0 };
        const ve::ChunkPosition missing{ 5, 0, 0 };


        world.getOrCreateChunk(existing);
        world.consumeDirtyChunks();


        require(!world.removeChunk(missing), "Removing a missing Chunk must return false");
        require(world.chunkCount() == 1, "Missing removal must not change Chunk count");
        require(world.dirtyChunkCount() == 0, "Missing removal must not create Dirty work");
        require(world.findChunk(existing) != nullptr, "Existing Chunk must remain");


        std::cout << "[PASS] Missing Chunk removal\n";
    }



    void testRemovedChunkDirtyCleanup() {
        ve::World world{};

        const ve::ChunkPosition target{ 0, 0, 0 };


        world.getOrCreateChunk(target);

        require(world.dirtyChunkCount() == 1, "New Chunk must be Dirty");

        require(world.removeChunk(target), "Removal must succeed");
        require(world.dirtyChunkCount() == 0, "Removed Chunk must leave Dirty set");


        int processed = 0;


        world.processDirtyChunks([&]( const ve::ChunkPosition& position ) {
            (void)position;
            ++processed;
        });


        require(processed == 0, "Removed Chunk must not be processed");


        std::cout << "[PASS] Removed Chunk Dirty cleanup\n";
    }



    void testSixNeighborsDirty() {
        ve::World world{};

        const ve::ChunkPosition center{ 0, 0, 0 };

        const std::array<ve::ChunkPosition, 6> neighbors{
            ve::ChunkPosition{ -1, 0, 0 },
            ve::ChunkPosition{  1, 0, 0 },
            ve::ChunkPosition{ 0, -1, 0 },
            ve::ChunkPosition{ 0,  1, 0 },
            ve::ChunkPosition{ 0, 0, -1 },
            ve::ChunkPosition{ 0, 0,  1 }
        };

        const ve::ChunkPosition diagonal{ 1, 0, 1 };


        world.getOrCreateChunk(center);

        for (const ve::ChunkPosition& position : neighbors) {
            world.getOrCreateChunk(position);
        }

        world.getOrCreateChunk(diagonal);

        world.consumeDirtyChunks();



        // Removing Center Must Affect Direct Neighbors Only

        require(world.removeChunk(center), "Center removal must succeed");

        const std::vector<ve::ChunkPosition> dirty = world.consumeDirtyChunks();


        require(dirty.size() == 6, "Exactly six direct neighbors must be Dirty");

        for (const ve::ChunkPosition& position : neighbors) {
            require(containsPosition(dirty, position), "Direct neighbor was not marked Dirty");
        }


        require(!containsPosition(dirty, diagonal), "Diagonal Chunk must not become Dirty");
        require(!containsPosition(dirty, center), "Removed Chunk must not be Dirty");

        require(world.chunkCount() == 7, "Seven Chunks must remain");


        std::cout << "[PASS] Six neighboring Chunks become Dirty\n";
    }



    void testPositiveXBoundaryExposure() {
        ve::World world{};

        const ve::ChunkPosition left{ 0, 0, 0 };
        const ve::ChunkPosition right{ 1, 0, 0 };


        world.setBlock(
            ve::WorldBlockPosition{ 15, 2, 5 },
            ve::BlockType::Stone
        );

        world.setBlock(
            ve::WorldBlockPosition{ 16, 2, 5 },
            ve::BlockType::Stone
        );


        const ve::MeshData initialLeft = ve::ChunkMeshFactory::create(world, left);
        const ve::MeshData initialRight = ve::ChunkMeshFactory::create(world, right);


        requireFaceCount(initialLeft, 5);
        requireFaceCount(initialRight, 5);


        world.consumeDirtyChunks();



        // Remove the Entire Right Chunk

        require(world.removeChunk(right), "Right Chunk removal must succeed");


        const auto dirty = world.consumeDirtyChunks();


        require(dirty.size() == 1, "Only left Chunk must be Dirty");
        require(containsPosition(dirty, left), "Left Chunk must be Dirty");



        // Left Stone Regains Its Right Face

        const ve::MeshData updatedLeft = ve::ChunkMeshFactory::create(world, left);

        requireFaceCount(updatedLeft, 6);


        require(
            world.blockAt(ve::WorldBlockPosition{ 16, 2, 5 }) == ve::BlockType::Air,
            "Removed neighbor must be treated as Air"
        );


        std::cout << "[PASS] Positive X boundary face re-exposure\n";
    }



    void testNegativeXBoundaryExposure() {
        ve::World world{};

        const ve::ChunkPosition left{ -1, 0, 0 };
        const ve::ChunkPosition right{ 0, 0, 0 };


        world.setBlock(
            ve::WorldBlockPosition{ -1, 2, 5 },
            ve::BlockType::Stone
        );

        world.setBlock(
            ve::WorldBlockPosition{ 0, 2, 5 },
            ve::BlockType::Stone
        );


        requireFaceCount(ve::ChunkMeshFactory::create(world, left), 5);
        requireFaceCount(ve::ChunkMeshFactory::create(world, right), 5);


        world.consumeDirtyChunks();

        require(world.removeChunk(right), "Origin Chunk removal must succeed");


        requireFaceCount(ve::ChunkMeshFactory::create(world, left), 6);

        require(world.dirtyChunkCount() == 1, "Negative neighbor must be Dirty");


        std::cout << "[PASS] Negative X boundary face re-exposure\n";
    }



    void testYBoundaryExposure() {
        ve::World world{};

        const ve::ChunkPosition lower{ 0, 0, 0 };
        const ve::ChunkPosition upper{ 0, 1, 0 };


        world.setBlock(
            ve::WorldBlockPosition{ 3, 15, 5 },
            ve::BlockType::Stone
        );

        world.setBlock(
            ve::WorldBlockPosition{ 3, 16, 5 },
            ve::BlockType::Stone
        );


        requireFaceCount(ve::ChunkMeshFactory::create(world, lower), 5);

        world.consumeDirtyChunks();

        require(world.removeChunk(upper), "Upper Chunk removal must succeed");


        requireFaceCount(ve::ChunkMeshFactory::create(world, lower), 6);

        require(world.dirtyChunkCount() == 1, "Lower Chunk must be Dirty");


        std::cout << "[PASS] Y boundary face re-exposure\n";
    }



    void testZBoundaryExposure() {
        ve::World world{};

        const ve::ChunkPosition back{ 0, 0, -1 };
        const ve::ChunkPosition front{ 0, 0, 0 };


        world.setBlock(
            ve::WorldBlockPosition{ 3, 2, -1 },
            ve::BlockType::Stone
        );

        world.setBlock(
            ve::WorldBlockPosition{ 3, 2, 0 },
            ve::BlockType::Stone
        );


        requireFaceCount(ve::ChunkMeshFactory::create(world, back), 5);

        world.consumeDirtyChunks();

        require(world.removeChunk(front), "Front Chunk removal must succeed");


        requireFaceCount(ve::ChunkMeshFactory::create(world, back), 6);

        require(world.dirtyChunkCount() == 1, "Back Chunk must be Dirty");


        std::cout << "[PASS] Z boundary face re-exposure\n";
    }



    void testChunkRecreation() {
        ve::World world{};

        const ve::ChunkPosition left{ 0, 0, 0 };
        const ve::ChunkPosition right{ 1, 0, 0 };


        world.setBlock(
            ve::WorldBlockPosition{ 15, 2, 5 },
            ve::BlockType::Stone
        );

        world.setBlock(
            ve::WorldBlockPosition{ 16, 2, 5 },
            ve::BlockType::Stone
        );


        world.consumeDirtyChunks();



        // Remove the Right Chunk

        require(world.removeChunk(right), "Initial removal must succeed");

        requireFaceCount(ve::ChunkMeshFactory::create(world, left), 6);

        world.consumeDirtyChunks();



        // Recreate the Chunk at the Same Coordinate

        ve::Chunk& recreated = world.getOrCreateChunk(right);


        require(
            recreated.blockAt(0, 2, 5) == ve::BlockType::Air,
            "Recreated Chunk must start empty"
        );

        require(world.chunkCount() == 2, "Two Chunks must exist after recreation");


        const auto dirty = world.consumeDirtyChunks();


        require(dirty.size() == 2, "Recreation must invalidate both neighboring Chunks");

        require(containsPosition(dirty, left), "Left Chunk must be invalidated");
        require(containsPosition(dirty, right), "Recreated Chunk must be invalidated");



        // New Neighbor Is Air, So Left Stone Remains Exposed

        requireFaceCount(ve::ChunkMeshFactory::create(world, left), 6);



        // Placing a New Stone Hides the Boundary Again

        world.setBlock(
            ve::WorldBlockPosition{ 16, 2, 5 },
            ve::BlockType::Stone
        );


        requireFaceCount(ve::ChunkMeshFactory::create(world, left), 5);
        requireFaceCount(ve::ChunkMeshFactory::create(world, right), 5);


        std::cout << "[PASS] Chunk removal and recreation\n";
    }



    void testDistantChunkUnaffected() {
        ve::World world{};

        const ve::ChunkPosition target{ 0, 0, 0 };
        const ve::ChunkPosition distant{ 5, 0, 5 };


        world.getOrCreateChunk(target);
        ve::Chunk& distantChunk = world.getOrCreateChunk(distant);

        ve::Chunk* originalAddress = &distantChunk;


        world.consumeDirtyChunks();


        require(world.removeChunk(target), "Target removal must succeed");

        require(world.dirtyChunkCount() == 0, "Distant Chunk must remain clean");
        require(world.findChunk(distant) == originalAddress, "Distant Chunk address must remain stable");


        std::cout << "[PASS] Distant Chunk remains unaffected\n";
    }
}



int main() {
    try {
        testRemoveExistingChunk();
        testMissingChunkRemoval();
        testRemovedChunkDirtyCleanup();
        testSixNeighborsDirty();
        testPositiveXBoundaryExposure();
        testNegativeXBoundaryExposure();
        testYBoundaryExposure();
        testZBoundaryExposure();
        testChunkRecreation();
        testDistantChunkUnaffected();


        std::cout << "\nAll World Removal tests passed.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}