#include <stdexcept>

#include "benchmark/BenchmarkSession.h"


namespace
{
    void require( bool condition ) {
        if (!condition)
            throw std::runtime_error("R6 benchmark session contract failed");
    }
}


int main() {
    ve::BenchmarkRunConfiguration configuration{};
    configuration.backend = ve::BenchmarkBackend::Software;
    configuration.scenarioName = "r6_fixed_scene";
    configuration.buildConfiguration = "test";
    configuration.framebufferWidth = 800;
    configuration.framebufferHeight = 600;
    configuration.warmupFrameCount = 2;
    configuration.measurementFrameCount = 3;

    ve::BenchmarkSession session{};
    session.begin(configuration, 4, -1);
    ve::BenchmarkCpuTimings timings{};
    ve::BenchmarkRenderStatistics statistics{};
    statistics.residentChunkCount = 4;
    ve::RenderSize size{ 800, 600 };

    for (std::uint64_t frame = 1; frame <= 5; frame++) {
        const bool completed = session.recordFrame(frame, timings, statistics, size, false, 0, {});
        require(completed == (frame == 5));
    }
    require(session.isCompleted());

    ve::BenchmarkSession rejected{};
    rejected.begin(configuration, 4, -1);
    bool detectedMutation = false;
    statistics.meshRebuildCount = 1;
    try {
        rejected.recordFrame(1, timings, statistics, size, false, 0, {});
    } catch (const std::runtime_error&) {
        detectedMutation = true;
    }
    require(detectedMutation);

    statistics.meshRebuildCount = 0;
    detectedMutation = false;
    try {
        rejected.recordFrame(1, timings, statistics, { 1024, 600 }, false, 0, {});
    } catch (const std::runtime_error&) {
        detectedMutation = true;
    }
    require(detectedMutation);
    return 0;
}
