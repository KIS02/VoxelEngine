#include "render/backends/opengl/OpenGLRenderBackend.h"

#include <cstdint>
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

            if (!gpuQueryStarted && (gpuTimer_.skippedCount() == 1 ||
                                     gpuTimer_.skippedCount() % 120 == 0)) {
                std::cout << "[Benchmark][GPU] Query pool full"
                          << " | Skipped samples " << gpuTimer_.skippedCount() << std::endl;
            }

            try {
                static bool shadowStateLogged = false;

                if (!shadowStateLogged) {
                    std::cout << "[ShadowState] SceneLights=" << frame.lighting.sceneLights.size() << '\n';

                    for (const auto& light : frame.lighting.sceneLights) {
                        std::cout << "[ShadowState] Light=" << light.objectId
                                << " | CastsShadows=" << light.castsShadows << '\n';
                    }

                    shadowStateLogged = true;
                }

                shadowPass_.render(frame, resources_, measuredRenderStatistics());


                // Render shadow maps before switching the color target to HDR.
                shadowPass_.render(frame, resources_, measuredRenderStatistics());
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
    }


    bool OpenGLRenderBackend::tryPopGpuTiming( std::uint64_t& frameId, BenchmarkGpuTiming& timing ) {
        return gpuTimer_.tryCollect(frameId, timing);
    }
}
