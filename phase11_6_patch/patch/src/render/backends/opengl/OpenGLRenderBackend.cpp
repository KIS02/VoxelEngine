#include "render/backends/opengl/OpenGLRenderBackend.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

#include "platform/window/Window.h"

#include "render/backends/opengl/OpenGLRenderCommand.h"
#include "render/backends/opengl/OpenGLRenderer.h"
#include "render/backends/opengl/pipeline/geometry/OpenGLGeometryPass.h"
#include "render/frame/RenderFrame.h"
#include "benchmark/BenchmarkCpuTimer.h"
#include "benchmark/BenchmarkTypes.h"
#include "math/Color.h"
#include "render/color/ColorSpace.h"

#include <glad/gl.h>

#ifndef GLFW_INCLUDE_NONE
    #define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>


namespace ve
{
    OpenGLRenderBackend::OpenGLRenderBackend( Window& window, OpenGLRenderer& renderer )
        : window_(window), renderer_(renderer) { }


    OpenGLRenderBackend::~OpenGLRenderBackend() = default;


    void OpenGLRenderBackend::initialize() {
        if (initialized_)
            throw std::runtime_error("OpenGLRenderBackend is already initialized");

        renderer_.initialize();
        overlayRenderer_.initialize();
        gpuTimer_.initialize();
        initialized_ = true;

        std::cout << "[OpenGLBackend] Initialized with Mesh ID Cache and Selection Outline" << std::endl;
        std::cout << "[Benchmark][GPU] OpenGL Timer Query initialized" << std::endl;
    }


    RenderSize OpenGLRenderBackend::framebufferSize() const {
        GLFWwindow* context = glfwGetCurrentContext();

        if (context == nullptr)
            throw std::runtime_error("No current OpenGL context");

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(context, &width, &height);
        return RenderSize{ width, height };
    }


    void OpenGLRenderBackend::render( const RenderFrame& frame ) {
        BenchmarkCpuTimings unusedTimings{};
        BenchmarkCpuTimings& timings = measuredCpuTimings() != nullptr
            ? *measuredCpuTimings() : unusedTimings;

        {
            BenchmarkCpuScope renderScope(timings.rendererWorkMs);

            if (!initialized_)
                throw std::runtime_error("OpenGLRenderBackend is not initialized");

            const RenderSize size = framebufferSize();

            if (size.width <= 0 || size.height <= 0)
                return;

            const std::uint64_t frameId = measuredFrameId() != 0
                ? measuredFrameId() : ++directRenderFrameId_;

            const bool gpuQueryStarted = gpuTimer_.begin(frameId);
            using Clock = std::chrono::steady_clock;
            const bool profile = std::getenv("VOXELENGINE_PROFILE_RENDER") != nullptr && frameId % 60 == 0;
            const auto profileStart = profile ? Clock::now() : Clock::time_point{};
            Clock::time_point afterShadow{};
            Clock::time_point afterGeometry{};

            if (!gpuQueryStarted && (gpuTimer_.skippedCount() == 1 ||
                                     gpuTimer_.skippedCount() % 120 == 0)) {
                std::cout << "[Benchmark][GPU] Query pool full"
                          << " | Skipped samples " << gpuTimer_.skippedCount() << std::endl;
            }

            try {
                // Render shadow maps before switching the color target to HDR.
                shadowPass_.render(frame, resources_, measuredRenderStatistics());
                if (profile)
                    afterShadow = Clock::now();
                if (frame.toneMapping.enabled)
                    hdrPass_.beginScene(size.width, size.height);

                constexpr float colorScale = 1.0f / 255.0f;
                OpenGLClearCommand clear{};
                clear.red = static_cast<float>(getRed(frame.clearColor)) * colorScale;
                clear.green = static_cast<float>(getGreen(frame.clearColor)) * colorScale;
                clear.blue = static_cast<float>(getBlue(frame.clearColor)) * colorScale;
                clear.alpha = static_cast<float>(getAlpha(frame.clearColor)) * colorScale;
                if (frame.toneMapping.enabled) {
                    const LinearRGB linearClear = decodeSrgb(frame.clearColor);
                    clear.red = linearClear.r;
                    clear.green = linearClear.g;
                    clear.blue = linearClear.b;
                }
                clear.clearDepth = true;
                renderer_.submit(clear);

                OpenGLGeometryPass::draw(frame, renderer_, resources_, measuredRenderStatistics(),
                                         &shadowPass_, frame.toneMapping.enabled);
                if (profile)
                    afterGeometry = Clock::now();
                if (frame.toneMapping.enabled)
                    hdrPass_.present(frame.toneMapping);

                overlayRenderer_.drawSelectionOutline(frame);
                overlayRenderer_.drawCrosshair(frame.ui.crosshair, size.width, size.height);
                debugOverlay_.draw(frame.ui.debugOverlay, size.width, size.height);
            } catch (...) {
                if (gpuQueryStarted)
                    gpuTimer_.end();
                throw;
            }

            if (gpuQueryStarted)
                gpuTimer_.end();
        }

        {
            BenchmarkCpuScope presentScope(timings.presentMs);
            window_.swapBuffers();
        }

        if (profile) {
            const auto end = Clock::now();
            const auto elapsed = []( Clock::time_point a, Clock::time_point b ) {
                return std::chrono::duration<double, std::milli>(b - a).count();
            };
            std::cout << "[Profile][OpenGL CPU] Shadow=" << elapsed(profileStart, afterShadow)
                      << " ms | Geometry=" << elapsed(afterShadow, afterGeometry)
                      << " ms | Post+Present=" << elapsed(afterGeometry, end) << " ms\n";
        }
    }


    bool OpenGLRenderBackend::tryPopGpuTiming( std::uint64_t& frameId, BenchmarkGpuTiming& timing ) {
        return gpuTimer_.tryCollect(frameId, timing);
    }
}
