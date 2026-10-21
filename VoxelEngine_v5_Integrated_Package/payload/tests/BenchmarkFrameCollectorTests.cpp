
#include <cmath>
#include <iostream>
#include <stdexcept>

#include "benchmark/BenchmarkFrameCollector.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    ve::BenchmarkRunConfiguration makeConfiguration( ve::BenchmarkBackend backend ) {
        ve::BenchmarkRunConfiguration config{};
        config.backend = backend;
        config.scenarioName = "fixed_demo_world_4_chunks";
        config.buildConfiguration = "Release";
        config.framebufferWidth = 800;
        config.framebufferHeight = 600;
        config.warmupFrameCount = 2;
        config.measurementFrameCount = 3;
        return config;
    }

    ve::BenchmarkCpuTimings makeCpuTimings() {
        ve::BenchmarkCpuTimings cpu{};
        cpu.frameWallMs = 12.0;
        cpu.rendererWorkMs = 4.0;
        cpu.presentMs = 8.0;
        return cpu;
    }

    ve::BenchmarkRenderStatistics makeRenderStatistics() {
        ve::BenchmarkRenderStatistics render{};
        render.residentChunkCount = 4;
        render.meshDrawsSubmitted = 5;
        render.meshVerticesReferenced = 11672;
        render.meshIndicesSubmitted = 17508;
        render.trianglesSubmitted = 5836;
        return render;
    }


    void testSoftwareSamples() {
        ve::BenchmarkFrameCollector collector;
        collector.begin(makeConfiguration(ve::BenchmarkBackend::Software), 3);

        for (std::uint64_t id = 3; id <= 5; ++id)
            collector.recordFrame(id, makeCpuTimings(), makeRenderStatistics());

        require(collector.allCpuFramesRecorded(), "All Software frames must be collected");
        require(collector.samples().size() == 3, "Wrong Software sample count");
        require(collector.gpuAvailableCount() == 0, "Software must have no GPU timings");
        require(collector.gpuUnresolvedCount() == 0, "Software GPU timing is not applicable");

        require(collector.samples()[0].gpu.state == ve::BenchmarkGpuTimingState::NotApplicable,
                "Software GPU state must be NotApplicable");

        require(collector.samples()[2].render.trianglesSubmitted == 5836,
                "Geometry statistics must be preserved");

        std::cout << "[PASS] Software frames and NotApplicable GPU timing\n";
    }


    void testDelayedAndOutOfOrderGpuResults() {
        ve::BenchmarkFrameCollector collector;
        collector.begin(makeConfiguration(ve::BenchmarkBackend::OpenGL), 121);

        for (std::uint64_t id = 121; id <= 123; ++id)
            collector.recordFrame(id, makeCpuTimings(), makeRenderStatistics());

        require(collector.gpuUnresolvedCount() == 3, "GPU samples should start Pending");

        ve::BenchmarkGpuTiming result{};
        result.state = ve::BenchmarkGpuTimingState::Available;
        result.elapsedMs = 0.75;

        require(!collector.attachGpuResult(120, result), "Warmup GPU result must be ignored");
        require(!collector.attachGpuResult(124, result), "Future GPU result must be ignored");
        require(collector.attachGpuResult(123, result), "Delayed GPU result must attach by Frame ID");

        result.elapsedMs = 0.25;
        require(collector.attachGpuResult(121, result), "Earlier GPU result must attach");

        require(collector.gpuAvailableCount() == 2, "Two GPU samples should be available");
        require(collector.gpuUnresolvedCount() == 1, "One GPU sample should be unresolved");

        require(std::fabs(collector.samples()[0].gpu.elapsedMs - 0.25) < 1e-9,
                "GPU result must be associated with original Frame ID");

        require(std::fabs(collector.samples()[2].gpu.elapsedMs - 0.75) < 1e-9,
                "Out-of-order GPU result must remain correctly associated");

        result.elapsedMs = 0.5;
        require(collector.attachGpuResult(122, result), "Final GPU result must attach");
        require(collector.gpuUnresolvedCount() == 0, "All GPU results should be resolved");

        std::cout << "[PASS] Delayed and out-of-order GPU results\n";
    }


    void testDuplicateAndInvalidGpuResult() {
        ve::BenchmarkFrameCollector collector;
        collector.begin(makeConfiguration(ve::BenchmarkBackend::OpenGL), 3);
        collector.recordFrame(3, makeCpuTimings(), makeRenderStatistics());

        ve::BenchmarkGpuTiming invalid{};
        invalid.state = ve::BenchmarkGpuTimingState::Pending;

        bool invalidRejected = false;

        try {
            collector.attachGpuResult(3, invalid);
        } catch (const std::invalid_argument&) {
            invalidRejected = true;
        }

        require(invalidRejected, "Pending GPU result must be rejected");

        ve::BenchmarkGpuTiming valid{};
        valid.state = ve::BenchmarkGpuTimingState::Available;
        valid.elapsedMs = 1.0;

        require(collector.attachGpuResult(3, valid), "Valid GPU result must attach");

        bool duplicateRejected = false;

        try {
            collector.attachGpuResult(3, valid);
        } catch (const std::logic_error&) {
            duplicateRejected = true;
        }

        require(duplicateRejected, "Duplicate GPU result must be rejected");

        std::cout << "[PASS] Invalid and duplicate GPU results rejected\n";
    }


    void testFrameOrderAndCapacity() {
        ve::BenchmarkFrameCollector collector;
        collector.begin(makeConfiguration(ve::BenchmarkBackend::Software), 3);

        bool orderRejected = false;

        try {
            collector.recordFrame(4, makeCpuTimings(), makeRenderStatistics());
        } catch (const std::invalid_argument&) {
            orderRejected = true;
        }

        require(orderRejected, "Wrong Frame ID must be rejected");

        for (std::uint64_t id = 3; id <= 5; ++id)
            collector.recordFrame(id, makeCpuTimings(), makeRenderStatistics());

        bool overflowRejected = false;

        try {
            collector.recordFrame(6, makeCpuTimings(), makeRenderStatistics());
        } catch (const std::logic_error&) {
            overflowRejected = true;
        }

        require(overflowRejected, "Extra measurement frame must be rejected");

        std::cout << "[PASS] Sequential Frame IDs and exact sample count\n";
    }


    void testBadSamplesAndReset() {
        ve::BenchmarkFrameCollector collector;
        collector.begin(makeConfiguration(ve::BenchmarkBackend::Software), 3);

        ve::BenchmarkCpuTimings invalidCpu = makeCpuTimings();
        invalidCpu.frameWallMs = -1.0;

        bool rejected = false;

        try {
            collector.recordFrame(3, invalidCpu, makeRenderStatistics());
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected, "Invalid CPU measurement must be rejected");
        require(collector.samples().empty(), "Rejected sample must not be stored");

        collector.recordFrame(3, makeCpuTimings(), makeRenderStatistics());
        collector.reset();

        require(collector.samples().empty(), "Reset must clear samples");

        collector.begin(makeConfiguration(ve::BenchmarkBackend::OpenGL), 121);
        require(collector.samples().empty(), "Restart must start empty");

        std::cout << "[PASS] Sample validation, reset and restart\n";
    }
}


int main() {
    try {
        testSoftwareSamples();
        testDelayedAndOutOfOrderGpuResults();
        testDuplicateAndInvalidGpuResult();
        testFrameOrderAndCapacity();
        testBadSamplesAndReset();

        std::cout << "\nAll Benchmark Frame Collector tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
