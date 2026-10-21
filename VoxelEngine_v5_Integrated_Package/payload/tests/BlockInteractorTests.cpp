#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>

#include "math/Vec3.h"

#include "gameplay/interaction/BlockInteractor.h"
#include "voxel/block/BlockId.h"
#include "voxel/chunk/VoxelStorage.h"



namespace
{
    void require( bool condition, const char* message ) {
        if (!condition) {
            throw std::runtime_error(message);
        }
    }



    bool samePosition( const ve::WorldBlockPosition& a, const ve::WorldBlockPosition& b ) {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }



    void testBlockBecomesAir() {
        ve::VoxelStorage world{};

        const ve::WorldBlockPosition target{ 0, 1, 0 };

        world.setBlock(target, ve::BlockId::Stone);



        const auto broken = ve::BlockInteractor::breakBlock(
            world,
            ve::Vec3{ 0.5f, 1.5f, 3.5f },
            ve::Vec3{ 0.0f, 0.0f, -1.0f },
            6.0f
        );



        require(broken.has_value(), "Stone block must be hit");
        require(samePosition(broken->block, target), "Incorrect block broken");
        require(broken->type == ve::BlockId::Stone, "Original block type must be preserved");

        require(
            world.blockAt(target) == ve::BlockId::Air,
            "Broken block must become Air"
        );

        std::cout << "[PASS] Solid block becomes Air\n";
    }



    void testOnlyNearestBlockRemoved() {
        ve::VoxelStorage world{};

        const ve::WorldBlockPosition nearBlock{ 0, 1, 2 };
        const ve::WorldBlockPosition farBlock{ 0, 1, 0 };

        world.setBlock(nearBlock, ve::BlockId::Grass);
        world.setBlock(farBlock, ve::BlockId::Stone);



        const auto broken = ve::BlockInteractor::breakBlock(
            world,
            ve::Vec3{ 0.5f, 1.5f, 5.5f },
            ve::Vec3{ 0.0f, 0.0f, -1.0f },
            6.0f
        );



        require(broken.has_value(), "Nearest block must be hit");
        require(samePosition(broken->block, nearBlock), "Wrong nearest block");

        require(world.blockAt(nearBlock) == ve::BlockId::Air, "Near block was not removed");
        require(world.blockAt(farBlock) == ve::BlockId::Stone, "Far block must remain");

        std::cout << "[PASS] Only nearest block is removed\n";
    }



    void testRepeatedRequests() {
        ve::VoxelStorage world{};

        const ve::WorldBlockPosition first{ 0, 1, 2 };
        const ve::WorldBlockPosition second{ 0, 1, 1 };

        world.setBlock(first, ve::BlockId::Grass);
        world.setBlock(second, ve::BlockId::Dirt);



        const ve::Vec3 origin{ 0.5f, 1.5f, 5.5f };
        const ve::Vec3 direction{ 0.0f, 0.0f, -1.0f };



        const auto firstHit = ve::BlockInteractor::breakBlock(
            world, origin, direction, 6.0f
        );

        require(firstHit.has_value(), "First request must hit");
        require(samePosition(firstHit->block, first), "First request hit wrong block");
        require(world.blockAt(second) == ve::BlockId::Dirt, "Second block removed too early");



        const auto secondHit = ve::BlockInteractor::breakBlock(
            world, origin, direction, 6.0f
        );

        require(secondHit.has_value(), "Second request must hit");
        require(samePosition(secondHit->block, second), "Second request hit wrong block");

        require(world.blockAt(first) == ve::BlockId::Air, "First block remains");
        require(world.blockAt(second) == ve::BlockId::Air, "Second block remains");

        std::cout << "[PASS] Repeated requests remove successive blocks\n";
    }



