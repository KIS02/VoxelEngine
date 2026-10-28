#include "render/backends/software/SoftwareRenderBackend.h"

#include <iostream>

#include "benchmark/BenchmarkCpuTimer.h"
#include "benchmark/BenchmarkTypes.h"
#include "platform/windows/Win32FramePresenter.h"
#include "render/backends/software/DepthBuffer.h"
#include "render/backends/software/FrameBuffer.h"
#include "render/backends/software/Rasterizer.h"
#include "render/backends/software/SoftwareOverlayRenderer.h"
#include "render/backends/software/pipeline/geometry/SoftwareGeometryPass.h"
#include "render/backends/software/resources/HdrColorBuffer.h"
#include "render/color/ColorSpace.h"
#include "render/frame/RenderFrame.h"
#include "render/overlay/SoftwareDebugOverlay.h"


namespace ve
{
    SoftwareRenderBackend::SoftwareRenderBackend( FrameBuffer& frameBuffer, DepthBuffer& depthBuffer,
                                                  Rasterizer& rasterizer, Viewport& viewport,
                                                  Win32FramePresenter& presenter )
        : frameBuffer_(frameBuffer), depthBuffer_(depthBuffer), rasterizer_(rasterizer),
          viewport_(viewport), presenter_(presenter) { }


    void SoftwareRenderBackend::initialize() {
        std::cout << "[SoftwareBackend] Initialized with HDR-ready pipeline\n";
    }


    RenderSize SoftwareRenderBackend::framebufferSize() const {
        return RenderSize{ frameBuffer_.width(), frameBuffer_.height() };
    }


    void SoftwareRenderBackend::render( const RenderFrame& frame ) {
        BenchmarkCpuTimings unusedTimings{};
        BenchmarkCpuTimings& timings = measuredCpuTimings() != nullptr
            ? *measuredCpuTimings() : unusedTimings;

        {
            BenchmarkCpuScope renderScope(timings.rendererWorkMs);
            depthBuffer_.clear(1.0f);

            if (frame.toneMapping.enabled) {
                if (hdrBuffer_.width() != frameBuffer_.width() || hdrBuffer_.height() != frameBuffer_.height())
                    hdrBuffer_.resize(frameBuffer_.width(), frameBuffer_.height());

                hdrBuffer_.clear(HdrPixel{ decodeSrgb(frame.clearColor), getAlpha(frame.clearColor) });
                SoftwareGeometryPass::draw(frame, rasterizer_, viewport_, &hdrBuffer_);
                hdrBuffer_.resolve(frameBuffer_, frame.toneMapping);
            } else {
                frameBuffer_.clear(frame.clearColor);
                SoftwareGeometryPass::draw(frame, rasterizer_, viewport_);
            }

            // HUD and selection outlines are SDR, drawn after Tone Mapping.
            SoftwareOverlayRenderer::drawSelectionOutline(frameBuffer_, depthBuffer_, viewport_, frame);
            SoftwareOverlayRenderer::drawCrosshair(frameBuffer_, frame.ui.crosshair);
            SoftwareDebugOverlay::draw(frameBuffer_, frame.ui.debugOverlay);
        }

        {
            BenchmarkCpuScope presentScope(timings.presentMs);
            presenter_.present(frameBuffer_);
        }
    }
}
