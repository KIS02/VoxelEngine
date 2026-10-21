#include "render/RenderHost.h"

#include <memory>
#include <stdexcept>

#include "core/Window.h"
#include "assets/texture/Texture2D.h"
#include "platform/windows/Win32FramePresenter.h"
#include "render/backends/opengl/OpenGLRenderer.h"
#include "render/backends/opengl/OpenGLRenderBackend.h"
#include "render/backends/software/DepthBuffer.h"
#include "render/backends/software/FrameBuffer.h"
#include "render/backends/software/Rasterizer.h"
#include "render/backends/software/SoftwareRenderBackend.h"
#include "render/backends/software/Viewport.h"


namespace ve
{
    struct RenderHost::Impl
    {
        FrameBuffer frameBuffer_;
        DepthBuffer depthBuffer_;
        Rasterizer rasterizer_;
        Viewport viewport_;
        Win32FramePresenter presenter_;
        OpenGLRenderer openGLRenderer_;
        std::unique_ptr<IRenderBackend> driver_;


        Impl( RenderBackend backend, Window& window, const Texture2D& texture )
            : frameBuffer_(800, 600), depthBuffer_(800, 600),
              rasterizer_(frameBuffer_, depthBuffer_), viewport_(800, 600),
              presenter_(window.nativeHandle()) {
            if (backend == RenderBackend::OpenGL) {
                driver_ = std::make_unique<OpenGLRenderBackend>(window, openGLRenderer_, texture);
            } else if (backend == RenderBackend::Software) {
                driver_ = std::make_unique<SoftwareRenderBackend>(
                    frameBuffer_, depthBuffer_, rasterizer_, viewport_, presenter_, texture
                );
            } else {
                throw std::invalid_argument("Unsupported Render Backend");
            }

            driver_->initialize();
        }
    };


    RenderHost::RenderHost( RenderBackend backend, Window& window, const Texture2D& texture )
        : impl_(std::make_unique<Impl>(backend, window, texture)) { }

    RenderHost::~RenderHost() = default;

    RenderSize RenderHost::framebufferSize() const {
        return impl_->driver_->framebufferSize();
    }

    void RenderHost::renderMeasured( const RenderFrame& frame, BenchmarkCpuTimings& cpuTimings,
                                     std::uint64_t frameId, BenchmarkRenderStatistics* statistics ) {
        impl_->driver_->renderMeasured(frame, cpuTimings, frameId, statistics);
    }

    bool RenderHost::tryPopGpuTiming( std::uint64_t& frameId, BenchmarkGpuTiming& timing ) {
        return impl_->driver_->tryPopGpuTiming(frameId, timing);
    }
}
