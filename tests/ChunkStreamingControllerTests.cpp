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


    void testInitialGenerationAndSameChunk() {
        ve::MeshIdGenerator ids{};
        ve::VoxelWorld world(ids);
        const ve::TerrainGenerationConfig config{ 1234, 8, 2, 12.0 };
        ve::ChunkStreamingController controller(world, config, 1);

        const ve::ChunkStreamingResult first = controller.update(ve::Vec3{ 8.5f, 20.0f, 8.5f });

        require(first.changed && first.generated == 9 && first.restored == 0 && first.unloaded == 0,
                "Initial update must generate a 3x3 Chunk window");

        require(world.blocks().chunkCount() == 9 && controller.cachedChunkCount() == 0,
                "Initial resident and cached counts are incorrect");

        world.rebuildDirtyMeshes();

        require(world.blocks().dirtyChunkCount() == 0,
                "Rebuild should consume Dirty state");

        const ve::ChunkStreamingResult unchanged = controller.update(ve::Vec3{ 15.5f, 20.0f, 9.0f });

        require(!unchanged.changed && unchanged.generated == 0 && unchanged.unloaded == 0,
                "Camera movement within a Chunk must not recalculate the window");

        require(world.blocks().dirtyChunkCount() == 0,
                "An unchanged camera Chunk must not mark Meshes Dirty");

        std::cout << "[PASS] Initial generation and same-Chunk fast path\n";
    }


    void testCrossingAndEditedChunkRestoration() {
        ve::MeshIdGenerator ids{};
        ve::VoxelWorld world(ids);
        ve::ChunkStreamingController controller(world, ve::TerrainGenerationConfig{ 1234, 8, 2, 12.0 }, 1);

        controller.update(ve::Vec3{ 8.5f, 20.0f, 8.5f });
        world.rebuildDirtyMeshes();

        const ve::WorldBlockPosition editedBlock{ 1, 0, 1 };

        world.blocks().setBlock(editedBlock, ve::BlockId::Air);

        require(world.blocks().blockAt(editedBlock) == ve::BlockId::Air,
                "Block edit must succeed");

        const ve::ChunkStreamingResult moved = controller.update(ve::Vec3{ 56.0f, 20.0f, 8.5f });

        require(moved.changed && moved.generated == 9 && moved.unloaded == 9 && moved.restored == 0,
                "Moving three Chunks should exchange the whole window");

        require(world.blocks().chunkCount() == 9 && controller.cachedChunkCount() == 1,
                "Resident window must stay bounded while evicted Chunks are cached");

        require(world.blocks().findChunk(ve::ChunkPosition{ 0, 0, 0 }) == nullptr,
                "Evicted Chunk must be absent from resident storage");

        world.rebuildDirtyMeshes();

        const ve::ChunkStreamingResult returned = controller.update(ve::Vec3{ 8.5f, 20.0f, 8.5f });

        require(returned.changed && returned.restored == 1 &&
                returned.generated == 8 && returned.unloaded == 9,
                "Returning to the original window must restore cached Chunks");

        require(world.blocks().blockAt(editedBlock) == ve::BlockId::Air,
                "Unloaded edited Block must be preserved on return");

        require(world.blocks().chunkCount() == 9,
                "Resident window size must remain nine after restoration");

        std::cout << "[PASS] Cross-boundary exchange and edited Chunk restoration\n";
    }


    void testNegativeBoundaryAndInvalidRadius() {
        ve::MeshIdGenerator ids{};
        ve::VoxelWorld world(ids);
        ve::ChunkStreamingController controller(world, ve::TerrainGenerationConfig{ 1234, 8, 2, 12.0 }, 0);

        const ve::ChunkStreamingResult first = controller.update(ve::Vec3{ -0.25f, 20.0f, 0.0f });

        require(first.generated == 1 &&
                world.blocks().findChunk(ve::ChunkPosition{ -1, 0, 0 }) != nullptr,
                "Negative fractional coordinate should map to Chunk -1");

        const ve::ChunkStreamingResult second = controller.update(ve::Vec3{ 0.0f, 20.0f, 0.0f });

        require(second.generated == 1 && second.unloaded == 1,
                "Crossing zero should exchange the active Chunk");

        bool rejected = false;

        try {
            ve::ChunkStreamingController invalid(world, ve::TerrainGenerationConfig{}, -1);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected, "Negative radius must be rejected");

        std::cout << "[PASS] Negative coordinate boundary and invalid radius\n";
    }
}


int main() {
    try {
        testInitialGenerationAndSameChunk();
        testCrossingAndEditedChunkRestoration();
        testNegativeBoundaryAndInvalidRadius();

        std::cout << "All ChunkStreamingController unit tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}