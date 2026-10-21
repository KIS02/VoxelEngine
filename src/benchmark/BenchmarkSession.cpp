#include "benchmark/BenchmarkSession.h"

#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

#include "benchmark/BenchmarkCsvWriter.h"
#include "benchmark/BenchmarkSummaryPrinter.h"


namespace ve
{
    void BenchmarkSession::begin( const BenchmarkRunConfiguration& configuration,
                                  std::uint64_t expectedChunkCount, int requestedSwapInterval ) {
        controller_.begin(configuration);
        collector_.begin(configuration, static_cast<std::uint64_t>(configuration.warmupFrameCount) + 1);
        expectedChunkCount_ = expectedChunkCount;

        std::cout << "[Benchmark] Fixed Mode Started"
                  << " | Backend " << (configuration.backend == BenchmarkBackend::OpenGL ? "OpenGL" : "Software")
                  << " | Resolution " << configuration.framebufferWidth << "x" << configuration.framebufferHeight
                  << " | Chunks " << expectedChunkCount_ << '\n';
        std::cout << "[Benchmark] Camera ("
                  << configuration.cameraPosition[0] << ", " << configuration.cameraPosition[1]
                  << ", " << configuration.cameraPosition[2] << ")"
                  << " | Forward (" << configuration.cameraForward[0] << ", "
                  << configuration.cameraForward[1] << ", " << configuration.cameraForward[2] << ")\n";
        std::cout << "[Benchmark][Environment] | Build " << configuration.buildConfiguration
                  << " | Scene " << configuration.scenarioName << '\n';
        if (configuration.backend == BenchmarkBackend::OpenGL) {
            std::cout << "[Benchmark][VSync] | Requested Swap Interval " << requestedSwapInterval
                      << " | Actual synchronization not verified\n";
        } else {
            std::cout << "[Benchmark][VSync] | NotApplicable (Software Backend)\n";
        }
        std::cout << "[Benchmark] Warmup " << configuration.warmupFrameCount
                  << " frames | Measurement " << configuration.measurementFrameCount << " frames\n";
    }

    bool BenchmarkSession::recordFrame( std::uint64_t frameIndex, const BenchmarkCpuTimings& cpuTimings,
                                        const BenchmarkRenderStatistics& renderStatistics, const RenderSize& size,
                                        bool gpuResultAvailable, std::uint64_t completedGpuFrameId,
                                        const BenchmarkGpuTiming& completedGpuTiming ) {
        const RenderSize currentSize = size;

        if (currentSize.width != 800 || currentSize.height != 600)
            throw std::runtime_error("Benchmark framebuffer resolution changed");

        if (renderStatistics.residentChunkCount != expectedChunkCount_)
            throw std::runtime_error("Benchmark World Chunk count changed");

        if (renderStatistics.meshRebuildCount != 0)
            throw std::runtime_error("Benchmark scene triggered an unexpected Mesh rebuild");


        const bool measurementFrame = controller_.shouldRecordCurrentFrame();


        // Resources Must Remain Stable During Measurement

        if (measurementFrame &&
            (renderStatistics.gpuMeshUploadCount != 0 ||
            renderStatistics.gpuMeshReleaseCount != 0)) {

            throw std::runtime_error("Benchmark GPU resources changed during measurement");
        }


        // Store Only Measurement Frames

        if (measurementFrame) {
            collector_.recordFrame(
                frameIndex, cpuTimings, renderStatistics
            );
        }


        // Associate GPU Results With Their Original Frame IDs

        if (gpuResultAvailable) {
            collector_.attachGpuResult(
                completedGpuFrameId, completedGpuTiming
            );
        }


        // Advance the Benchmark State

        const BenchmarkRunPhase previousPhase = controller_.phase();

        controller_.commitFrame();


        if (previousPhase == BenchmarkRunPhase::Warmup &&
            controller_.phase() == BenchmarkRunPhase::Measurement) {

            std::cout << "[Benchmark] Warmup Completed"
                    << " | Frames " << controller_.warmupFramesCompleted()
                    << '\n';

            std::cout << "[Benchmark] Measurement Started"
                    << '\n';
        }


        if (controller_.isCompleted()) {
            std::cout << "[Benchmark] Measurement Completed"
                    << " | Frames " << controller_.measurementFramesCompleted()
                    << '\n';

            std::cout << "[Benchmark] Total Frames "
                    << controller_.totalFramesCompleted()
                    << '\n';

            return true;
        }

    return false;
    }
    bool BenchmarkSession::isCompleted() const {
        return controller_.isCompleted();
    }

    void BenchmarkSession::finish( const std::function<bool(std::uint64_t&, BenchmarkGpuTiming&)>& tryPopGpuTiming ) {
        // Finalize Completed Benchmark

        if (controller_.isCompleted()) {
            if (!collector_.allCpuFramesRecorded())
                throw std::runtime_error("Benchmark CPU sample collection is incomplete");


            // Drain Outstanding GPU Queries After Measurement

            if (controller_.configuration().backend == BenchmarkBackend::OpenGL &&
                collector_.gpuUnresolvedCount() > 0) {

                const auto deadline = std::chrono::steady_clock::now() +
                                    std::chrono::milliseconds(250);

                while (collector_.gpuUnresolvedCount() > 0 &&
                    std::chrono::steady_clock::now() < deadline) {

                    std::uint64_t completedFrameId = 0;
                    BenchmarkGpuTiming completedTiming{};

                    if (tryPopGpuTiming(completedFrameId, completedTiming)) {
                        collector_.attachGpuResult(completedFrameId, completedTiming);
                    } else {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                }
            }


            // Collection Summary

            const std::vector<BenchmarkFrameSample>& samples = collector_.samples();

            std::cout << "[Benchmark][Collection]"
                    << " | CPU Samples " << samples.size()
                    << " / " << controller_.configuration().measurementFrameCount
                    << '\n';


            if (!samples.empty()) {
                std::cout << "[Benchmark][Collection]"
                        << " | First Frame " << samples.front().frameId
                        << " | Last Frame " << samples.back().frameId
                        << '\n';
            }


            if (controller_.configuration().backend == BenchmarkBackend::OpenGL) {
                std::cout << "[Benchmark][Collection]"
                        << " | GPU Available " << collector_.gpuAvailableCount()
                        << " | GPU Unresolved " << collector_.gpuUnresolvedCount()
                        << '\n';

                if (collector_.gpuUnresolvedCount() > 0) {
                    std::cout << "[Benchmark][Warning] Some GPU results were not collected"
                            << '\n';
                }
            } else {
                std::cout << "[Benchmark][Collection]"
                        << " | GPU NotApplicable (Software Backend)"
                        << '\n';
            }


        }

        if (!controller_.isCompleted()) {
            std::cout << "[Benchmark] Run Aborted"
                    << " | Completed Frames " << controller_.totalFramesCompleted()
                    << '\n';
        }

        if (controller_.isCompleted()) {
            std::cout << "[Benchmark] Frame Sample Collection Completed" << '\n';
            BenchmarkSummaryPrinter::print(std::cout, collector_);

            const auto csvPath = BenchmarkCsvWriter::writeToDirectory(
                collector_, "benchmark_results"
            );

            std::cout << "[Benchmark][CSV] Saved " << csvPath.string()
                    << " | Frames " << collector_.samples().size()
                    << '\n';
        }

    }
}
