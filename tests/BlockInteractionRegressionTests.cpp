#include <cstddef>
#include <exception>
#include <initializer_list>
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

    bool samePosition( const ve::WorldBlockPosition& a, const ve::WorldBlockPosition& b ) {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }

    DirtySet takeDirty( ve::World& world ) {
        DirtySet result;

        world.processDirtyChunks([&]( const ve::ChunkPosition& position ) {
            result.insert(position);
        });

        return result;
    }

    void expectDirty( ve::World& world, std::initializer_list<ve::ChunkPosition> expected ) {
        const DirtySet actual = takeDirty(world);

        require(actual.size() == expected.size(), "Unexpected number of Dirty Chunks");

        for (const ve::ChunkPosition& position : expected) {
            require(actual.find(position) != actual.end(), "Expected Dirty Chunk is missing");
        }

        require(world.dirtyChunkCount() == 0, "Dirty queue was not consumed");
    }

    void expectBlock( const ve::World& world, const ve::WorldBlockPosition& position,
                      ve::BlockType expected ) {
        require(world.blockAt(position) == expected, "Unexpected World BlockType");
    }

    void expectMesh( const ve::World& world, const ve::ChunkPosition& position,
                     std::size_t expectedIndices ) {
        const ve::MeshData mesh = ve::ChunkMeshFactory::create(world, position);

        require(mesh.indexCount() == expectedIndices, "Unexpected Chunk Mesh index count");
        require(mesh.triangleCount() == expectedIndices / 3, "Unexpected triangle count");
    }

    void expectBroken( ve::World& world, const ve::Vec3& origin, const ve::Vec3& direction,
                       const ve::WorldBlockPosition& expected ) {
        const auto hit = ve::BlockInteractor::breakBlock(world, origin, direction, 6.0f);

        require(hit.has_value(), "Expected Block Breaking to succeed");
        require(samePosition(hit->block, expected), "Wrong Block was broken");
        expectBlock(world, expected, ve::BlockType::Air);
    }

    void expectPlaced( ve::World& world, const ve::Vec3& origin, const ve::Vec3& direction,
                       const ve::WorldBlockPosition& expected, ve::BlockType type ) {
        const auto result = ve::BlockInteractor::placeBlock(world, origin, direction, 6.0f, type);

        require(result.has_value(), "Expected Block Placement to succeed");
        require(samePosition(result->placedBlock, expected), "Wrong Block was placed");
        expectBlock(world, expected, type);
    }



    void testSingleChunkEditCycle() {
        ve::World world{};

        const ve::WorldBlockPosition first{ 2, 2, 2 };
        const ve::WorldBlockPosition second{ 3, 2, 2 };
        const ve::ChunkPosition chunk{ 0, 0, 0 };
        const ve::Vec3 origin{ 0.5f, 2.5f, 2.5f };
        const ve::Vec3 direction{ 1.0f, 0.0f, 0.0f };

        world.setBlock(first, ve::BlockType::Stone);
        world.setBlock(second, ve::BlockType::Dirt);
        takeDirty(world);

        expectMesh(world, chunk, 60);

        expectBroken(world, origin, direction, first);
        expectDirty(world, { chunk });
        expectMesh(world, chunk, 36);
        expectBlock(world, second, ve::BlockType::Dirt);

        expectPlaced(world, origin, direction, first, ve::BlockType::Grass);
        expectDirty(world, { chunk });
        expectMesh(world, chunk, 60);
        expectBlock(world, second, ve::BlockType::Dirt);

        std::cout << "[PASS] Single Chunk break-place Mesh cycle\n";
    }



    void testPositiveBoundaryLifecycle() {
        ve::World world{};

        const ve::WorldBlockPosition first{ 15, 1, 1 };
        const ve::WorldBlockPosition second{ 16, 1, 1 };
        const ve::WorldBlockPosition distant{ 34, 1, 1 };
        const ve::ChunkPosition firstChunk{ 0, 0, 0 };
        const ve::ChunkPosition secondChunk{ 1, 0, 0 };
        const ve::ChunkPosition distantChunk{ 2, 0, 0 };
        const ve::Vec3 fromLeft{ 13.5f, 1.5f, 1.5f };
        const ve::Vec3 fromRight{ 18.5f, 1.5f, 1.5f };
        const ve::Vec3 positiveX{ 1.0f, 0.0f, 0.0f };
        const ve::Vec3 negativeX{ -1.0f, 0.0f, 0.0f };

        world.setBlock(first, ve::BlockType::Stone);
        world.setBlock(second, ve::BlockType::Dirt);
        world.setBlock(distant, ve::BlockType::Grass);
        takeDirty(world);

        expectMesh(world, firstChunk, 30);
        expectMesh(world, secondChunk, 30);
        expectMesh(world, distantChunk, 36);

        expectBroken(world, fromLeft, positiveX, first);
        expectDirty(world, { firstChunk, secondChunk });
        expectMesh(world, firstChunk, 0);
        expectMesh(world, secondChunk, 36);

        expectPlaced(world, fromLeft, positiveX, first, ve::BlockType::Stone);
        expectDirty(world, { firstChunk, secondChunk });
        expectMesh(world, firstChunk, 30);
        expectMesh(world, secondChunk, 30);

        expectBroken(world, fromRight, negativeX, second);
        expectDirty(world, { firstChunk, secondChunk });
        expectMesh(world, firstChunk, 36);
        expectMesh(world, secondChunk, 0);

        expectPlaced(world, fromRight, negativeX, second, ve::BlockType::Dirt);
        expectDirty(world, { firstChunk, secondChunk });
        expectMesh(world, firstChunk, 30);
        expectMesh(world, secondChunk, 30);
        expectMesh(world, distantChunk, 36);
        expectBlock(world, distant, ve::BlockType::Grass);

        std::cout << "[PASS] Positive Chunk boundary multi-step lifecycle\n";
    }



    void testNegativeBoundaryLifecycle() {
        ve::World world{};

        const ve::WorldBlockPosition first{ -17, 1, 1 };
        const ve::WorldBlockPosition second{ -16, 1, 1 };
        const ve::ChunkPosition firstChunk{ -2, 0, 0 };
        const ve::ChunkPosition secondChunk{ -1, 0, 0 };
        const ve::Vec3 origin{ -13.5f, 1.5f, 1.5f };
        const ve::Vec3 direction{ -1.0f, 0.0f, 0.0f };

        world.setBlock(first, ve::BlockType::Stone);
        world.setBlock(second, ve::BlockType::Dirt);
        takeDirty(world);

        expectMesh(world, firstChunk, 30);
        expectMesh(world, secondChunk, 30);

        expectBroken(world, origin, direction, second);
        expectDirty(world, { firstChunk, secondChunk });
        expectMesh(world, firstChunk, 36);
        expectMesh(world, secondChunk, 0);

        expectPlaced(world, origin, direction, second, ve::BlockType::Grass);
        expectDirty(world, { firstChunk, secondChunk });
        expectMesh(world, firstChunk, 30);
        expectMesh(world, secondChunk, 30);

        expectBroken(world, origin, direction, second);
        expectDirty(world, { firstChunk, secondChunk });
        expectBroken(world, origin, direction, first);
        expectDirty(world, { firstChunk, secondChunk });
        expectMesh(world, firstChunk, 0);
        expectMesh(world, secondChunk, 0);

        const auto miss = ve::BlockInteractor::breakBlock(world, origin, direction, 6.0f);
        require(!miss.has_value(), "Breaking an empty ray must fail");
        expectDirty(world, { });

        std::cout << "[PASS] Negative Chunk boundary and full removal\n";
    }



    void testNewChunkCreationByPlacement() {
        ve::World world{};

        const ve::WorldBlockPosition anchor{ 15, 2, 2 };
        const ve::WorldBlockPosition target{ 16, 2, 2 };
        const ve::ChunkPosition originalChunk{ 0, 0, 0 };
        const ve::ChunkPosition newChunk{ 1, 0, 0 };
        const ve::Vec3 origin{ 18.5f, 2.5f, 2.5f };
        const ve::Vec3 direction{ -1.0f, 0.0f, 0.0f };

        world.setBlock(anchor, ve::BlockType::Stone);
        takeDirty(world);

        require(world.findChunk(newChunk) == nullptr, "Neighbor Chunk must start missing");
        require(world.chunkCount() == 1, "Unexpected initial Chunk count");
        expectMesh(world, originalChunk, 36);

        expectPlaced(world, origin, direction, target, ve::BlockType::Grass);
        require(world.findChunk(newChunk) != nullptr, "Placement did not create a Chunk");
        require(world.chunkCount() == 2, "Placement must create one Chunk");
        expectDirty(world, { originalChunk, newChunk });
        expectMesh(world, originalChunk, 30);
        expectMesh(world, newChunk, 30);

        expectBroken(world, origin, direction, target);
        expectDirty(world, { originalChunk, newChunk });
        expectMesh(world, originalChunk, 36);
        expectMesh(world, newChunk, 0);

        std::cout << "[PASS] Placement creates a Chunk and updates its neighbor\n";
    }



    void testCornerNeighborsAndDiagonalIsolation() {
        ve::World world{};

        const ve::WorldBlockPosition center{ 15, 15, 15 };
        const ve::WorldBlockPosition xNeighbor{ 16, 15, 15 };
        const ve::WorldBlockPosition yNeighbor{ 15, 16, 15 };
        const ve::WorldBlockPosition zNeighbor{ 15, 15, 16 };
        const ve::WorldBlockPosition diagonal{ 17, 17, 17 };

        const ve::ChunkPosition c0{ 0, 0, 0 };
        const ve::ChunkPosition cx{ 1, 0, 0 };
        const ve::ChunkPosition cy{ 0, 1, 0 };
        const ve::ChunkPosition cz{ 0, 0, 1 };
        const ve::ChunkPosition cd{ 1, 1, 1 };
        const ve::Vec3 origin{ 13.5f, 15.5f, 15.5f };
        const ve::Vec3 direction{ 1.0f, 0.0f, 0.0f };

        world.setBlock(center, ve::BlockType::Stone);
        world.setBlock(xNeighbor, ve::BlockType::Dirt);
        world.setBlock(yNeighbor, ve::BlockType::Dirt);
        world.setBlock(zNeighbor, ve::BlockType::Dirt);
        world.setBlock(diagonal, ve::BlockType::Stone);
        takeDirty(world);

        expectMesh(world, c0, 18);
        expectMesh(world, cx, 30);
        expectMesh(world, cy, 30);
        expectMesh(world, cz, 30);
        expectMesh(world, cd, 36);

        expectBroken(world, origin, direction, center);
        expectDirty(world, { c0, cx, cy, cz });
        expectMesh(world, c0, 0);
        expectMesh(world, cx, 36);
        expectMesh(world, cy, 36);
        expectMesh(world, cz, 36);
        expectMesh(world, cd, 36);

        expectPlaced(world, origin, direction, center, ve::BlockType::Stone);
        expectDirty(world, { c0, cx, cy, cz });
        expectMesh(world, c0, 18);
        expectMesh(world, cx, 30);
        expectMesh(world, cy, 30);
        expectMesh(world, cz, 30);
        expectMesh(world, cd, 36);

        std::cout << "[PASS] Corner edit updates face neighbors but not diagonal\n";
    }



    void testDirtyBatchDeduplication() {
        ve::World world{};

        const ve::WorldBlockPosition target{ 15, 1, 1 };
        const ve::WorldBlockPosition anchor{ 16, 1, 1 };
        const ve::ChunkPosition a{ 0, 0, 0 };
        const ve::ChunkPosition b{ 1, 0, 0 };
        const ve::Vec3 origin{ 13.5f, 1.5f, 1.5f };
        const ve::Vec3 direction{ 1.0f, 0.0f, 0.0f };

        world.setBlock(target, ve::BlockType::Stone);
        world.setBlock(anchor, ve::BlockType::Dirt);
        takeDirty(world);

        // Four World Edits Before Processing the Dirty Queue

        expectBroken(world, origin, direction, target);
        expectPlaced(world, origin, direction, target, ve::BlockType::Stone);
        expectBroken(world, origin, direction, target);
        expectPlaced(world, origin, direction, target, ve::BlockType::Stone);

        require(world.dirtyChunkCount() == 2, "Dirty queue contains duplicate Chunks");
        expectDirty(world, { a, b });
        expectMesh(world, a, 30);
        expectMesh(world, b, 30);

        std::cout << "[PASS] Multiple edits deduplicate Dirty Chunks\n";
    }



    void testRemovalAndRecreation() {
        ve::World world{};

        const ve::WorldBlockPosition removedBlock{ 15, 1, 1 };
        const ve::WorldBlockPosition survivingBlock{ 16, 1, 1 };
        const ve::ChunkPosition removedChunk{ 0, 0, 0 };
        const ve::ChunkPosition survivingChunk{ 1, 0, 0 };
        const ve::Vec3 origin{ 13.5f, 1.5f, 1.5f };
        const ve::Vec3 direction{ 1.0f, 0.0f, 0.0f };

        world.setBlock(removedBlock, ve::BlockType::Stone);
        world.setBlock(survivingBlock, ve::BlockType::Dirt);
        takeDirty(world);

        require(world.removeChunk(removedChunk), "Chunk removal failed");
        require(world.findChunk(removedChunk) == nullptr, "Removed Chunk still exists");
        expectDirty(world, { survivingChunk });
        expectMesh(world, survivingChunk, 36);

        expectPlaced(world, origin, direction, removedBlock, ve::BlockType::Grass);
        require(world.findChunk(removedChunk) != nullptr, "Recreated Chunk does not exist");
        expectDirty(world, { removedChunk, survivingChunk });
        expectMesh(world, removedChunk, 30);
        expectMesh(world, survivingChunk, 30);

        std::cout << "[PASS] Removed Chunk is recreated through Block Placement\n";
    }



    void testMissesDoNotChangeWorld() {
        ve::World world{};

        const ve::WorldBlockPosition target{ 2, 2, 2 };
        const ve::ChunkPosition chunk{ 0, 0, 0 };

        world.setBlock(target, ve::BlockType::Stone);
        takeDirty(world);

        const std::size_t originalChunkCount = world.chunkCount();
        const auto broken = ve::BlockInteractor::breakBlock(
            world, ve::Vec3{ 0.5f, 5.5f, 2.5f }, ve::Vec3{ 1.0f, 0.0f, 0.0f }, 6.0f
        );
        const auto placed = ve::BlockInteractor::placeBlock(
            world, ve::Vec3{ 0.5f, 5.5f, 2.5f }, ve::Vec3{ 1.0f, 0.0f, 0.0f },
            6.0f, ve::BlockType::Dirt
        );

        require(!broken.has_value() && !placed.has_value(), "A miss modified the World");
        require(world.chunkCount() == originalChunkCount, "A miss created a Chunk");
        expectBlock(world, target, ve::BlockType::Stone);
        expectDirty(world, { });
        expectMesh(world, chunk, 36);

        std::cout << "[PASS] Misses preserve World, Dirty queue and Mesh\n";
    }
}



int main() {
    try {
        testSingleChunkEditCycle();
        testPositiveBoundaryLifecycle();
        testNegativeBoundaryLifecycle();
        testNewChunkCreationByPlacement();
        testCornerNeighborsAndDiagonalIsolation();
        testDirtyBatchDeduplication();
        testRemovalAndRecreation();
        testMissesDoNotChangeWorld();

        std::cout << "\nAll Block Interaction Regression tests passed.\n";
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
