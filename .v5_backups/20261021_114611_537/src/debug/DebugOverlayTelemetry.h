
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "benchmark/BenchmarkTypes.h"


namespace ve
{
    struct DebugOverlayScene
    {
        BenchmarkBackend backend = BenchmarkBackend::Software;

        std::array<float, 3> cameraPosition{};
        std::array<int, 3> chunkPosition{};

        std::string targetBlock = "None";
    };


    class DebugOverlayTelemetry {
        private:
            static constexpr std::size_t HistorySize = 60;

            std::array<double, HistorySize> frameHistory_{};
            std::size_t historyCount_ = 0;
            std::size_t nextHistoryIndex_ = 0;
            double frameTimeSumMs_ = 0.0;

            BenchmarkCpuTimings latestCpu_{};
            BenchmarkRenderStatistics latestRender_{};
            std::optional<double> latestGpuMs_{};


        public:
            void recordFrame( const BenchmarkCpuTimings& cpu, const BenchmarkRenderStatistics& render ) {
                if (!cpu.isValid() || !render.isValid())
                    throw std::invalid_argument("Invalid Debug Overlay frame measurements");

                latestCpu_ = cpu;
                latestRender_ = render;

                if (cpu.frameWallMs <= 0.0)
                    return;

                if (historyCount_ == HistorySize)
                    frameTimeSumMs_ -= frameHistory_[nextHistoryIndex_];
                else
                    ++historyCount_;

                frameHistory_[nextHistoryIndex_] = cpu.frameWallMs;
                frameTimeSumMs_ += cpu.frameWallMs;
                nextHistoryIndex_ = (nextHistoryIndex_ + 1) % HistorySize;
            }

            void observeGpuTiming( const BenchmarkGpuTiming& timing ) {
                if (!timing.isValid())
                    throw std::invalid_argument("Invalid Debug Overlay GPU timing");

                if (timing.state == BenchmarkGpuTimingState::Available)
                    latestGpuMs_ = timing.elapsedMs;
            }

            double averageFrameMs() const {
                if (historyCount_ == 0)
                    return 0.0;

                return frameTimeSumMs_ / static_cast<double>(historyCount_);
            }

            double estimatedFps() const {
                const double meanMs = averageFrameMs();
                return meanMs > 0.0 ? 1000.0 / meanMs : 0.0;
            }

            std::size_t frameSampleCount() const {
                return historyCount_;
            }

            std::vector<std::string> makeLines( const DebugOverlayScene& scene ) const {
                std::vector<std::string> lines;
                lines.reserve(12);

                std::ostringstream line;
                line << std::fixed << std::setprecision(2);

                lines.push_back("VOXEL ENGINE [F3]");

                line << "FPS (AVG 60): " << estimatedFps();
                lines.push_back(line.str());
                line.str(""); line.clear();

                line << "FRAME: " << latestCpu_.frameWallMs << " MS";
                lines.push_back(line.str());
                line.str(""); line.clear();

                line << "CPU RENDER: " << latestCpu_.rendererWorkMs << " MS";
                lines.push_back(line.str());
                line.str(""); line.clear();

                line << "PRESENT: " << latestCpu_.presentMs << " MS";
                lines.push_back(line.str());
                line.str(""); line.clear();

                if (scene.backend == BenchmarkBackend::OpenGL) {
                    if (latestGpuMs_.has_value())
                        line << "GPU RENDER: " << *latestGpuMs_ << " MS";
                    else
                        line << "GPU RENDER: PENDING";
                } else {
                    line << "GPU RENDER: N/A";
                }

                lines.push_back(line.str());
                line.str(""); line.clear();

                line << "XYZ: " << scene.cameraPosition[0] << " / "
                     << scene.cameraPosition[1] << " / " << scene.cameraPosition[2];
                lines.push_back(line.str());
                line.str(""); line.clear();

                line << "CHUNK: " << scene.chunkPosition[0] << " / "
                     << scene.chunkPosition[1] << " / " << scene.chunkPosition[2];
                lines.push_back(line.str());
                line.str(""); line.clear();

                lines.push_back("TARGET: " + scene.targetBlock);
                lines.push_back(scene.backend == BenchmarkBackend::OpenGL
                    ? "BACKEND: OPENGL" : "BACKEND: SOFTWARE");

                line << "CHUNKS: " << latestRender_.residentChunkCount
                     << "  DRAWS: " << latestRender_.meshDrawsSubmitted;
                lines.push_back(line.str());
                line.str(""); line.clear();

                line << "TRIANGLES: " << latestRender_.trianglesSubmitted;
                lines.push_back(line.str());

                return lines;
            }
    };
}
