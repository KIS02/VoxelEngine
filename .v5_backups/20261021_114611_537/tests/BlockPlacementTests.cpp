#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>

#include "math/Vec3.h"

#include "world/BlockInteractor.h"
#include "world/BlockType.h"
#include "world/World.h"



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



    void testPlacementOnPositiveXRay() {
        ve::World world{};

        const ve::WorldBlockPosition anchor{ 2, 1, 1 };
        const ve::WorldBlockPosition target{ 1, 1, 1 };

        world.setBlock(anchor, ve::BlockType::Stone);



        const auto placed = ve::BlockInteractor::placeBlock(
            world,
            ve::Vec3{ 0.5f, 1.5f, 1.5f },
            ve::Vec3{ 1.0f, 0.0f, 0.0f },
            6.0f,
            ve::BlockType::Dirt
        );



        require(placed.has_value(), "Block placement must succeed");
        require(samePosition(placed->placedBlock, target), "Incorrect placement position");
        require(samePosition(placed->attachedTo, anchor), "Incorrect anchor block");

        require(world.blockAt(anchor) == ve::BlockType::Stone, "Anchor block must remain");
        require(world.blockAt(target) == ve::BlockType::Dirt, "Target block must become Dirt");

        std::cout << "[PASS] Block placement against positive X Ray\n";
    }



    void testPlacementOnNegativeXRay() {
        ve::World world{};

        const ve::WorldBlockPosition anchor{ -2, 0, 0 };
        const ve::WorldBlockPosition target{ -1, 0, 0 };

        world.setBlock(anchor, ve::BlockType::Stone);



        const auto placed = ve::BlockInteractor::placeBlock(
            world,
            ve::Vec3{ 0.5f, 0.5f, 0.5f },
            ve::Vec3{ -1.0f, 0.0f, 0.0f },
            6.0f,
            ve::BlockType::Dirt
        );



        require(placed.has_value(), "Negative X placement must succeed");
        require(samePosition(placed->placedBlock, target), "Incorrect negative X target");

        require(world.blockAt(anchor) == ve::BlockType::Stone, "Anchor block was overwritten");
        require(world.blockAt(target) == ve::BlockType::Dirt, "Negative X block not placed");

        std::cout << "[PASS] Block placement against negative X Ray\n";
    }



    void checkFacePlacement( const ve::WorldBlockPosition& anchor,
                             const ve::WorldBlockPosition& target,
                             const ve::Vec3& direction ) {
        ve::World world{};

        world.setBlock(anchor, ve::BlockType::Stone);



        const auto placed = ve::BlockInteractor::placeBlock(
            world,
            ve::Vec3{ 0.5f, 0.5f, 0.5f },
            direction,
            6.0f,
            ve::BlockType::Grass
        );



        require(placed.has_value(), "Expected face placement missing");
        require(samePosition(placed->placedBlock, target), "Incorrect face placement target");
        require(world.blockAt(target) == ve::BlockType::Grass, "Wrong placed BlockType");
        require(world.blockAt(anchor) == ve::BlockType::Stone, "Anchor block changed");
    }



    void testSixFacePlacements() {
        checkFacePlacement(
            ve::WorldBlockPosition{ 2, 0, 0 },
            ve::WorldBlockPosition{ 1, 0, 0 },
            ve::Vec3{ 1.0f, 0.0f, 0.0f }
        );

        checkFacePlacement(
            ve::WorldBlockPosition{ -2, 0, 0 },
            ve::WorldBlockPosition{ -1, 0, 0 },
            ve::Vec3{ -1.0f, 0.0f, 0.0f }
        );

        checkFacePlacement(
            ve::WorldBlockPosition{ 0, 2, 0 },
            ve::WorldBlockPosition{ 0, 1, 0 },
            ve::Vec3{ 0.0f, 1.0f, 0.0f }
        );

        checkFacePlacement(
            ve::WorldBlockPosition{ 0, -2, 0 },
            ve::WorldBlockPosition{ 0, -1, 0 },
            ve::Vec3{ 0.0f, -1.0f, 0.0f }
        );

        checkFacePlacement(
            ve::WorldBlockPosition{ 0, 0, 2 },
            ve::WorldBlockPosition{ 0, 0, 1 },
            ve::Vec3{ 0.0f, 0.0f, 1.0f }
        );

        checkFacePlacement(
            ve::WorldBlockPosition{ 0, 0, -2 },
            ve::WorldBlockPosition{ 0, 0, -1 },
            ve::Vec3{ 0.0f, 0.0f, -1.0f }
        );

        std::cout << "[PASS] Block placement on all six faces\n";
    }



    void testEmptyWorld() {
        ve::World world{};

        const auto placed = ve::BlockInteractor::placeBlock(
            world,
            ve::Vec3{ 0.5f, 0.5f, 0.5f },
            ve::Vec3{ 1.0f, 0.0f, 0.0f },
            6.0f,
            ve::BlockType::Dirt
        );



        require(!placed.has_value(), "Empty World must not allow placement");
        require(world.chunkCount() == 0, "Miss must not create a Chunk");

        std::cout << "[PASS] No placement when Ray misses\n";
    }



    void testMaximumReach() {
        ve::World world{};

        const ve::WorldBlockPosition anchor{ 3, 0, 0 };
        const ve::WorldBlockPosition target{ 2, 0, 0 };

        world.setBlock(anchor, ve::BlockType::Stone);



        const auto tooShort = ve::BlockInteractor::placeBlock(
            world,
            ve::Vec3{ 0.5f, 0.5f, 0.5f },
            ve::Vec3{ 1.0f, 0.0f, 0.0f },
            2.49f,
            ve::BlockType::Dirt
        );



        require(!tooShort.has_value(), "Block outside reach must not be placed");
        require(world.blockAt(target) == ve::BlockType::Air, "Out-of-range placement changed World");



        const auto exact = ve::BlockInteractor::placeBlock(
            world,
            ve::Vec3{ 0.5f, 0.5f, 0.5f },
            ve::Vec3{ 1.0f, 0.0f, 0.0f },
            2.5f,
            ve::BlockType::Dirt
        );



        require(exact.has_value(), "Placement at exact maximum reach must succeed");
        require(world.blockAt(target) == ve::BlockType::Dirt, "Block at reach was not placed");

        std::cout << "[PASS] Block placement respects maximum reach\n";
    }



    void testRayStartsInsideBlock() {
        ve::World world{};

        const ve::WorldBlockPosition anchor{ 0, 0, 0 };

        world.setBlock(anchor, ve::BlockType::Stone);



        const auto placed = ve::BlockInteractor::placeBlock(
            world,
            ve::Vec3{ 0.5f, 0.5f, 0.5f },
            ve::Vec3{ 1.0f, 0.0f, 0.0f },
            6.0f,
            ve::BlockType::Dirt
        );



        require(!placed.has_value(), "Inside-block Ray must not place without a face");
        require(world.blockAt(anchor) == ve::BlockType::Stone, "Original block must remain");

        std::cout << "[PASS] Placement rejected for inside-block Ray\n";
    }



    void testCameraCellProtection() {
        ve::World world{};

        const ve::WorldBlockPosition anchor{ 1, 0, 0 };
        const ve::WorldBlockPosition cameraCell{ 0, 0, 0 };

        world.setBlock(anchor, ve::BlockType::Stone);



        const auto placed = ve::BlockInteractor::placeBlock(
            world,
            ve::Vec3{ 0.5f, 0.5f, 0.5f },
            ve::Vec3{ 1.0f, 0.0f, 0.0f },
            6.0f,
            ve::BlockType::Dirt
        );



        require(!placed.has_value(), "Cannot place a block inside the Camera cell");
        require(world.blockAt(cameraCell) == ve::BlockType::Air, "Camera cell must remain Air");

        std::cout << "[PASS] Camera cell is protected from placement\n";
    }



    void testNegativeChunkBoundary() {
        ve::World world{};

        const ve::WorldBlockPosition anchor{ -17, 1, 1 };
        const ve::WorldBlockPosition target{ -16, 1, 1 };

        world.setBlock(anchor, ve::BlockType::Stone);



        const auto placed = ve::BlockInteractor::placeBlock(
            world,
            ve::Vec3{ -14.5f, 1.5f, 1.5f },
            ve::Vec3{ -1.0f, 0.0f, 0.0f },
            6.0f,
            ve::BlockType::Dirt
        );



        require(placed.has_value(), "Negative Chunk boundary placement must succeed");
        require(samePosition(placed->placedBlock, target), "Incorrect negative Chunk target");
        require(world.blockAt(target) == ve::BlockType::Dirt, "Negative Chunk target was not filled");

        std::cout << "[PASS] Placement across negative Chunk boundary\n";
    }



    void testPositiveChunkBoundary() {
        ve::World world{};

        const ve::WorldBlockPosition anchor{ 15, 1, 1 };
        const ve::WorldBlockPosition target{ 16, 1, 1 };

        world.setBlock(anchor, ve::BlockType::Stone);



        const auto placed = ve::BlockInteractor::placeBlock(
            world,
            ve::Vec3{ 17.5f, 1.5f, 1.5f },
            ve::Vec3{ -1.0f, 0.0f, 0.0f },
            6.0f,
            ve::BlockType::Dirt
        );



        require(placed.has_value(), "Positive Chunk boundary placement must succeed");
        require(samePosition(placed->placedBlock, target), "Incorrect positive Chunk target");
        require(world.blockAt(target) == ve::BlockType::Dirt, "Positive Chunk target was not filled");

        std::cout << "[PASS] Placement across positive Chunk boundary\n";
    }



    void testDirtyChunkScheduling() {
        ve::World world{};

        world.setBlock(
            ve::WorldBlockPosition{ 2, 2, 2 },
            ve::BlockType::Stone
        );



        // Remove Dirty State Left by the Initial Setup

        world.processDirtyChunks([]( const ve::ChunkPosition& ) { });

        require(world.dirtyChunkCount() == 0, "Initial Dirty state must be cleared");



        const auto placed = ve::BlockInteractor::placeBlock(
            world,
            ve::Vec3{ 0.5f, 2.5f, 2.5f },
            ve::Vec3{ 1.0f, 0.0f, 0.0f },
            6.0f,
            ve::BlockType::Dirt
        );



        require(placed.has_value(), "Block placement must succeed");
        require(world.dirtyChunkCount() > 0, "Placement must mark a Chunk Dirty");



        bool modifiedChunkWasDirty = false;

        world.processDirtyChunks([&]( const ve::ChunkPosition& position ) {
            if (position.x == 0 && position.y == 0 && position.z == 0) {
                modifiedChunkWasDirty = true;
            }
        });



        require(modifiedChunkWasDirty, "Modified Chunk was not marked Dirty");

        std::cout << "[PASS] Block placement schedules Dirty Chunk\n";
    }



    void testAirPlacementRejected() {
        ve::World world{};

        world.setBlock(
            ve::WorldBlockPosition{ 2, 0, 0 },
            ve::BlockType::Stone
        );



        bool rejected = false;

        try {
            (void)ve::BlockInteractor::placeBlock(
                world,
                ve::Vec3{ 0.5f, 0.5f, 0.5f },
                ve::Vec3{ 1.0f, 0.0f, 0.0f },
                6.0f,
                ve::BlockType::Air
            );
        }
        catch (const std::invalid_argument&) {
            rejected = true;
        }



        require(rejected, "Placing Air must be rejected");
        require(
            world.blockAt(ve::WorldBlockPosition{ 2, 0, 0 }) == ve::BlockType::Stone,
            "Invalid placement changed World"
        );

        std::cout << "[PASS] Air placement is rejected\n";
    }
}



int main() {
    try {
        testPlacementOnPositiveXRay();
        testPlacementOnNegativeXRay();
        testSixFacePlacements();
        testEmptyWorld();
        testMaximumReach();
        testRayStartsInsideBlock();
        testCameraCellProtection();
        testNegativeChunkBoundary();
        testPositiveChunkBoundary();
        testDirtyChunkScheduling();
        testAirPlacementRejected();



        std::cout << "\nAll Block Placement tests passed.\n";

        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}