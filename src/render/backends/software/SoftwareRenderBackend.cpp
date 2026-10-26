#include "render/backends/software/SoftwareRenderBackend.h"

#include <iostream>

#include "platform/windows/Win32FramePresenter.h"


#include "render/backends/software/DepthBuffer.h"
#include "render/backends/software/FrameBuffer.h"
#include "render/backends/software/Rasterizer.h"
#include "render/backends/software/pipeline/geometry/SoftwareGeometryPass.h"
#include "render/backends/software/Viewport.h"

#include "render/frame/RenderFrame.h"

#include "render/backends/software/SoftwareOverlayRenderer.h"
#include "render/overlay/SoftwareDebugOverlay.h"

#include "benchmark/BenchmarkCpuTimer.h"
#include "benchmark/BenchmarkTypes.h"


namespace ve
{
    SoftwareRenderBackend::SoftwareRenderBackend( FrameBuffer& frameBuffer, DepthBuffer& depthBuffer,
                                                  Rasterizer& rasterizer, Viewport& viewport,
                                                  Win32FramePresenter& presenter )
        : frameBuffer_(frameBuffer), depthBuffer_(depthBuffer), rasterizer_(rasterizer),
          viewport_(viewport), presenter_(presenter) { }


    void SoftwareRenderBackend::initialize() {
        std::cout << "[SoftwareBackend] Initialized with Clip Space triangle clipping" << std::endl;
    }


    RenderSize SoftwareRenderBackend::framebufferSize() const {
        return RenderSize{ frameBuffer_.width(), frameBuffer_.height() };
    }


    void SoftwareRenderBackend::render( const RenderFrame& frame ) {
        BenchmarkCpuTimings unusedTimings{};

        BenchmarkCpuTimings& timings = measuredCpuTimings() != nullptr
            ? *measuredCpuTimings() : unusedTimings;


        // CPU Rendering Work

        {
            BenchmarkCpuScope renderScope(timings.rendererWorkMs);

            frameBuffer_.clear(frame.clearColor);
            depthBuffer_.clear(1.0f);

            SoftwareGeometryPass::draw(frame, rasterizer_, viewport_);


            // Depth-tested 3D Selection Outline

            SoftwareOverlayRenderer::drawSelectionOutline(
                frameBuffer_, depthBuffer_, viewport_, frame
            );


            // Screen-space 2D Crosshair

            SoftwareOverlayRenderer::drawCrosshair(
                frameBuffer_, frame.ui.crosshair
            );


            // Screen-space F3 Debug Overlay

            SoftwareDebugOverlay::draw(
                frameBuffer_, frame.ui.debugOverlay
            );
        }


        // Win32 Frame Presentation

        {
            BenchmarkCpuScope presentScope(timings.presentMs);
            presenter_.present(frameBuffer_);
        }
    }
}