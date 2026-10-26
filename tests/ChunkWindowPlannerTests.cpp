#include <algorithm>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "voxel/streaming/ChunkWindowPlanner.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    bool contains( const std::vector<ve::ChunkPosition>& positions, const ve::ChunkPosition& target ) {
        return std::any_of(positions.begin(), positions.end(), [&]( const ve::ChunkPosition& position ) {
            return position.x == target.x && position.y == target.y && position.z == target.z;
        });
    }


    void testInitialWindow() {
        const ve::VoxelStorage storage{};
        const ve::ChunkWindowPlan plan = ve::planHorizontalChunkWindow(
            ve::Vec3{ 8.5f, 20.0f, 8.5f }, 1, storage
        );

        require(plan.toLoad.size() == 9 && plan.toUnload.empty(),
                "Empty world should request nine Chunks");

        require(contains(plan.toLoad, ve::ChunkPosition{ -1, 0, -1 }),
                "Negative corner missing");

        require(contains(plan.toLoad, ve::ChunkPosition{ 1, 0, 1 }),
                "Positive corner missing");

        std::cout << "[PASS] Initial 3x3 window\n";
    }


    void testLoadedAndUnloaded() {
        ve::VoxelStorage storage{};

        storage.getOrCreateChunk(ve::ChunkPosition{ 0, 0, 0 });
        storage.getOrCreateChunk(ve::ChunkPosition{ -1, 0, -1 });
        storage.getOrCreateChunk(ve::ChunkPosition{ 5, 0, 5 });
        storage.getOrCreateChunk(ve::ChunkPosition{ 5, 1, 5 });

        const std::size_t before = storage.chunkCount();
        const std::size_t dirtyBefore = storage.dirtyChunkCount();

        const ve::ChunkWindowPlan plan = ve::planHorizontalChunkWindow(
            ve::Vec3{ 8.5f, 0.0f, 8.5f }, 1, storage
        );

        require(plan.toLoad.size() == 7 && plan.toUnload.size() == 2,
                "Window difference counts are incorrect");

        require(contains(plan.toUnload, ve::ChunkPosition{ 5, 0, 5 }),
                "Distant horizontal Chunk should unload");

        require(contains(plan.toUnload, ve::ChunkPosition{ 5, 1, 5 }),
                "Distant vertical Chunk must also unload");

        require(storage.chunkCount() == before && storage.dirtyChunkCount() == dirtyBefore,
                "Planning must not mutate storage");

        std::cout << "[PASS] Set difference and no mutation\n";
    }


    void testNegativeCoordinates() {
        const ve::VoxelStorage storage{};

        const ve::ChunkWindowPlan negative = ve::planHorizontalChunkWindow(
            ve::Vec3{ -0.25f, 5.0f, -0.25f }, 0, storage
        );

        const ve::ChunkWindowPlan positive = ve::planHorizontalChunkWindow(
            ve::Vec3{ 16.0f, 5.0f, 16.0f }, 0, storage
        );

        const ve::ChunkWindowPlan boundary = ve::planHorizontalChunkWindow(
            ve::Vec3{ 15.5f, 5.0f, 15.5f }, 0, storage
        );

        require(contains(negative.toLoad, ve::ChunkPosition{ -1, 0, -1 }),
                "Negative fractional coordinate maps to wrong Chunk");

        require(contains(positive.toLoad, ve::ChunkPosition{ 1, 0, 1 }),
                "Positive boundary maps to wrong Chunk");

        require(contains(boundary.toLoad, ve::ChunkPosition{ 0, 0, 0 }),
                "Pre-boundary position maps to wrong Chunk");

        std::cout << "[PASS] Positive and negative boundaries\n";
    }


    void testInvalidRadius() {
        const ve::VoxelStorage storage{};
        bool rejected = false;

        try {
            ve::planHorizontalChunkWindow(ve::Vec3{}, -1, storage);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected, "Negative radius must be rejected");

        std::cout << "[PASS] Invalid radius\n";
    }
}


int main() {
    try {
        testInitialWindow();
        testLoadedAndUnloaded();
        testNegativeCoordinates();
        testInvalidRadius();

        std::cout << "All ChunkWindowPlanner unit tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}