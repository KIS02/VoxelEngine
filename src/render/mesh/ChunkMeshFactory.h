#pragma once

#include "render/MeshData.h"



namespace ve
{
    class Chunk;
    class World;

    struct ChunkPosition;



    class ChunkMeshFactory {
        public:
            static MeshData create( const Chunk& chunk );

            static MeshData create( const World& world, const ChunkPosition& position );
    };
}