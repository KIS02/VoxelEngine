#pragma once

#include <cstdint>
#include <memory>

#include "render/api/IRenderBackend.h"
#include "render/api/RenderBackend.h"


namespace ve
{
    class Window;
    struct RenderFrame;
    struct BenchmarkCpuTimings;
    struct BenchmarkGpuTiming;
    struct BenchmarkRenderStatistics;


    class RenderHost {
        private:
            struct Impl;
            std::unique_ptr<Impl> impl_;


        public:
            RenderHost( RenderBackend backend, Window& window );
            ~RenderHost();

            RenderHost( const RenderHost& ) = delete;
            RenderHost& operator=( const RenderHost& ) = delete;

            RenderSize framebufferSize() const;
            void renderMeasured( const RenderFrame& frame, BenchmarkCpuTimings& cpuTimings,
                                 std::uint64_t frameId, BenchmarkRenderStatistics* statistics );
            bool tryPopGpuTiming( std::uint64_t& frameId, BenchmarkGpuTiming& timing );
    };
}
