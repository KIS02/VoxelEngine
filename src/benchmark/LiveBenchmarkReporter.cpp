#include "benchmark/LiveBenchmarkReporter.h"

#include <iostream>


namespace ve
{
    void LiveBenchmarkReporter::report( std::uint64_t frameIndex, const BenchmarkCpuTimings& cpuTimings,
                                        const BenchmarkRenderStatistics& statistics, double backendCallWallMs,
                                        bool gpuResultAvailable, std::uint64_t completedGpuFrameId,
                                        const BenchmarkGpuTiming& completedGpuTiming ) {
        // Normal Mode: CPU and Geometry Diagnostics Every 60 Frames

        if (frameIndex % 60 == 0) {
            std::cout << "[Benchmark][CPU]"
                    << " | Frame " << frameIndex
                    << " | Wall " << cpuTimings.frameWallMs << " ms"
                    << " | Events " << cpuTimings.eventPollMs << " ms"
                    << " | Input " << cpuTimings.inputMs << " ms"
                    << " | Update " << cpuTimings.updateMs << " ms"
                    << '\n';

            std::cout << "[Benchmark][Render]"
                    << " | Meshing " << cpuTimings.meshRebuildMs << " ms"
                    << " | Assembly " << cpuTimings.frameAssemblyMs << " ms"
                    << " | Work " << cpuTimings.rendererWorkMs << " ms"
                    << " | Present " << cpuTimings.presentMs << " ms"
                    << " | Backend+Present " << backendCallWallMs << " ms"
                    << '\n';

            std::cout << "[Benchmark][Geometry]"
                    << " | Chunks " << statistics.residentChunkCount
                    << " | Draws " << statistics.meshDrawsSubmitted
                    << " | Vertices " << statistics.meshVerticesReferenced
                    << " | Indices " << statistics.meshIndicesSubmitted
                    << " | Triangles " << statistics.trianglesSubmitted
                    << '\n';
        }


        // Normal Mode: Resource Diagnostics

        const bool resourceEvent =
            statistics.meshRebuildCount > 0 ||
            statistics.gpuMeshUploadCount > 0 ||
            statistics.gpuMeshReleaseCount > 0;

        if (resourceEvent || frameIndex % 60 == 0) {
            std::cout << "[Benchmark][Resources]"
                    << " | Frame " << frameIndex
                    << " | MeshRebuild " << statistics.meshRebuildCount
                    << " | GPUUploads " << statistics.gpuMeshUploadCount
                    << " | GPUReleases " << statistics.gpuMeshReleaseCount
                    << '\n';
        }


        // Normal Mode: Report Asynchronous GPU Results

        if (gpuResultAvailable &&
            completedGpuTiming.state == BenchmarkGpuTimingState::Available &&
            completedGpuFrameId % 60 == 0) {

            std::cout << "[Benchmark][GPU]"
                    << " | Original Frame " << completedGpuFrameId
                    << " | GPU Render " << completedGpuTiming.elapsedMs << " ms"
                    << '\n';
        }
    }
}
