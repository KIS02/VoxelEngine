#pragma once

#include "assets/mesh/MeshData.h"



namespace ve
{
    class Chunk;
    class VoxelStorage;

    struct ChunkPosition;



    class ChunkMeshBuilder {
        public:
            static MeshData create( const Chunk& chunk );

            static MeshData create( const VoxelStorage& world, const ChunkPosition& position );
    };
}