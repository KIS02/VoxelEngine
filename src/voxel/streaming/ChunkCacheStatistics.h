#pragma once

#include <cstddef>
#include <map>

#include "voxel/chunk/VoxelStorage.h"
#include "voxel/streaming/ChunkDelta.h"


namespace ve
{
    using ChunkDeltaCache = std::map<ChunkPosition, ChunkDelta, ChunkPositionLess>;


    struct ChunkCacheStatistics
    {
        std::size_t cachedChunks = 0;
        std::size_t changedBlocks = 0;
        std::size_t usedDeltaBytes = 0;
        std::size_t reservedDeltaBytes = 0;
        std::size_t deltaMapValueBytes = 0;
        std::size_t fullMapValueBytes = 0;


        std::size_t accountedDeltaBytes() const {
            return deltaMapValueBytes + reservedDeltaBytes;
        }
    };


    ChunkCacheStatistics calculateChunkCacheStatistics( const ChunkDeltaCache& cache );
}