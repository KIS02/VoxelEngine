#include "world/meshing/ChunkMeshManager.h"

#include <iostream>
#include <limits>
#include <stdexcept>
#include <utility>

#include "assets/material/MaterialData.h"
#include "math/Mat4.h"
#include "math/Vec3.h"
#include "world/meshing/ChunkMeshFactory.h"


namespace ve
{
    ChunkMeshManager::ChunkMeshManager( MeshIdGenerator& meshIdGenerator )
        : meshIdGenerator_(meshIdGenerator) { }


    std::uint64_t ChunkMeshManager::rebuildDirtyMeshes( World& world ) {
        std::uint64_t rebuildCount = 0;

        try {
            world.processDirtyChunks([this, &world, &rebuildCount]( const ChunkPosition& position ) {
                if (world.findChunk(position) == nullptr)
                    return;

                MeshData newMesh = ChunkMeshFactory::create(world, position);
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


    void ChunkMeshManager::remove( const ChunkPosition& position ) {
        chunkMeshes_.erase(position);
    }


    void ChunkMeshManager::appendDraws( RenderFrame& frame, const MaterialData& material ) const {
        for (const auto& [position, entry] : chunkMeshes_) {
            const Vec3 translation{
                static_cast<float>(position.x) * static_cast<float>(Chunk::Size),
                static_cast<float>(position.y) * static_cast<float>(Chunk::Size),
                static_cast<float>(position.z) * static_cast<float>(Chunk::Size)
            };

            frame.draws.push_back(RenderDraw{
                &entry.mesh, Mat4::translation(translation), entry.revision, entry.id, &material
            });
        }
    }
}
