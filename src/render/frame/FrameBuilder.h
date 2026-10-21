#pragma once

#include <cstdint>

#include "render/frame/RenderFrame.h"
#include "render/api/RenderBackend.h"
#include "benchmark/BenchmarkTypes.h"


namespace ve
{
    class Scene;
    class AssetManager;
    class UIManager;
    class RenderHost;

    class FrameBuilder {
        public:
            static RenderFrame build( const Scene& scene, const AssetManager& assets,
                                      const UIManager& ui, int width, int height,
                                      bool cursorCaptured, RenderBackend backend );
            static void submit( RenderHost& host, const RenderFrame& frame,
                                BenchmarkCpuTimings& cpuTimings, std::uint64_t frameId,
                                BenchmarkRenderStatistics* statistics );
    };
}
