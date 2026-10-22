#pragma once

#include <cstddef>
#include <filesystem>
#include <optional>

#include "math/Vec3.h"
#include "scene/voxel/VoxelWorld.h"
#include "voxel/generation/TerrainGenerator.h"
#include "voxel/streaming/ChunkCacheStatistics.h"


namespace ve
{
    struct ChunkStreamingResult
    {
        bool changed = false;
        std::size_t generated = 0;
        std::size_t restored = 0;
        std::size_t unloaded = 0;
    };


    class ChunkStreamingController {
        private:
            VoxelWorld& world_;
            TerrainGenerator generator_;
            int radius_ = 1;
            std::optional<ChunkPosition> lastCenter_{};
            ChunkDeltaCache cachedDeltas_{};


        public:
            ChunkStreamingController( VoxelWorld& world, TerrainGenerationConfig config, int radius );


            ChunkStreamingResult update( const Vec3& cameraPosition );

            std::size_t cachedChunkCount() const;

            ChunkCacheStatistics cacheStatistics() const;

            bool loadWorld( const std::filesystem::path& path );

            void saveWorld( const std::filesystem::path& path ) const;
    };
}