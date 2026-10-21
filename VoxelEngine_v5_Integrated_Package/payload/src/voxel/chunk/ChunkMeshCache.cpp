#include "voxel/chunk/ChunkMeshCache.h"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

#include "voxel/chunk/ChunkMeshBuilder.h"


namespace ve
{
    ChunkMeshCache::ChunkMeshCache( MeshIdGenerator& meshIdGenerator )
        : meshIdGenerator_(meshIdGenerator) { }


    std::uint64_t ChunkMeshCache::rebuildDirtyMeshes( VoxelStorage& world ) {
        std::uint64_t rebuildCount = 0;

        try {
            world.processDirtyChunks([this, &world, &rebuildCount]( const ChunkPosition& position ) {
                if (world.findChunk(position) == nullptr)
                    return;

                MeshData newMesh = ChunkMeshBuilder::create(world, position);
                const auto found = chunkMeshes_.find(position);

                if (found == chunkMeshes_.end()) {
                    const std::uint64_t meshId = meshIdGenerator_.generate();
                    ChunkMeshEntry entry{ std::move(newMesh), 1, meshId };
                    chunkMeshes_.emplace(position, std::move(entry));
                    ++rebuildCount;

                    std::cout << "[ChunkMesh] Created ("
                              << position.x << ", " << position.y << ", " << position.z << ")"
                              << " | Mesh ID " << meshId << " | Revision 1" << std::endl;
                } else {
                    if (found->second.revision == std::numeric_limits<std::uint64_t>::max())
                        throw std::overflow_error("Chunk Mesh revision overflow");

                    found->second.mesh = std::move(newMesh);
                    ++found->second.revision;
                    ++rebuildCount;

                    std::cout << "[ChunkMesh] Rebuilt ("
                              << position.x << ", " << position.y << ", " << position.z << ")"
                              << " | Mesh ID " << found->second.id
                              << " | Revision " << found->second.revision << std::endl;
                }
            });
        } catch (const std::exception& error) {
            std::cerr << "[ChunkMesh] Rebuild failed: " << error.what()
                      << " | Dirty Chunks Remaining: " << world.dirtyChunkCount() << std::endl;
        }

        return rebuildCount;
    }


    void ChunkMeshCache::remove( const ChunkPosition& position ) {
        chunkMeshes_.erase(position);
    }


}
