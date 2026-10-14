#include <cstddef>
#include <exception>
#include <iostream>
#include <set>
#include <stdexcept>

#include "math/Vec3.h"

#include "render/MeshData.h"
#include "render/mesh/ChunkMeshFactory.h"

#include "world/BlockInteractor.h"
#include "world/BlockType.h"
#include "world/Chunk.h"
#include "world/World.h"



namespace
{
    using DirtySet = std::set<ve::ChunkPosition, ve::ChunkPositionLess>;



    void require( bool condition, const char* message ) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }



    bool sameBlock( const ve::WorldBlockPosition& a, const ve::WorldBlockPosition& b ) {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }



    DirtySet consumeDirty( ve::World& world ) {
        DirtySet result;

        world.processDirtyChunks([&]( const ve::ChunkPosition& position ) {
            result.insert(position);
        });

        return result;
    }



    bool containsChunk( const DirtySet& dirty, const ve::ChunkPosition& position ) {
        return dirty.find(position) != dirty.end();
    }



    void checkMeshIndices( const ve::World& world, const ve::ChunkPosition& position,
                           std::size_t expected, const char* message ) {
        const ve::MeshData mesh = ve::ChunkMeshFactory::create(world, position);
        require(mesh.indexCount() == expected, message);
    }



    void checkTwoDirtyChunks( ve::World& world, const ve::ChunkPosition& first,
                              const ve::ChunkPosition& second ) {
        const DirtySet dirty = consumeDirty(world);

        require(dirty.size() == 2, "Expected exactly two Dirty Chunks");
        require(containsChunk(dirty, first), "First Chunk is not Dirty");
        require(containsChunk(dirty, second), "Second Chunk is not Dirty");
    }



    void testRepeatedBreakPlaceInOneChunk() {
        constexpr int repetitions = 200;

        ve::World world{};

        const ve::WorldBlockPosition target{ 4, 2, 4 };
        const ve::WorldBlockPosition anchor{ 4, 2, 3 };
        const ve::ChunkPosition chunk{ 0, 0, 0 };
        const ve::Vec3 origin{ 4.5f, 2.5f, 6.5f };
        const ve::Vec3 direction{ 0.0f, 0.0f, -1.0f };

        world.setBlock(target, ve::BlockType::Dirt);
        world.setBlock(anchor, ve::BlockType::Stone);
        consumeDirty(world);

        checkMeshIndices(world, chunk, 60, "Initial pair must have ten visible faces");

        for (int i = 0; i < repetitions; ++i) {
            const auto broken = ve::BlockInteractor::breakBlock(world, origin, direction, 6.0f);

            require(broken.has_value(), "Repeated break failed");
            require(sameBlock(broken->block, target), "Repeated break targeted wrong block");
            require(world.blockAt(target) == ve::BlockType::Air, "Broken block remains solid");
            require(world.dirtyChunkCount() == 1, "Single-Chunk break has wrong Dirty count");

            consumeDirty(world);
            checkMeshIndices(world, chunk, 36, "Mesh after break is incorrect");

            const auto placed = ve::BlockInteractor::placeBlock(
                world, origin, direction, 6.0f, ve::BlockType::Dirt
            );

            require(placed.has_value(), "Repeated placement failed");
            require(sameBlock(placed->placedBlock, target), "Placement targeted wrong block");
            require(world.blockAt(target) == ve::BlockType::Dirt, "Placed block is missing");
            require(world.dirtyChunkCount() == 1, "Single-Chunk placement has wrong Dirty count");

            consumeDirty(world);
            checkMeshIndices(world, chunk, 60, "Mesh was not restored after placement");
        }

        require(world.chunkCount() == 1, "Repeated edits unexpectedly changed Chunk count");
        require(world.dirtyChunkCount() == 0, "Dirty queue was not emptied");

        std::cout << "[PASS] Repeated single-Chunk break/place (" << repetitions << " cycles)\n";
    }



    void testRepeatedPositiveBoundaryEdits() {
        constexpr int repetitions = 100;

        ve::World world{};

        const ve::WorldBlockPosition target{ 15, 2, 2 };
        const ve::WorldBlockPosition anchor{ 16, 2, 2 };
        const ve::ChunkPosition leftChunk{ 0, 0, 0 };
        const ve::ChunkPosition rightChunk{ 1, 0, 0 };
        const ve::Vec3 origin{ 14.5f, 2.5f, 2.5f };
        const ve::Vec3 direction{ 1.0f, 0.0f, 0.0f };

        world.setBlock(target, ve::BlockType::Dirt);
        world.setBlock(anchor, ve::BlockType::Stone);
        consumeDirty(world);

        for (int i = 0; i < repetitions; ++i) {
            const auto broken = ve::BlockInteractor::breakBlock(world, origin, direction, 6.0f);

            require(broken.has_value() && sameBlock(broken->block, target), "Positive boundary break failed");
            checkTwoDirtyChunks(world, leftChunk, rightChunk);
            checkMeshIndices(world, leftChunk, 0, "Broken positive-boundary block left geometry");
            checkMeshIndices(world, rightChunk, 36, "Positive neighbor face did not reappear");

            const auto placed = ve::BlockInteractor::placeBlock(
                world, origin, direction, 6.0f, ve::BlockType::Dirt
            );

            require(placed.has_value() && sameBlock(placed->placedBlock, target), "Positive boundary placement failed");
            checkTwoDirtyChunks(world, leftChunk, rightChunk);
            checkMeshIndices(world, leftChunk, 30, "Restored positive-boundary Mesh is incorrect");
            checkMeshIndices(world, rightChunk, 30, "Positive neighbor shared face remains visible");
        }

        require(world.chunkCount() == 2, "Positive boundary edits unexpectedly created Chunks");
        std::cout << "[PASS] Repeated positive Chunk boundary edits (" << repetitions << " cycles)\n";
    }



    void testRepeatedNegativeBoundaryEdits() {
        constexpr int repetitions = 100;

        ve::World world{};

        const ve::WorldBlockPosition target{ -17, 2, 2 };
        const ve::WorldBlockPosition anchor{ -16, 2, 2 };
        const ve::ChunkPosition leftChunk{ -2, 0, 0 };
        const ve::ChunkPosition rightChunk{ -1, 0, 0 };
        const ve::Vec3 origin{ -18.5f, 2.5f, 2.5f };
        const ve::Vec3 direction{ 1.0f, 0.0f, 0.0f };

        world.setBlock(target, ve::BlockType::Dirt);
        world.setBlock(anchor, ve::BlockType::Stone);
        consumeDirty(world);

        for (int i = 0; i < repetitions; ++i) {
            const auto broken = ve::BlockInteractor::breakBlock(world, origin, direction, 6.0f);

            require(broken.has_value() && sameBlock(broken->block, target), "Negative boundary break failed");
            checkTwoDirtyChunks(world, leftChunk, rightChunk);
            checkMeshIndices(world, leftChunk, 0, "Broken negative-boundary block left geometry");
            checkMeshIndices(world, rightChunk, 36, "Negative neighbor face did not reappear");

            const auto placed = ve::BlockInteractor::placeBlock(
                world, origin, direction, 6.0f, ve::BlockType::Dirt
            );

            require(placed.has_value() && sameBlock(placed->placedBlock, target), "Negative boundary placement failed");
            checkTwoDirtyChunks(world, leftChunk, rightChunk);
            checkMeshIndices(world, leftChunk, 30, "Restored negative-boundary Mesh is incorrect");
            checkMeshIndices(world, rightChunk, 30, "Negative neighbor shared face remains visible");
        }

        require(world.chunkCount() == 2, "Negative boundary edits unexpectedly created Chunks");
        std::cout << "[PASS] Repeated negative Chunk boundary edits (" << repetitions << " cycles)\n";
    }



    void testRepeatedChunkRemovalAndRecreation() {
        constexpr int repetitions = 80;

        ve::World world{};

        const ve::ChunkPosition chunk{ 0, 0, 0 };
        const ve::WorldBlockPosition block{ 2, 2, 2 };

        world.setBlock(block, ve::BlockType::Stone);
        consumeDirty(world);

        for (int i = 0; i < repetitions; ++i) {
            require(world.removeChunk(chunk), "Existing Chunk removal failed");
            require(world.chunkCount() == 0, "Removed Chunk still exists");
            require(world.findChunk(chunk) == nullptr, "Removed Chunk remains findable");
            require(world.blockAt(block) == ve::BlockType::Air, "Removed Chunk left block data");
            require(world.dirtyChunkCount() == 0, "Removed Chunk left unexpected Dirty entries");

            world.setBlock(block, ve::BlockType::Stone);

            require(world.chunkCount() == 1, "Chunk was not recreated");
            require(world.blockAt(block) == ve::BlockType::Stone, "Recreated Chunk lost block data");
            require(world.dirtyChunkCount() == 1, "Recreated Chunk was not Dirty");

            consumeDirty(world);
            checkMeshIndices(world, chunk, 36, "Recreated Chunk Mesh is incorrect");
        }

        std::cout << "[PASS] Repeated Chunk removal/recreation (" << repetitions << " cycles)\n";
    }



    void testDirtyDeduplicationUnderBurstEdits() {
        constexpr int repetitions = 300;
        constexpr int last = ve::Chunk::Size - 1;

        ve::World world{};

        const ve::ChunkPosition center{ 0, 0, 0 };
        const ve::ChunkPosition positiveX{ 1, 0, 0 };
        const ve::ChunkPosition positiveY{ 0, 1, 0 };
        const ve::ChunkPosition positiveZ{ 0, 0, 1 };
        const ve::ChunkPosition diagonal{ 1, 1, 1 };
        const ve::WorldBlockPosition corner{ last, last, last };

        world.getOrCreateChunk(center);
        world.getOrCreateChunk(positiveX);
        world.getOrCreateChunk(positiveY);
        world.getOrCreateChunk(positiveZ);
        world.getOrCreateChunk(diagonal);
        consumeDirty(world);

        for (int i = 0; i < repetitions; ++i) {
            const ve::BlockType next = i % 2 == 0 ? ve::BlockType::Stone : ve::BlockType::Air;
            world.setBlock(corner, next);
        }

        require(world.blockAt(corner) == ve::BlockType::Air, "Burst edits had wrong final state");
        require(world.dirtyChunkCount() == 4, "Burst edits did not deduplicate Dirty Chunks");

        const DirtySet dirty = consumeDirty(world);

        require(dirty.size() == 4, "Burst edits produced an unexpected Dirty set");
        require(containsChunk(dirty, center), "Center Chunk missing from Dirty set");
        require(containsChunk(dirty, positiveX), "+X Chunk missing from Dirty set");
        require(containsChunk(dirty, positiveY), "+Y Chunk missing from Dirty set");
        require(containsChunk(dirty, positiveZ), "+Z Chunk missing from Dirty set");
        require(!containsChunk(dirty, diagonal), "Diagonal Chunk was unnecessarily dirtied");
        require(world.dirtyChunkCount() == 0, "Burst Dirty queue was not emptied");

        checkMeshIndices(world, center, 0, "Burst edits left stale Mesh geometry");

        std::cout << "[PASS] Dirty deduplication after " << repetitions << " rapid edits\n";
    }



    void testUnrelatedChunkIsolationUnderRepeatedEdits() {
        constexpr int repetitions = 240;

        ve::World world{};

        const ve::WorldBlockPosition localBlock{ 2, 2, 2 };
        const ve::WorldBlockPosition distantBlock{ 50, 2, 2 };
        const ve::ChunkPosition localChunk{ 0, 0, 0 };
        const ve::ChunkPosition distantChunk{ 3, 0, 0 };

        world.setBlock(localBlock, ve::BlockType::Stone);
        world.setBlock(distantBlock, ve::BlockType::Stone);
        consumeDirty(world);

        for (int i = 0; i < repetitions; ++i) {
            const bool remove = i % 2 == 0;
            world.setBlock(localBlock, remove ? ve::BlockType::Air : ve::BlockType::Stone);

            const DirtySet dirty = consumeDirty(world);

            require(dirty.size() == 1, "Distant Chunk became Dirty during local edit");
            require(containsChunk(dirty, localChunk), "Local Chunk was not Dirty");
            require(!containsChunk(dirty, distantChunk), "Distant Chunk was incorrectly Dirty");

            if (i % 20 == 0) {
                checkMeshIndices(world, distantChunk, 36, "Distant Mesh changed during local edits");
            }
        }

        require(world.chunkCount() == 2, "Local edits unexpectedly created Chunks");
        require(world.blockAt(distantBlock) == ve::BlockType::Stone, "Distant block was modified");
        checkMeshIndices(world, localChunk, 36, "Local Mesh was not restored");
        checkMeshIndices(world, distantChunk, 36, "Distant Mesh was not preserved");

        std::cout << "[PASS] Unrelated Chunk isolation (" << repetitions << " edits)\n";
    }



    void testRepeatedNeighborChunkCreationAndRelease() {
        constexpr int repetitions = 60;

        ve::World world{};

        const ve::WorldBlockPosition anchor{ 15, 2, 2 };
        const ve::WorldBlockPosition target{ 16, 2, 2 };
        const ve::ChunkPosition anchorChunk{ 0, 0, 0 };
        const ve::ChunkPosition targetChunk{ 1, 0, 0 };
        const ve::Vec3 origin{ 18.5f, 2.5f, 2.5f };
        const ve::Vec3 direction{ -1.0f, 0.0f, 0.0f };

        world.setBlock(anchor, ve::BlockType::Stone);
        consumeDirty(world);

        for (int i = 0; i < repetitions; ++i) {
            const auto placed = ve::BlockInteractor::placeBlock(
                world, origin, direction, 6.0f, ve::BlockType::Dirt
            );

            require(placed.has_value() && sameBlock(placed->placedBlock, target), "Neighbor creation placement failed");
            require(world.chunkCount() == 2, "Neighbor Chunk was not created");
            checkTwoDirtyChunks(world, anchorChunk, targetChunk);
            checkMeshIndices(world, anchorChunk, 30, "Anchor face was not hidden");
            checkMeshIndices(world, targetChunk, 30, "Created neighbor has wrong Mesh");

            const auto broken = ve::BlockInteractor::breakBlock(world, origin, direction, 6.0f);

            require(broken.has_value() && sameBlock(broken->block, target), "Neighbor breaking failed");
            checkTwoDirtyChunks(world, anchorChunk, targetChunk);
            checkMeshIndices(world, anchorChunk, 36, "Anchor face did not reappear");
            checkMeshIndices(world, targetChunk, 0, "Removed neighbor still has geometry");

            require(world.removeChunk(targetChunk), "Empty neighbor Chunk removal failed");
            require(world.chunkCount() == 1, "Removed neighbor Chunk remains in World");
            require(world.blockAt(target) == ve::BlockType::Air, "Removed neighbor left stale block data");

            const DirtySet afterRemoval = consumeDirty(world);

            require(containsChunk(afterRemoval, anchorChunk), "Anchor was not dirtied after neighbor removal");
            checkMeshIndices(world, anchorChunk, 36, "Anchor Mesh changed after empty neighbor removal");
        }

        std::cout << "[PASS] Neighbor Chunk create/break/remove (" << repetitions << " cycles)\n";
    }
}



int main() {
    try {
        testRepeatedBreakPlaceInOneChunk();
        testRepeatedPositiveBoundaryEdits();
        testRepeatedNegativeBoundaryEdits();
        testRepeatedChunkRemovalAndRecreation();
        testDirtyDeduplicationUnderBurstEdits();
        testUnrelatedChunkIsolationUnderRepeatedEdits();
        testRepeatedNeighborChunkCreationAndRelease();

        std::cout << "\nAll World Stability Stress tests passed.\n";
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
