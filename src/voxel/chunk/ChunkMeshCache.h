#pragma once

#include <cstdint>
#include <map>

#include "assets/mesh/MeshData.h"
#include "assets/mesh/MeshIdGenerator.h"
#include "voxel/chunk/VoxelStorage.h"


namespace ve
{
    class ChunkMeshCache {
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
            explicit ChunkMeshCache( MeshIdGenerator& meshIdGenerator );

            std::uint64_t rebuildDirtyMeshes( VoxelStorage& world );
            void remove( const ChunkPosition& position );
            template <typename Visitor>
            void forEachMesh( Visitor&& visitor ) const {
                for (const auto& [position, entry] : chunkMeshes_)
                    visitor(position, entry.mesh, entry.id, entry.revision);
            }
    };
}
