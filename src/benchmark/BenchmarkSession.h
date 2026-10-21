#pragma once

#include <cstdint>
#include <functional>

#include "benchmark/BenchmarkFrameCollector.h"
#include "benchmark/BenchmarkRunController.h"
#include "render/api/IRenderBackend.h"


namespace ve
{
    class BenchmarkSession {
        private:
            BenchmarkRunController controller_;
            BenchmarkFrameCollector collector_;
            std::uint64_t expectedChunkCount_ = 0;


        public:
            void begin( const BenchmarkRunConfiguration& configuration,
                        std::uint64_t expectedChunkCount, int requestedSwapInterval );

            bool recordFrame( std::uint64_t frameIndex, const BenchmarkCpuTimings& cpuTimings,
                              const BenchmarkRenderStatistics& statistics, const RenderSize& size,
                              bool gpuResultAvailable, std::uint64_t completedGpuFrameId,
                              const BenchmarkGpuTiming& completedGpuTiming );

            bool isCompleted() const;
            void finish( const std::function<bool(std::uint64_t&, BenchmarkGpuTiming&)>& tryPopGpuTiming );
    };
}
