#include <cstddef>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "voxel/streaming/ChunkCacheStatistics.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    void testEmptyCache() {
        const ve::ChunkDeltaCache cache{};
        const ve::ChunkCacheStatistics stats = ve::calculateChunkCacheStatistics(cache);

        require(stats.cachedChunks == 0 && stats.changedBlocks == 0,
                "Empty cache counts must be zero");

        require(stats.usedDeltaBytes == 0 && stats.reservedDeltaBytes == 0,
                "Empty delta bytes must be zero");

        require(stats.accountedDeltaBytes() == 0 && stats.fullMapValueBytes == 0,
                "Empty storage bytes must be zero");

        std::cout << "[PASS] Empty cache statistics\n";
    }


    void testChangedBlockCountsAndCapacity() {
        ve::ChunkDeltaCache cache{};

        ve::ChunkDelta first{};
        first.blocks.reserve(32);
        first.blocks.push_back(ve::BlockDelta{ 2, ve::BlockId::Air });
        first.blocks.push_back(ve::BlockDelta{ 40, ve::BlockId::Stone });
        cache.emplace(ve::ChunkPosition{ 0, 0, 0 }, std::move(first));

        ve::ChunkDelta second{};
        second.blocks.reserve(8);
        second.blocks.push_back(ve::BlockDelta{ 17, ve::BlockId::Grass });
        cache.emplace(ve::ChunkPosition{ 1, 0, 0 }, std::move(second));

        const ve::ChunkCacheStatistics stats = ve::calculateChunkCacheStatistics(cache);

        require(stats.cachedChunks == 2 && stats.changedBlocks == 3,
                "Cache statistics must count records");

        require(stats.usedDeltaBytes == 3 * sizeof(ve::BlockDelta),
                "Used payload bytes are incorrect");

        require(stats.reservedDeltaBytes == 40 * sizeof(ve::BlockDelta),
                "Reserved payload bytes are incorrect");

        require(stats.reservedDeltaBytes > stats.usedDeltaBytes,
                "Capacity should exceed size");

        require(stats.deltaMapValueBytes == 2 * sizeof(ve::ChunkDeltaCache::value_type),
                "Map values incorrectly measured");

        require(stats.accountedDeltaBytes() == stats.deltaMapValueBytes + stats.reservedDeltaBytes,
                "Accounted memory total is incorrect");

        std::cout << "[PASS] Changed blocks and vector capacity\n";
    }


    void testBaselineComparison() {
        ve::ChunkDeltaCache cache{};
        ve::ChunkDelta delta{};
        delta.blocks.push_back(ve::BlockDelta{ 1, ve::BlockId::Dirt });
        cache.emplace(ve::ChunkPosition{ -1, 1, 0 }, std::move(delta));

        const ve::ChunkCacheStatistics stats = ve::calculateChunkCacheStatistics(cache);

        require(stats.fullMapValueBytes > stats.accountedDeltaBytes(),
                "One changed block should cost less than storing an entire Chunk");

        require(stats.fullMapValueBytes >= sizeof(ve::Chunk),
                "Full Chunk comparison must include Chunk storage");

        std::cout << "[PASS] Delta cache vs full Chunk comparison\n";
    }
}


int main() {
    try {
        testEmptyCache();
        testChangedBlockCountsAndCapacity();
        testBaselineComparison();

        std::cout << "All ChunkCacheStatistics unit tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}