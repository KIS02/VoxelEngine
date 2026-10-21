#include "scene/voxel/VoxelWorld.h"


namespace ve
{
    VoxelWorld::VoxelWorld( MeshIdGenerator& meshIds ) : meshes_(meshIds) { }

    VoxelStorage& VoxelWorld::blocks() {
        return blocks_;
    }

    const VoxelStorage& VoxelWorld::blocks() const {
        return blocks_;
    }

    const ChunkMeshCache& VoxelWorld::meshes() const {
        return meshes_;
    }

    std::uint64_t VoxelWorld::rebuildDirtyMeshes() {
        return meshes_.rebuildDirtyMeshes(blocks_);
    }

    bool VoxelWorld::removeChunk( const ChunkPosition& position ) {
        if (!blocks_.removeChunk(position))
            return false;

        meshes_.remove(position);
        return true;
    }
}
