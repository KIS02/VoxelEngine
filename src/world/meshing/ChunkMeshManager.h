#pragma once

#include <cstdint>
#include <map>

#include "assets/mesh/MeshData.h"
#include "render/MeshIDGenerator.h"
#include "render/api/RenderFrame.h"
#include "world/World.h"


namespace ve
{
    class MaterialData;


    class ChunkMeshManager {
        private:
            struct ChunkMeshEntry
            {
                MeshData mesh;
                std::uint64_t revision = 1;
                std::uint64_t id = 0;
            };


            MeshIdGenerator& meshIdGenerator_;
            std::map<ChunkPosition, ChunkMeshEntry, ChunkPositionLess> chunkMeshes_;


        public:
            explicit ChunkMeshManager( MeshIdGenerator& meshIdGenerator );

            std::uint64_t rebuildDirtyMeshes( World& world );
            void remove( const ChunkPosition& position );
            void appendDraws( RenderFrame& frame, const MaterialData& material ) const;
    };
}
