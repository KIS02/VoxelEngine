#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <string>


namespace ve
{
    inline constexpr std::uint32_t BenchmarkSchemaVersion = 1;


    enum class BenchmarkBackend {
        Software,
        OpenGL
    };


    enum class BenchmarkGpuTimingState {
        NotApplicable,
        Pending,
        Available,
        Unsupported
    };


    struct BenchmarkRunConfiguration
    {
        BenchmarkBackend backend = BenchmarkBackend::Software;

        std::string scenarioName{};
        std::string buildConfiguration{};

        int framebufferWidth = 0;
        int framebufferHeight = 0;

        bool vsyncEnabled = false;
        bool crosshairEnabled = false;
        bool selectionOutlineEnabled = false;

        std::array<float, 3> cameraPosition{ 0.0f, 0.0f, 0.0f };
        std::array<float, 3> cameraForward{ 0.0f, 0.0f, -1.0f };

        std::uint32_t warmupFrameCount = 120;
        std::uint32_t measurementFrameCount = 600;


        bool isValid() const {
            if (scenarioName.empty() || buildConfiguration.empty())
                return false;

            if (framebufferWidth <= 0 || framebufferHeight <= 0)
                return false;

            if (warmupFrameCount == 0 || measurementFrameCount == 0)
                return false;

            for (float value : cameraPosition) {
                if (!std::isfinite(value))
                    return false;
            }

            float directionLengthSquared = 0.0f;

            for (float value : cameraForward) {
                if (!std::isfinite(value))
                    return false;

                directionLengthSquared += value * value;
            }

            return std::isfinite(directionLengthSquared) && directionLengthSquared > 0.0f;
        }
    };


    struct BenchmarkCpuTimings
    {
        // Frame loop wall time includes waiting in presentation calls.
        // Individual timings may be nested or overlap; do not sum them blindly.

        double frameWallMs = 0.0;
        double eventPollMs = 0.0;
        double inputMs = 0.0;
        double updateMs = 0.0;
        double meshRebuildMs = 0.0;
        double frameAssemblyMs = 0.0;
        double rendererWorkMs = 0.0;
        double presentMs = 0.0;


        bool isValid() const {
            const std::array<double, 8> values{
                frameWallMs, eventPollMs, inputMs, updateMs,
                meshRebuildMs, frameAssemblyMs, rendererWorkMs, presentMs
            };

            for (double value : values) {
                if (!std::isfinite(value) || value < 0.0)
                    return false;
            }

            return true;
        }
    };


    struct BenchmarkGpuTiming
    {
        BenchmarkGpuTimingState state = BenchmarkGpuTimingState::NotApplicable;
        double elapsedMs = 0.0;


        bool isValid() const {
            if (state == BenchmarkGpuTimingState::Available)
                return std::isfinite(elapsedMs) && elapsedMs >= 0.0;

            // Pending, unsupported, and not-applicable results have no measured value.
            return elapsedMs == 0.0;
        }
    };


    struct BenchmarkRenderStatistics
    {
        // Geometry submitted via RenderFrame; clipping can change rasterized triangles.
        // Overlay lines are excluded from triangle/index counts.

        std::uint64_t residentChunkCount = 0;
        std::uint64_t meshDrawsSubmitted = 0;
        std::uint64_t meshVerticesReferenced = 0;
        std::uint64_t meshIndicesSubmitted = 0;
        std::uint64_t trianglesSubmitted = 0;

        std::uint64_t meshRebuildCount = 0;
        std::uint64_t gpuMeshUploadCount = 0;
        std::uint64_t gpuMeshReleaseCount = 0;


        bool isValid() const {
            return meshIndicesSubmitted % 3 == 0 &&
                   trianglesSubmitted == meshIndicesSubmitted / 3;
        }
    };


    struct BenchmarkFrameSample
    {
        std::uint64_t frameId = 0;

        BenchmarkCpuTimings cpu{};
        BenchmarkGpuTiming gpu{};
        BenchmarkRenderStatistics render{};


        bool isValid( const BenchmarkRunConfiguration& configuration ) const {
            if (!configuration.isValid() || !cpu.isValid() ||
                !gpu.isValid() || !render.isValid()) {
                return false;
            }

            if (configuration.backend == BenchmarkBackend::Software) {
                return gpu.state == BenchmarkGpuTimingState::NotApplicable &&
                       render.gpuMeshUploadCount == 0 &&
                       render.gpuMeshReleaseCount == 0;
            }

            return gpu.state != BenchmarkGpuTimingState::NotApplicable;
        }
    };
}
