#include "render/host/RenderHost.h"

#include <memory>
#include <stdexcept>

#include "platform/window/Window.h"
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
        struct SoftwareResources
        {
            FrameBuffer frameBuffer;
            DepthBuffer depthBuffer;
            Rasterizer rasterizer;
            Viewport viewport;
            Win32FramePresenter presenter;

            explicit SoftwareResources( Window& window )
                : frameBuffer(800, 600), depthBuffer(800, 600),
                  rasterizer(frameBuffer, depthBuffer), viewport(800, 600),
                  presenter(window.nativeHandle()) { }
        };

        std::unique_ptr<SoftwareResources> software_;
        std::unique_ptr<OpenGLRenderer> openGL_;
        std::unique_ptr<IRenderBackend> driver_;

        Impl( RenderBackend backend, Window& window ) {
            if (backend == RenderBackend::OpenGL) {
                openGL_ = std::make_unique<OpenGLRenderer>();
                driver_ = std::make_unique<OpenGLRenderBackend>(window, *openGL_);
            } else if (backend == RenderBackend::Software) {
                software_ = std::make_unique<SoftwareResources>(window);
                driver_ = std::make_unique<SoftwareRenderBackend>(
                    software_->frameBuffer, software_->depthBuffer, software_->rasterizer,
                    software_->viewport, software_->presenter
                );
            } else {
                throw std::invalid_argument("Unsupported Render Backend");
            }
            driver_->initialize();
        }
    };

    RenderHost::RenderHost( RenderBackend backend, Window& window )
        : impl_(std::make_unique<Impl>(backend, window)) { }

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
