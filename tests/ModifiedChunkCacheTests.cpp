#include <exception>
#include <iostream>
#include <stdexcept>

#include "assets/mesh/MeshIdGenerator.h"
#include "scene/voxel/VoxelWorld.h"
#include "voxel/streaming/ChunkStreamingController.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    void testPristineChunksAreRegenerated() {
        ve::MeshIdGenerator ids{};
        ve::VoxelWorld world(ids);
        ve::ChunkStreamingController controller(world, ve::TerrainGenerationConfig{ 1234, 8, 2, 0.0 }, 1);

        const auto first = controller.update(ve::Vec3{ 8.5f, 20.0f, 8.5f });

        require(first.generated == 9 && world.blocks().chunkCount() == 9,
                "Initial window must generate nine Chunks");

        const auto away = controller.update(ve::Vec3{ 56.0f, 20.0f, 8.5f });

        require(away.unloaded == 9 && away.generated == 9 && controller.cachedChunkCount() == 0,
                "Unmodified Chunks must not remain in cache");

        const auto back = controller.update(ve::Vec3{ 8.5f, 20.0f, 8.5f });

        require(back.generated == 9 && back.restored == 0 && controller.cachedChunkCount() == 0,
                "Pristine Chunks must be regenerated from Seed");

        require(world.blocks().blockAt(ve::WorldBlockPosition{ 1, 8, 1 }) == ve::BlockId::Grass,
                "Regenerated ground must preserve original terrain");

        std::cout << "[PASS] Pristine Chunks are regenerated without caching\n";
    }


    void testModifiedGroundIsRestored() {
        ve::MeshIdGenerator ids{};
        ve::VoxelWorld world(ids);
        ve::ChunkStreamingController controller(world, ve::TerrainGenerationConfig{ 1234, 8, 2, 0.0 }, 1);

        controller.update(ve::Vec3{ 8.5f, 20.0f, 8.5f });

        const ve::WorldBlockPosition target{ 1, 8, 1 };

        require(world.blocks().blockAt(target) == ve::BlockId::Grass,
                "Unexpected baseline");

        world.blocks().setBlock(target, ve::BlockId::Air);

        controller.update(ve::Vec3{ 56.0f, 20.0f, 8.5f });

        require(controller.cachedChunkCount() == 1,
                "Only the edited ground Chunk should be cached");

        const auto back = controller.update(ve::Vec3{ 8.5f, 20.0f, 8.5f });

        require(back.restored == 1 && back.generated == 8,
                "One edited Chunk must restore and eight pristine Chunks regenerate");

        require(world.blocks().blockAt(target) == ve::BlockId::Air,
                "Edited block must remain Air");

        std::cout << "[PASS] Modified ground is restored\n";
    }


    void testRevertedEditIsNotCached() {
        ve::MeshIdGenerator ids{};
        ve::VoxelWorld world(ids);
        ve::ChunkStreamingController controller(world, ve::TerrainGenerationConfig{ 1234, 8, 2, 0.0 }, 1);

        controller.update(ve::Vec3{ 8.5f, 20.0f, 8.5f });

        const ve::WorldBlockPosition target{ 1, 8, 1 };

        world.blocks().setBlock(target, ve::BlockId::Air);
        world.blocks().setBlock(target, ve::BlockId::Grass);

        controller.update(ve::Vec3{ 56.0f, 20.0f, 8.5f });

        require(controller.cachedChunkCount() == 0,
                "A reverted edit must not cause a saved Chunk");

        std::cout << "[PASS] Reverted edits require no cache\n";
    }


    void testVerticalEditsAndEmptyChunks() {
        ve::MeshIdGenerator ids{};
        ve::VoxelWorld world(ids);
        ve::ChunkStreamingController controller(world, ve::TerrainGenerationConfig{ 1234, 8, 2, 0.0 }, 1);

        controller.update(ve::Vec3{ 8.5f, 20.0f, 8.5f });

        const ve::WorldBlockPosition upper{ 1, 16, 1 };
        const ve::WorldBlockPosition lower{ 1, -1, 1 };

        world.blocks().setBlock(upper, ve::BlockId::Stone);
        world.blocks().setBlock(lower, ve::BlockId::Dirt);
        world.blocks().getOrCreateChunk(ve::ChunkPosition{ 0, 2, 0 });

        require(world.blocks().chunkCount() == 12,
                "Expected 9 ground and 3 vertical Chunks");

        const auto away = controller.update(ve::Vec3{ 56.0f, 20.0f, 8.5f });

        require(away.unloaded == 12 && controller.cachedChunkCount() == 2,
                "Only modified upper and lower vertical Chunks should be cached");

        const auto back = controller.update(ve::Vec3{ 8.5f, 20.0f, 8.5f });

        require(back.restored == 2 && back.generated == 9,
                "Two modified vertical Chunks should restore; nine ground Chunks regenerate");

        require(world.blocks().blockAt(upper) == ve::BlockId::Stone &&
                world.blocks().blockAt(lower) == ve::BlockId::Dirt,
                "Both edited vertical blocks must survive");

        require(world.blocks().findChunk(ve::ChunkPosition{ 0, 2, 0 }) == nullptr,
                "Unused empty upper Chunk need not be recreated");

        std::cout << "[PASS] Vertical modified Chunks survive and empty ones are discarded\n";
    }
}


int main() {
    try {
        testPristineChunksAreRegenerated();
        testModifiedGroundIsRestored();
        testRevertedEditIsNotCached();
        testVerticalEditsAndEmptyChunks();

        std::cout << "All ModifiedChunkCache unit tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}