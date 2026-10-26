#pragma once

#include <cstdint>
#include <map>
#include <utility>

#include "assets/block/BlockDefinition.h"
#include "assets/mesh/MeshData.h"
#include "assets/mesh/MeshIdGenerator.h"
#include "voxel/chunk/VoxelStorage.h"


namespace ve
{
    class ChunkMeshCache {
        private:
            struct MaterialMeshEntry
            {
                MeshData mesh;
                std::uint64_t revision = 1;
                std::uint64_t id = 0;
            };

            struct ChunkMeshEntry
            {
                MeshData mesh;
                std::uint64_t revision = 1;
                std::uint64_t id = 0;
                std::map<BlockMaterialKey, MaterialMeshEntry> materials;
            };


            MeshIdGenerator& meshIdGenerator_;
            std::map<ChunkPosition, ChunkMeshEntry, ChunkPositionLess> chunkMeshes_;


        public:
            explicit ChunkMeshCache( MeshIdGenerator& meshIdGenerator );

            std::uint64_t rebuildDirtyMeshes( VoxelStorage& world );
            void remove( const ChunkPosition& position );

            template<typename Visitor>
            void forEachMesh( Visitor&& visitor ) const {
                for (const auto& [position, entry] : chunkMeshes_)
                    visitor(position, entry.mesh, entry.id, entry.revision);
            }

            template<typename Visitor>
            void forEachMaterialMesh( Visitor&& visitor ) const {
                for (const auto& [position, chunk] : chunkMeshes_) {
                    for (const auto& [material, entry] : chunk.materials)
                        visitor(position, material, entry.mesh, entry.id, entry.revision);
                }
            }
    };
}