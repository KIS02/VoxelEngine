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

                MeshData legacy = ChunkMeshBuilder::create(world, position);
                MaterialMeshes next = ChunkMeshBuilder::createMaterialMeshes(world, position);

                auto found = chunkMeshes_.find(position);

                if (found != chunkMeshes_.end() &&
                    found->second.revision == std::numeric_limits<std::uint64_t>::max())
                    throw std::overflow_error("Chunk Mesh revision overflow");

                std::map<BlockMaterialKey, MaterialMeshEntry> updated;

                for (auto& [key, mesh] : next) {
                    std::uint64_t id = 0;
                    std::uint64_t revision = 1;

                    if (found != chunkMeshes_.end()) {
                        const auto previous = found->second.materials.find(key);

                        if (previous != found->second.materials.end()) {
                            id = previous->second.id;
                            revision = previous->second.revision;

                            if (revision == std::numeric_limits<std::uint64_t>::max())
                                throw std::overflow_error("Material Submesh revision overflow");

                            ++revision;
                        }
                    }

                    if (id == 0)
                        id = meshIdGenerator_.generate();

                    updated.emplace(key, MaterialMeshEntry{ std::move(mesh), revision, id });
                }

                if (found == chunkMeshes_.end()) {
                    const std::uint64_t id = meshIdGenerator_.generate();

                    ChunkMeshEntry entry{ std::move(legacy), 1, id, std::move(updated) };
                    chunkMeshes_.emplace(position, std::move(entry));

                    std::cout << "[ChunkMesh] Created ("
                              << position.x << ", " << position.y << ", " << position.z << ")"
                              << " | Mesh ID " << id << " | Revision 1\n";
                } else {
                    found->second.mesh = std::move(legacy);
                    found->second.materials = std::move(updated);
                    ++found->second.revision;

                    std::cout << "[ChunkMesh] Rebuilt ("
                              << position.x << ", " << position.y << ", " << position.z << ")"
                              << " | Mesh ID " << found->second.id
                              << " | Revision " << found->second.revision << '\n';
                }

                ++rebuildCount;
            });
        } catch (const std::exception& error) {
            std::cerr << "[ChunkMesh] Rebuild failed: " << error.what()
                      << " | Dirty Chunks Remaining: " << world.dirtyChunkCount() << '\n';
        }

        return rebuildCount;
    }


    void ChunkMeshCache::remove( const ChunkPosition& position ) {
        chunkMeshes_.erase(position);
    }
}