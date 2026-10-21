#include "scene/demo/DemoChunkGenerator.h"

#include "voxel/block/BlockId.h"



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
                    BlockId type = BlockId::Stone;


                    if (y == height - 1) {
                        type = BlockId::Grass;
                    }
                    else if (y == height - 2) {
                        type = BlockId::Dirt;
                    }


                    chunk.setBlock(x, y, z, type);
                }
            }
        }


        return chunk;
    }
}