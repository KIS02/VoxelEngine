#pragma once

#include <cstdint>

#include "voxel/chunk/VoxelStorage.h"
#include "voxel/chunk/ChunkMeshCache.h"


namespace ve
{
    // Per-Scene voxel instance: owns the block storage and derived CPU mesh cache.
    class VoxelWorld {
        private:
            VoxelStorage blocks_;
            ChunkMeshCache meshes_;

        public:
            explicit VoxelWorld( MeshIdGenerator& meshIds );
            VoxelStorage& blocks();
            const VoxelStorage& blocks() const;
            const ChunkMeshCache& meshes() const;
            std::uint64_t rebuildDirtyMeshes();
            bool removeChunk( const ChunkPosition& position );
    };
}
