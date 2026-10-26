#include "render/backends/opengl/resources/OpenGLResourceCache.h"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "assets/mesh/MeshData.h"
#include "assets/texture/Texture2D.h"
#include "benchmark/BenchmarkTypes.h"
#include "math/Color.h"
#include "render/backends/opengl/pipeline/shading/DefaultTextureShaders.h"
#include "render/common/MaterialCachePolicy.h"


namespace ve
{
    namespace
    {
        std::vector<std::uint8_t> toRGBA8( const Texture2D& texture ) {
            if (texture.width() <= 0 || texture.height() <= 0)
                throw std::invalid_argument("OpenGL Material Texture dimensions are invalid");

            const std::size_t count = static_cast<std::size_t>(texture.width()) *
                                      static_cast<std::size_t>(texture.height());
            std::vector<std::uint8_t> bytes(count * 4);

            for (int y = 0; y < texture.height(); ++y) {
                for (int x = 0; x < texture.width(); ++x) {
                    const Color32 color = texture.pixel(x, y);
                    const std::size_t offset =
                        (static_cast<std::size_t>(y) * static_cast<std::size_t>(texture.width()) +
                         static_cast<std::size_t>(x)) * 4;
                    bytes[offset + 0] = getRed(color);
                    bytes[offset + 1] = getGreen(color);
                    bytes[offset + 2] = getBlue(color);
                    bytes[offset + 3] = getAlpha(color);
                }
            }
            return bytes;
        }
    }


    OpenGLMaterial& OpenGLResourceCache::resolveMaterial( const Texture2D& texture ) {
        const auto found = materials_.find(texture.id());
        const MaterialCacheAction action = MaterialCachePolicy::decide(
            found != materials_.end(), found != materials_.end() ? found->second.revision : 0,
            texture.revision()
        );

        if (action == MaterialCacheAction::Reuse)
            return *found->second.material;

        std::vector<std::uint8_t> pixels = toRGBA8(texture);
        auto replacement = std::make_unique<OpenGLMaterial>();
        replacement->initialize(DefaultTextureVertexShader, DefaultTextureFragmentShader,
                                texture.width(), texture.height(), pixels.data());

        const std::uint64_t revision = texture.revision();
        if (found != materials_.end()) {
            found->second.material = std::move(replacement);
            found->second.revision = revision;
            return *found->second.material;
        }

        CachedMaterial entry{};
        entry.material = std::move(replacement);
        entry.revision = revision;
        return *materials_.emplace(texture.id(), std::move(entry)).first->second.material;
    }


    const OpenGLMesh& OpenGLResourceCache::resolveMesh( const MeshData& mesh,
                                                        std::uint64_t meshId, std::uint64_t revision,
                                                        BenchmarkRenderStatistics* statistics ) {
        if (meshId == 0)
            throw std::invalid_argument("Cached OpenGL Mesh requires a nonzero Mesh ID");


        const auto found = meshes_.find(meshId);

        if (found != meshes_.end() && found->second.revision == revision)
            return *found->second.mesh;


        // Create Updated GPU Mesh

        auto newGpuMesh = std::make_unique<OpenGLMesh>();
        newGpuMesh->initialize(mesh);


        // First Upload

        if (found == meshes_.end()) {
            CachedMesh entry{};

            entry.mesh = std::move(newGpuMesh);
            entry.revision = revision;

            auto [iterator, inserted] = meshes_.emplace(meshId, std::move(entry));
            (void)inserted;


            // GPU Resource Statistics

            if (statistics != nullptr)
                ++statistics->gpuMeshUploadCount;


            std::cout << "[OpenGLBackend] Mesh uploaded"
                      << " | ID " << meshId
                      << " | Revision " << revision
                      << " | " << mesh.vertexCount() << " vertices"
                      << std::endl;

            return *iterator->second.mesh;
        }


        // Mesh Revision Changed

        found->second.mesh = std::move(newGpuMesh);
        found->second.revision = revision;


        // GPU Resource Statistics

        if (statistics != nullptr)
            ++statistics->gpuMeshUploadCount;


        std::cout << "[OpenGLBackend] Mesh reuploaded"
                  << " | ID " << meshId
                  << " | Revision " << revision
                  << " | " << mesh.vertexCount() << " vertices"
                  << std::endl;

        return *found->second.mesh;
    }


    void OpenGLResourceCache::releaseUnusedMeshes( const std::unordered_set<std::uint64_t>& activeIds,
                                                   BenchmarkRenderStatistics* statistics ) {
        for (auto iterator = meshes_.begin(); iterator != meshes_.end();) {
            if (activeIds.find(iterator->first) != activeIds.end()) {
                ++iterator;
                continue;
            }


            // Release Missing Mesh

            const std::uint64_t releasedId = iterator->first;

            iterator = meshes_.erase(iterator);


            // GPU Resource Statistics

            if (statistics != nullptr)
                ++statistics->gpuMeshReleaseCount;


            std::cout << "[OpenGLBackend] Mesh released"
                      << " | ID " << releasedId
                      << std::endl;
        }
    }


    void OpenGLResourceCache::releaseUnusedMaterials( const std::unordered_set<std::uint64_t>& activeIds ) {
        for (auto iterator = materials_.begin(); iterator != materials_.end();) {
            if (!MaterialCachePolicy::shouldRelease(iterator->first, activeIds)) {
                ++iterator;
                continue;
            }

            iterator = materials_.erase(iterator);
        }
    }


    void OpenGLResourceCache::releaseUnused( const std::unordered_set<std::uint64_t>& meshIds,
                                             const std::unordered_set<std::uint64_t>& textureIds,
                                             BenchmarkRenderStatistics* statistics ) {
        releaseUnusedMeshes(meshIds, statistics);
        releaseUnusedMaterials(textureIds);
    }
}
