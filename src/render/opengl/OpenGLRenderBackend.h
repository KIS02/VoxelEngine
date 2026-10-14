#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>
#include <unordered_set>

#include "render/IRenderBackend.h"
#include "render/opengl/OpenGLMesh.h"



namespace ve
{
    class Window;
    class MeshData;
    class Texture2D;
    class OpenGLRenderer;
    class OpenGLCubeDemo;



    class OpenGLRenderBackend final : public IRenderBackend {
        private:
            struct CachedMesh
            {
                std::unique_ptr<OpenGLMesh> mesh;
                std::uint64_t revision = 0;
            };



            Window& window_;

            OpenGLRenderer& renderer_;
            OpenGLCubeDemo& demo_;

            const MeshData& sourceMesh_;
            const Texture2D& sourceTexture_;

            std::unordered_map<std::uint64_t, CachedMesh> uploadedMeshes_;

            unsigned int outlineProgram_ = 0;
            unsigned int outlineVao_ = 0;
            unsigned int outlineVbo_ = 0;

            bool initialized_ = false;


        public:
            OpenGLRenderBackend( Window& window, OpenGLRenderer& renderer,
                                 OpenGLCubeDemo& demo, const MeshData& sourceMesh,
                                 const Texture2D& sourceTexture );

            ~OpenGLRenderBackend() override;

            void initialize() override;

            RenderSize framebufferSize() const override;

            void render( const RenderFrame& frame ) override;


        private:
            const OpenGLMesh& resolveMesh( const MeshData& mesh,
                                           std::uint64_t meshId, std::uint64_t revision );

            void releaseUnusedMeshes( const std::unordered_set<std::uint64_t>& activeIds );

            void initializeOutlineResources();
            void drawSelectionOutline( const RenderFrame& frame );
    };
}
