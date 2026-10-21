
#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "render/api/IRenderBackend.h"
#include "render/backends/opengl/OpenGLGpuTimer.h"
#include "render/backends/opengl/OpenGLMesh.h"
#include "render/backends/opengl/OpenGLMaterial.h"
#include "render/backends/opengl/OpenGLDebugOverlay.h"
#include "render/backends/opengl/OpenGLOverlayRenderer.h"

namespace ve
{
    class Window;
    class MeshData;
    class Texture2D;
    class OpenGLRenderer;


    class OpenGLRenderBackend final : public IRenderBackend {
        private:

            struct CachedMesh
            {
                std::unique_ptr<OpenGLMesh> mesh;
                std::uint64_t revision = 0;
            };


            Window& window_;

            OpenGLRenderer& renderer_;
            
            // 2D Debug HUD
            OpenGLDebugOverlay debugOverlay_;
            OpenGLOverlayRenderer overlayRenderer_;


            std::unordered_map<std::uint64_t, CachedMesh> uploadedMeshes_;
            struct CachedMaterial
            {
                std::unique_ptr<OpenGLMaterial> material;
                std::uint64_t revision = 0;
            };

            std::unordered_map<std::uint64_t, CachedMaterial> materials_;


            OpenGLGpuTimer gpuTimer_;
            std::uint64_t directRenderFrameId_ = 0;

            bool initialized_ = false;


        public:
            OpenGLRenderBackend( Window& window, OpenGLRenderer& renderer );

            ~OpenGLRenderBackend() override;

            void initialize() override;

            RenderSize framebufferSize() const override;

            void render( const RenderFrame& frame ) override;

            bool tryPopGpuTiming( std::uint64_t& frameId, BenchmarkGpuTiming& timing ) override;


        private:
            OpenGLMaterial& resolveMaterial( const Texture2D& texture );

            const OpenGLMesh& resolveMesh( const MeshData& mesh,
                                           std::uint64_t meshId, std::uint64_t revision );

            void releaseUnusedMeshes( const std::unordered_set<std::uint64_t>& activeIds );
            void releaseUnusedMaterials( const std::unordered_set<std::uint64_t>& activeIds );

    };
}
