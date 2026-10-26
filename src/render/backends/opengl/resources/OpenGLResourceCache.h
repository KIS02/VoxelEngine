#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "render/backends/opengl/OpenGLMaterial.h"
#include "render/backends/opengl/OpenGLMesh.h"


namespace ve
{
    class MeshData;
    class Texture2D;
    struct BenchmarkRenderStatistics;


    class OpenGLResourceCache {
        private:
            struct CachedMesh
            {
                std::unique_ptr<OpenGLMesh> mesh;
                std::uint64_t revision = 0;
            };

            struct CachedMaterial
            {
                std::unique_ptr<OpenGLMaterial> material;
                std::uint64_t revision = 0;
            };

            std::unordered_map<std::uint64_t, CachedMesh> meshes_;
            std::unordered_map<std::uint64_t, CachedMaterial> materials_;


        private:
            void releaseUnusedMeshes( const std::unordered_set<std::uint64_t>& activeIds,
                                      BenchmarkRenderStatistics* statistics );
            void releaseUnusedMaterials( const std::unordered_set<std::uint64_t>& activeIds );


        public:
            OpenGLMaterial& resolveMaterial( const Texture2D& texture );
            const OpenGLMesh& resolveMesh( const MeshData& mesh, std::uint64_t meshId,
                                           std::uint64_t revision, BenchmarkRenderStatistics* statistics );

            void releaseUnused( const std::unordered_set<std::uint64_t>& meshIds,
                                const std::unordered_set<std::uint64_t>& textureIds,
                                BenchmarkRenderStatistics* statistics );
    };
}
