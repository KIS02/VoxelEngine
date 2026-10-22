#pragma once

#include <cstdint>
#include <vector>

#include "voxel/chunk/Chunk.h"


namespace ve
{
    struct BlockDelta
    {
        std::uint16_t index = 0;
        BlockId type = BlockId::Air;
    };


    struct ChunkDelta
    {
        std::vector<BlockDelta> blocks{};
    };


    ChunkDelta createChunkDelta( const Chunk& current, const Chunk& baseline );

    void applyChunkDelta( Chunk& destination, const ChunkDelta& delta );
}