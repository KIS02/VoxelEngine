#pragma once

#include <cstdint>

#include "render/api/IRenderBackend.h"
#include "render/backends/opengl/OpenGLGpuTimer.h"
#include "render/backends/opengl/OpenGLDebugOverlay.h"
#include "render/backends/opengl/OpenGLOverlayRenderer.h"
#include "render/backends/opengl/resources/OpenGLResourceCache.h"
#include "render/backends/opengl/pipeline/shadow/OpenGLShadowPass.h"
#include "render/backends/opengl/pipeline/postprocess/OpenGLHdrPass.h"


namespace ve
{
    class Window;
    class OpenGLRenderer;


    class OpenGLRenderBackend final : public IRenderBackend {
        private:
            Window& window_;
            OpenGLRenderer& renderer_;
            OpenGLDebugOverlay debugOverlay_;
            OpenGLOverlayRenderer overlayRenderer_;
            OpenGLResourceCache resources_;
            OpenGLShadowPass shadowPass_;
            OpenGLHdrPass hdrPass_;
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
    };
}
