#pragma once

#include <cstdint>

#include "benchmark/BenchmarkTypes.h"


namespace ve
{
    class LiveBenchmarkReporter {
        public:
            static void report( std::uint64_t frameIndex, const BenchmarkCpuTimings& cpuTimings,
                                const BenchmarkRenderStatistics& statistics, double backendCallWallMs,
                                bool gpuResultAvailable, std::uint64_t completedGpuFrameId,
                                const BenchmarkGpuTiming& completedGpuTiming );
    };
}
