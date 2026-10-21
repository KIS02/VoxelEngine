#pragma once

#include <array>
#include <cstddef>

#include "voxel/block/BlockId.h"



namespace ve
{
    class Chunk {
        public:
            static constexpr int Size = 16;
            static constexpr std::size_t BlockCount = Size * Size * Size;


        private:
            std::array<BlockId, BlockCount> blocks_{};


        public:
            Chunk() = default;


            static constexpr bool isInside( int x, int y, int z ) {
                return x >= 0 && x < Size &&
                       y >= 0 && y < Size &&
                       z >= 0 && z < Size;
            }


            BlockId blockAt( int x, int y, int z ) const;

            void setBlock( int x, int y, int z, BlockId type );

            void fill( BlockId type );

            std::size_t solidBlockCount() const;


        private:
            static std::size_t indexOf( int x, int y, int z );
    };
}