    void testMissDoesNotChangeWorld() {
        ve::VoxelStorage world{};

        const ve::WorldBlockPosition target{ 0, 1, 0 };

        world.setBlock(target, ve::BlockId::Stone);



        const auto broken = ve::BlockInteractor::breakBlock(
            world,
            ve::Vec3{ 0.5f, 2.5f, 3.5f },
            ve::Vec3{ 0.0f, 0.0f, -1.0f },
            6.0f
        );



        require(!broken.has_value(), "Off-axis Ray must miss");
        require(world.blockAt(target) == ve::BlockId::Stone, "Miss must preserve block");

        std::cout << "[PASS] Miss leaves VoxelStorage unchanged\n";
    }



    void testMaximumReach() {
        ve::VoxelStorage world{};

        const ve::WorldBlockPosition target{ 0, 1, 2 };

        world.setBlock(target, ve::BlockId::Stone);



        const ve::Vec3 origin{ 0.5f, 1.5f, 5.5f };
        const ve::Vec3 direction{ 0.0f, 0.0f, -1.0f };



        const auto tooShort = ve::BlockInteractor::breakBlock(
            world, origin, direction, 2.49f
        );

        require(!tooShort.has_value(), "Block outside reach must not be removed");

        require(
            world.blockAt(target) == ve::BlockId::Stone,
            "Out-of-range block must remain"
        );



        const auto exact = ve::BlockInteractor::breakBlock(
            world, origin, direction, 2.5f
        );

        require(exact.has_value(), "Block at exact reach must be removed");
        require(world.blockAt(target) == ve::BlockId::Air, "Block at reach not removed");

        std::cout << "[PASS] Block breaking respects maximum reach\n";
    }



    void testNegativeChunkCoordinates() {
        ve::VoxelStorage world{};

        const ve::WorldBlockPosition target{ -17, 1, 1 };

        world.setBlock(target, ve::BlockId::Stone);



        const auto broken = ve::BlockInteractor::breakBlock(
            world,
            ve::Vec3{ -15.5f, 1.5f, 1.5f },
            ve::Vec3{ -1.0f, 0.0f, 0.0f },
            3.0f
        );



        require(broken.has_value(), "Negative Chunk block must be found");
        require(samePosition(broken->block, target), "Incorrect negative block coordinate");

        require(
            world.blockAt(target) == ve::BlockId::Air,
            "Negative Chunk block was not removed"
        );

        std::cout << "[PASS] Block breaking across negative Chunk boundary\n";
    }



    void testDirtyChunkScheduling() {
        ve::VoxelStorage world{};

        const ve::WorldBlockPosition target{ 2, 2, 2 };

        world.setBlock(target, ve::BlockId::Stone);



        // Clear the Initial Dirty State

        world.processDirtyChunks([]( const ve::ChunkPosition& ) { });

        require(world.dirtyChunkCount() == 0, "Initial Dirty state must be cleared");



        const auto broken = ve::BlockInteractor::breakBlock(
            world,
            ve::Vec3{ 2.5f, 2.5f, 5.5f },
            ve::Vec3{ 0.0f, 0.0f, -1.0f },
            6.0f
        );



        require(broken.has_value(), "Block must be removed");

        require(
            world.dirtyChunkCount() > 0,
            "Breaking a block must schedule Chunk mesh rebuilding"
        );



        bool targetChunkWasDirty = false;

        world.processDirtyChunks([&]( const ve::ChunkPosition& position ) {
            if (position.x == 0 && position.y == 0 && position.z == 0) {
                targetChunkWasDirty = true;
            }
        });



        require(targetChunkWasDirty, "Modified Chunk must be Dirty");

        std::cout << "[PASS] Block breaking schedules Dirty Chunk\n";
    }
}



int main() {
    try {
        testBlockBecomesAir();
        testOnlyNearestBlockRemoved();
        testRepeatedRequests();
        testMissDoesNotChangeWorld();
        testMaximumReach();
        testNegativeChunkCoordinates();
        testDirtyChunkScheduling();



        std::cout << "\nAll Block Interactor tests passed.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}