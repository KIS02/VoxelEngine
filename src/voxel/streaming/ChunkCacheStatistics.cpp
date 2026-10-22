#include "voxel/streaming/ChunkCacheStatistics.h"


namespace ve
{
    ChunkCacheStatistics calculateChunkCacheStatistics( const ChunkDeltaCache& cache ) {
        using FullChunkCache = std::map<ChunkPosition, Chunk, ChunkPositionLess>;

        ChunkCacheStatistics statistics{};
        statistics.cachedChunks = cache.size();
        statistics.deltaMapValueBytes = cache.size() * sizeof(ChunkDeltaCache::value_type);
        statistics.fullMapValueBytes = cache.size() * sizeof(FullChunkCache::value_type);

        for (const auto& entry : cache) {
            const std::vector<BlockDelta>& blocks = entry.second.blocks;

            statistics.changedBlocks += blocks.size();
            statistics.usedDeltaBytes += blocks.size() * sizeof(BlockDelta);
            statistics.reservedDeltaBytes += blocks.capacity() * sizeof(BlockDelta);
        }

        return statistics;
    }
}