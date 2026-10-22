#include <algorithm>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "assets/mesh/MeshIdGenerator.h"
#include "scene/voxel/VoxelWorld.h"
#include "voxel/streaming/ChunkStreamingController.h"
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


    void testPlannerUnloadsEntireColumns() {
        ve::VoxelStorage storage{};

        storage.setChunk(ve::ChunkPosition{ 0, 0, 0 }, ve::Chunk{});
        storage.setChunk(ve::ChunkPosition{ 0, 1, 0 }, ve::Chunk{});
        storage.setChunk(ve::ChunkPosition{ 0, -1, 0 }, ve::Chunk{});

        storage.setChunk(ve::ChunkPosition{ 4, 0, 0 }, ve::Chunk{});
        storage.setChunk(ve::ChunkPosition{ 4, 1, 0 }, ve::Chunk{});
        storage.setChunk(ve::ChunkPosition{ 4, -2, 0 }, ve::Chunk{});

        const auto plan = ve::planHorizontalChunkWindow(ve::Vec3{ 0.5f, 10.0f, 0.5f }, 1, storage);

        require(plan.toLoad.size() == 8,
                "Existing base Chunk must count as loaded");

        require(plan.toUnload.size() == 3,
                "All distant vertical Chunks must unload");

        require(contains(plan.toUnload, ve::ChunkPosition{ 4, 0, 0 }),
                "Distant Y0 missing");

        require(contains(plan.toUnload, ve::ChunkPosition{ 4, 1, 0 }),
                "Distant Y+1 missing");

        require(contains(plan.toUnload, ve::ChunkPosition{ 4, -2, 0 }),
                "Distant negative Y missing");

        require(!contains(plan.toUnload, ve::ChunkPosition{ 0, 1, 0 }),
                "Nearby positive Y must stay");

        require(!contains(plan.toUnload, ve::ChunkPosition{ 0, -1, 0 }),
                "Nearby negative Y must stay");

        std::cout << "[PASS] Entire distant X/Z columns selected\n";
    }


    void testTowerRestoration() {
        ve::MeshIdGenerator ids{};
        ve::VoxelWorld world(ids);

        ve::ChunkStreamingController controller(world, ve::TerrainGenerationConfig{ 1234, 8, 2, 12.0 }, 1);

        const auto initial = controller.update(ve::Vec3{ 8.5f, 20.0f, 8.5f });

        require(initial.generated == 9 && initial.restored == 0,
                "Initial window must generate nine base Chunks");

        world.rebuildDirtyMeshes();


        const ve::WorldBlockPosition upperA{ 1, 16, 1 };
        const ve::WorldBlockPosition upperB{ 1, 17, 1 };
        const ve::WorldBlockPosition lower{ 1, -1, 1 };
        const ve::WorldBlockPosition changedBase{ 1, 0, 1 };

        world.blocks().setBlock(upperA, ve::BlockId::Stone);
        world.blocks().setBlock(upperB, ve::BlockId::Grass);
        world.blocks().setBlock(lower, ve::BlockId::Dirt);
        world.blocks().setBlock(changedBase, ve::BlockId::Air);

        world.rebuildDirtyMeshes();

        require(world.blocks().chunkCount() == 11,
                "Two additional Y layers expected");


        const auto moved = controller.update(ve::Vec3{ 56.0f, 20.0f, 8.5f });

        require(moved.unloaded == 11 && moved.generated == 9 && moved.restored == 0,
                "All eleven vertical Chunks must unload on distant move");

        require(controller.cachedChunkCount() == 11 && world.blocks().chunkCount() == 9,
                "Resident and cache counts after move are wrong");

        require(world.blocks().findChunk(ve::ChunkPosition{ 0, 1, 0 }) == nullptr,
                "Upper tower Chunk must not remain loaded");

        require(world.blocks().findChunk(ve::ChunkPosition{ 0, -1, 0 }) == nullptr,
                "Lower Chunk must not remain loaded");

        world.rebuildDirtyMeshes();


        const auto returned = controller.update(ve::Vec3{ 8.5f, 20.0f, 8.5f });

        require(returned.restored == 11 && returned.generated == 0 && returned.unloaded == 9,
                "Returning must restore ground and all cached vertical Chunks");

        require(world.blocks().blockAt(upperA) == ve::BlockId::Stone,
                "Upper Y16 block not restored");

        require(world.blocks().blockAt(upperB) == ve::BlockId::Grass,
                "Upper Y17 block not restored");

        require(world.blocks().blockAt(lower) == ve::BlockId::Dirt,
                "Lower block not restored");

        require(world.blocks().blockAt(changedBase) == ve::BlockId::Air,
                "Edited base not restored");

        require(world.blocks().chunkCount() == 11,
                "Returned window must contain full tower");

        world.rebuildDirtyMeshes();

        require(world.blocks().dirtyChunkCount() == 0,
                "Restoration rebuild must clear Dirty state");

        std::cout << "[PASS] Full tower and edits restored on return\n";
    }
}


int main() {
    try {
        testPlannerUnloadsEntireColumns();
        testTowerRestoration();

        std::cout << "All VerticalStreaming tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}