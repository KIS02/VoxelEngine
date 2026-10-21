#include "voxel/chunk/Chunk.h"

#include <stdexcept>



namespace ve
{
    std::size_t Chunk::indexOf( int x, int y, int z ) {
        if (!isInside(x, y, z)) {
            throw std::out_of_range("Chunk block coordinates are out of range");
        }


        return static_cast<std::size_t>(x + Size * (z + Size * y));
    }



    BlockId Chunk::blockAt( int x, int y, int z ) const {
        return blocks_[indexOf(x, y, z)];
    }



    void Chunk::setBlock( int x, int y, int z, BlockId type ) {
        blocks_[indexOf(x, y, z)] = type;
    }



    void Chunk::fill( BlockId type ) {
        blocks_.fill(type);
    }



    std::size_t Chunk::solidBlockCount() const {
        std::size_t count = 0;


        for (BlockId type : blocks_) {
            if (isSolid(type)) {
                ++count;
            }
        }


        return count;
    }
}