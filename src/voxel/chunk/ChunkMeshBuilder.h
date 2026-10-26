#pragma once

#include <map>

#include "assets/block/BlockDefinition.h"
#include "assets/mesh/MeshData.h"


namespace ve
{
    class Chunk;
    class VoxelStorage;
    struct ChunkPosition;

    using MaterialMeshes = std::map<BlockMaterialKey, MeshData>;


    class ChunkMeshBuilder {
        public:
            static MeshData create( const Chunk& chunk );
            static MeshData create( const VoxelStorage& world, const ChunkPosition& position );

            static MaterialMeshes createMaterialMeshes( const Chunk& chunk );
            static MaterialMeshes createMaterialMeshes( const VoxelStorage& world,
                                                        const ChunkPosition& position );
    };
}