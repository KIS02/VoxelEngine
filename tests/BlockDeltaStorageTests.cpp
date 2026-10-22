#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>

#include "assets/mesh/MeshIdGenerator.h"
#include "scene/voxel/VoxelWorld.h"
#include "voxel/streaming/ChunkDelta.h"
#include "voxel/streaming/ChunkStreamingController.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    void testDeltaEncodingAndApplication() {
        ve::Chunk baseline{};
        baseline.setBlock(0, 0, 0, ve::BlockId::Stone);

        ve::Chunk modified = baseline;
        modified.setBlock(0, 0, 0, ve::BlockId::Air);
        modified.setBlock(15, 15, 15, ve::BlockId::Grass);
        modified.setBlock(5, 7, 9, ve::BlockId::Dirt);

        const ve::ChunkDelta delta = ve::createChunkDelta(modified, baseline);

        require(delta.blocks.size() == 3,
                "Expected exactly three changed blocks");

        ve::Chunk restored = baseline;
        ve::applyChunkDelta(restored, delta);

        for (int y = 0; y < ve::Chunk::Size; ++y) {
            for (int z = 0; z < ve::Chunk::Size; ++z) {
                for (int x = 0; x < ve::Chunk::Size; ++x)
                    require(restored.blockAt(x, y, z) == modified.blockAt(x, y, z),
                            "Block Delta roundtrip must reproduce modified Chunk");
            }
        }

        require(ve::createChunkDelta(baseline, baseline).blocks.empty(),
                "Pristine Chunk must produce no Delta");

        std::cout << "[PASS] Delta encoding and application\n";
    }


    void testInvalidDeltaIndex() {
        ve::Chunk destination{};
        ve::ChunkDelta invalid{};

        invalid.blocks.push_back(ve::BlockDelta{ 4096, ve::BlockId::Stone });

        bool rejected = false;

        try {
            ve::applyChunkDelta(destination, invalid);
        } catch (const std::out_of_range&) {
            rejected = true;
        }

        require(rejected, "Out-of-range Delta index must be rejected");

        std::cout << "[PASS] Invalid Delta index\n";
    }


    void testPristineRegeneration() {
        ve::MeshIdGenerator ids{};
        ve::VoxelWorld world(ids);
        ve::ChunkStreamingController controller(world, ve::TerrainGenerationConfig{ 1234, 8, 2, 0.0 }, 1);

        require(controller.update(ve::Vec3{ 8.5f, 20.0f, 8.5f }).generated == 9,
                "Initial window must generate nine Chunks");

        const auto moved = controller.update(ve::Vec3{ 56.0f, 20.0f, 8.5f });

        require(moved.unloaded == 9 && controller.cachedChunkCount() == 0,
                "Pristine Chunks should not be cached");

        const auto returned = controller.update(ve::Vec3{ 8.5f, 20.0f, 8.5f });

        require(returned.generated == 9 && returned.restored == 0,
                "Pristine Chunks should regenerate rather than restore");

        require(world.blocks().blockAt(ve::WorldBlockPosition{ 1, 8, 1 }) == ve::BlockId::Grass,
                "Pristine regenerated Chunk should match terrain baseline");

        std::cout << "[PASS] Pristine regeneration\n";
    }


    void testEditedAndRevertedGround() {
        ve::MeshIdGenerator ids{};
        ve::VoxelWorld world(ids);
        ve::ChunkStreamingController controller(world, ve::TerrainGenerationConfig{ 1234, 8, 2, 0.0 }, 1);

        controller.update(ve::Vec3{ 8.5f, 20.0f, 8.5f });

        const ve::WorldBlockPosition target{ 1, 8, 1 };

        world.blocks().setBlock(target, ve::BlockId::Air);

        controller.update(ve::Vec3{ 56.0f, 20.0f, 8.5f });

        require(controller.cachedChunkCount() == 1,
                "Exactly one modified Chunk must be cached");

        const auto returned = controller.update(ve::Vec3{ 8.5f, 20.0f, 8.5f });

        require(returned.restored == 1 && returned.generated == 8,
                "Edited Chunk must restore while pristine Chunks regenerate");

        require(world.blocks().blockAt(target) == ve::BlockId::Air,
                "Edited Air block must be preserved");


        world.blocks().setBlock(target, ve::BlockId::Grass);

        controller.update(ve::Vec3{ 56.0f, 20.0f, 8.5f });

        require(controller.cachedChunkCount() == 0,
                "Reverted edit must not be cached");

        std::cout << "[PASS] Edited and reverted ground\n";
    }


    void testVerticalBlocksAndEmptyChunk() {
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
                "Nine ground and three vertical Chunks expected");

        const auto away = controller.update(ve::Vec3{ 56.0f, 20.0f, 8.5f });

        require(away.unloaded == 12 && controller.cachedChunkCount() == 2,
                "Only two edited vertical Chunks must be cached");

        const auto returned = controller.update(ve::Vec3{ 8.5f, 20.0f, 8.5f });

        require(returned.restored == 2 && returned.generated == 9,
                "Returning must restore two vertical Chunks and regenerate ground");

        require(world.blocks().blockAt(upper) == ve::BlockId::Stone &&
                world.blocks().blockAt(lower) == ve::BlockId::Dirt,
                "Upper and lower edits must survive streaming");

        require(world.blocks().findChunk(ve::ChunkPosition{ 0, 2, 0 }) == nullptr,
                "Pristine empty vertical Chunk must not be restored");

        std::cout << "[PASS] Vertical edits and empty-Chunk pruning\n";
    }
}


int main() {
    try {
        testDeltaEncodingAndApplication();
        testInvalidDeltaIndex();
        testPristineRegeneration();
        testEditedAndRevertedGround();
        testVerticalBlocksAndEmptyChunk();

        std::cout << "All BlockDeltaStorage unit tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}