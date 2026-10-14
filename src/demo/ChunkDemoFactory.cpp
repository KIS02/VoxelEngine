#include "demo/ChunkDemoFactory.h"

#include "world/BlockType.h"



namespace ve
{
    Chunk createDemoChunk() {
        Chunk chunk{};



        // Generate a Simple Stepped Terrain

        for (int z = 0; z < Chunk::Size; ++z) {
            for (int x = 0; x < Chunk::Size; ++x) {
                const int height = 2 + ((x / 4 + z / 4) % 3);



                // Fill the Column from Bottom to Surface

                for (int y = 0; y < height; ++y) {
                    BlockType type = BlockType::Stone;


                    if (y == height - 1) {
                        type = BlockType::Grass;
                    }
                    else if (y == height - 2) {
                        type = BlockType::Dirt;
                    }


                    chunk.setBlock(x, y, z, type);
                }
            }
        }


        return chunk;
    }
}