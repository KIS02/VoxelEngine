#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "assets/mesh/MeshIdGenerator.h"
#include "scene/voxel/VoxelWorld.h"
#include "voxel/save/WorldSaveFile.h"
#include "voxel/streaming/ChunkStreamingController.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    ve::TerrainGenerationConfig sampleConfig() {
        ve::TerrainGenerationConfig config{};
        config.seed = 4242;
        config.surfaceY = 8;
        config.dirtDepth = 2;
        config.heightAmplitude = 0.0;
        return config;
    }


    void testFileRoundTripAndReplacement( const std::filesystem::path& path ) {
        ve::WorldSaveData data{};
        data.terrain = sampleConfig();

        ve::ChunkDelta delta{};
        delta.blocks.push_back(ve::BlockDelta{ 2098, ve::BlockId::Air });
        delta.blocks.push_back(ve::BlockDelta{ 255, ve::BlockId::Stone });

        data.deltas.emplace(ve::ChunkPosition{ -1, 1, 2 }, delta);

        ve::writeWorldSaveFile(path, data);
        const ve::WorldSaveData loaded = ve::readWorldSaveFile(path);

        require(loaded.terrain.seed == 4242 && loaded.terrain.surfaceY == 8,
                "Seed and surface height must survive saving");

        require(loaded.deltas.size() == 1 && loaded.deltas.begin()->second.blocks.size() == 2,
                "Signed Chunk coordinates and Delta values must survive saving");

        require(loaded.deltas.begin()->first.x == -1 && loaded.deltas.begin()->first.y == 1,
                "Negative and vertical Chunk coordinates must survive saving");

        data.deltas.clear();
        ve::writeWorldSaveFile(path, data);

        require(ve::readWorldSaveFile(path).deltas.empty(),
                "Replacing save must discard old Delta records");

        std::cout << "[PASS] Binary round trip and safe replacement\n";
    }


    void testResidentAndCachedDeltaPersistence( const std::filesystem::path& path ) {
        const ve::WorldBlockPosition editedGround{ 1, 8, 1 };
        const ve::WorldBlockPosition upper{ 1, 16, 1 };
        const ve::WorldBlockPosition lower{ 1, -1, 1 };
        const ve::WorldBlockPosition editedRemote{ 33, 8, 1 };

        {
            ve::MeshIdGenerator ids{};
            ve::VoxelWorld world(ids);
            ve::ChunkStreamingController streaming(world, sampleConfig(), 1);

            require(!streaming.loadWorld(path), "Missing save should start a new world");

            streaming.update(ve::Vec3{ 8.5f, 20.0f, 8.5f });
            world.blocks().setBlock(editedGround, ve::BlockId::Air);
            world.blocks().setBlock(upper, ve::BlockId::Stone);
            world.blocks().setBlock(lower, ve::BlockId::Dirt);

            streaming.update(ve::Vec3{ 40.5f, 20.0f, 8.5f });
            world.blocks().setBlock(editedRemote, ve::BlockId::Air);

            require(streaming.cachedChunkCount() == 3,
                    "Three edited Chunks should be held in cold cache");

            streaming.saveWorld(path);
        }

        const ve::WorldSaveData stored = ve::readWorldSaveFile(path);

        require(stored.deltas.size() == 4,
                "Save must include both cold and resident modifications");

        {
            ve::MeshIdGenerator ids{};
            ve::VoxelWorld world(ids);
            ve::ChunkStreamingController streaming(world, sampleConfig(), 1);

            require(streaming.loadWorld(path), "Existing save should load");
            require(streaming.cachedChunkCount() == 4,
                    "Save must restore all cold records before first update");

            streaming.update(ve::Vec3{ 8.5f, 20.0f, 8.5f });

            require(world.blocks().blockAt(editedGround) == ve::BlockId::Air,
                    "Ground edit must survive application restart");

            require(world.blocks().blockAt(upper) == ve::BlockId::Stone,
                    "Positive vertical Chunk must survive application restart");

            require(world.blocks().blockAt(lower) == ve::BlockId::Dirt,
                    "Negative vertical Chunk must survive application restart");

            streaming.update(ve::Vec3{ 40.5f, 20.0f, 8.5f });

            require(world.blocks().blockAt(editedRemote) == ve::BlockId::Air,
                    "Resident-at-save edit must survive application restart");
        }

        std::cout << "[PASS] Cold, resident, positive/negative Y edits persist across restart\n";
    }


    void testSettingsMismatchAndCorruptFile( const std::filesystem::path& path,
                                              const std::filesystem::path& corrupt ) {
        ve::MeshIdGenerator ids{};
        ve::VoxelWorld world(ids);

        ve::TerrainGenerationConfig wrong = sampleConfig();
        wrong.seed = 99;

        ve::ChunkStreamingController mismatch(world, wrong, 1);
        bool rejected = false;

        try {
            mismatch.loadWorld(path);
        } catch (const std::runtime_error&) {
            rejected = true;
        }

        require(rejected && world.blocks().chunkCount() == 0,
                "Mismatched Seed must not change world state");

        std::filesystem::copy_file(path, corrupt, std::filesystem::copy_options::overwrite_existing);

        {
            std::fstream file(corrupt, std::ios::binary | std::ios::in | std::ios::out);
            require(static_cast<bool>(file), "Unable to open test-corrupted file");

            file.seekp(16);
            file.put('X');
        }

        rejected = false;

        try {
            ve::readWorldSaveFile(corrupt);
        } catch (const std::runtime_error&) {
            rejected = true;
        }

        require(rejected, "Corrupted World Save must be rejected");

        require(ve::readWorldSaveFile(path).deltas.size() == 4,
                "Corruption of separate file must not affect valid original");

        std::cout << "[PASS] Seed mismatch and checksum corruption rejected\n";
    }


    void testInvalidDeltaAndLoadTiming( const std::filesystem::path& path ) {
        ve::WorldSaveData invalid{};
        invalid.terrain = sampleConfig();

        ve::ChunkDelta delta{};
        delta.blocks.push_back(ve::BlockDelta{ 4096, ve::BlockId::Stone });
        invalid.deltas.emplace(ve::ChunkPosition{ 0, 0, 0 }, delta);

        bool rejected = false;

        try {
            ve::writeWorldSaveFile(path, invalid);
        } catch (const std::runtime_error&) {
            rejected = true;
        }

        require(rejected, "Invalid Block Delta index must be rejected before writing");

        require(ve::readWorldSaveFile(path).deltas.size() == 4,
                "Invalid save attempt must preserve existing file");

        ve::MeshIdGenerator ids{};
        ve::VoxelWorld world(ids);
        ve::ChunkStreamingController streaming(world, sampleConfig(), 1);

        streaming.update(ve::Vec3{ 8.5f, 20.0f, 8.5f });
        rejected = false;

        try {
            streaming.loadWorld(path);
        } catch (const std::logic_error&) {
            rejected = true;
        }

        require(rejected, "Loading after initialization must be rejected");

        std::cout << "[PASS] Invalid Delta rejected and late loading prohibited\n";
    }
}


int main() {
    try {
        const std::filesystem::path directory = "build/unit/world_save_tests";
        std::filesystem::create_directories(directory);

        const std::filesystem::path save = directory / "test_world.vew";
        const std::filesystem::path corrupt = directory / "corrupt_world.vew";

        std::filesystem::remove(save);
        std::filesystem::remove(corrupt);

        testFileRoundTripAndReplacement(save);

        std::filesystem::remove(save);

        testResidentAndCachedDeltaPersistence(save);
        testSettingsMismatchAndCorruptFile(save, corrupt);
        testInvalidDeltaAndLoadTiming(save);

        std::filesystem::remove(save);
        std::filesystem::remove(corrupt);

        std::cout << "All WorldSaveLoad unit tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}