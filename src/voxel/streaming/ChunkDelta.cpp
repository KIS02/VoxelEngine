#include "voxel/streaming/ChunkDelta.h"

#include <cstddef>
#include <limits>
#include <stdexcept>


namespace ve
{
    namespace
    {
        static_assert(Chunk::BlockCount - 1 <= std::numeric_limits<std::uint16_t>::max());
    }


    ChunkDelta createChunkDelta( const Chunk& current, const Chunk& baseline ) {
        ChunkDelta delta{};

        for (int y = 0; y < Chunk::Size; ++y) {
            for (int z = 0; z < Chunk::Size; ++z) {
                for (int x = 0; x < Chunk::Size; ++x) {
                    const BlockId type = current.blockAt(x, y, z);

                    if (type == baseline.blockAt(x, y, z))
                        continue;

                    const int index = x + Chunk::Size * (z + Chunk::Size * y);
                    delta.blocks.push_back(BlockDelta{ static_cast<std::uint16_t>(index), type });
                }
            }
        }

        return delta;
    }


    void applyChunkDelta( Chunk& destination, const ChunkDelta& delta ) {
        for (const BlockDelta& change : delta.blocks) {
            if (change.index >= Chunk::BlockCount)
                throw std::out_of_range("Block Delta index is outside Chunk range");

            const int index = static_cast<int>(change.index);
            const int x = index % Chunk::Size;
            const int z = (index / Chunk::Size) % Chunk::Size;
            const int y = index / (Chunk::Size * Chunk::Size);

            destination.setBlock(x, y, z, change.type);
        }
    }
}