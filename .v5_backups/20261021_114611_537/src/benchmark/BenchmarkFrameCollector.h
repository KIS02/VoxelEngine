
#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>

#include "benchmark/BenchmarkTypes.h"


namespace ve
{
    class BenchmarkFrameCollector {
        private:
            BenchmarkRunConfiguration configuration_{};
            std::vector<BenchmarkFrameSample> samples_{};

            std::uint64_t firstFrameId_ = 0;
            bool started_ = false;


        public:
            void begin( const BenchmarkRunConfiguration& configuration, std::uint64_t firstFrameId ) {
                if (started_)
                    throw std::logic_error("BenchmarkFrameCollector is already started");

                if (!configuration.isValid())
                    throw std::invalid_argument("Invalid benchmark collection configuration");

                const std::uint64_t count = configuration.measurementFrameCount;

                if (firstFrameId == 0 ||
                    firstFrameId > std::numeric_limits<std::uint64_t>::max() - count + 1)
                    throw std::invalid_argument("Invalid first measurement Frame ID");

                configuration_ = configuration;
                firstFrameId_ = firstFrameId;
                samples_.clear();
                samples_.reserve(configuration.measurementFrameCount);
                started_ = true;
            }

            void reset() {
                configuration_ = BenchmarkRunConfiguration{};
                samples_.clear();
                firstFrameId_ = 0;
                started_ = false;
            }


            void recordFrame( std::uint64_t frameId, const BenchmarkCpuTimings& cpu,
                              const BenchmarkRenderStatistics& render ) {
                if (!started_)
                    throw std::logic_error("BenchmarkFrameCollector has not started");

                if (samples_.size() >= configuration_.measurementFrameCount)
                    throw std::logic_error("All measurement frames have already been recorded");

                const std::uint64_t expectedFrameId = firstFrameId_ + samples_.size();

                if (frameId != expectedFrameId)
                    throw std::invalid_argument("Benchmark Frame IDs must be consecutive");

                BenchmarkFrameSample sample{};
                sample.frameId = frameId;
                sample.cpu = cpu;
                sample.render = render;

                sample.gpu.state = configuration_.backend == BenchmarkBackend::OpenGL
                    ? BenchmarkGpuTimingState::Pending
                    : BenchmarkGpuTimingState::NotApplicable;

                if (!sample.isValid(configuration_))
                    throw std::invalid_argument("Invalid benchmark frame sample");

                samples_.push_back(sample);
            }


            bool attachGpuResult( std::uint64_t frameId, const BenchmarkGpuTiming& timing ) {
                if (!started_)
                    throw std::logic_error("BenchmarkFrameCollector has not started");

                if (configuration_.backend != BenchmarkBackend::OpenGL || frameId < firstFrameId_)
                    return false;

                const std::uint64_t index = frameId - firstFrameId_;

                if (index >= samples_.size())
                    return false;

                if (timing.state != BenchmarkGpuTimingState::Available || !timing.isValid())
                    throw std::invalid_argument("Only valid, completed GPU timing results may be attached");

                BenchmarkFrameSample& sample = samples_[static_cast<std::size_t>(index)];

                if (sample.gpu.state != BenchmarkGpuTimingState::Pending)
                    throw std::logic_error("GPU timing result already attached");

                sample.gpu = timing;
                return true;
            }


            bool allCpuFramesRecorded() const {
                return started_ && samples_.size() == configuration_.measurementFrameCount;
            }

            std::size_t gpuAvailableCount() const {
                std::size_t count = 0;

                for (const BenchmarkFrameSample& sample : samples_) {
                    if (sample.gpu.state == BenchmarkGpuTimingState::Available)
                        ++count;
                }

                return count;
            }

            std::size_t gpuUnresolvedCount() const {
                std::size_t count = 0;

                for (const BenchmarkFrameSample& sample : samples_) {
                    if (sample.gpu.state == BenchmarkGpuTimingState::Pending)
                        ++count;
                }

                return count;
            }


            const std::vector<BenchmarkFrameSample>& samples() const {
                return samples_;
            }

            const BenchmarkRunConfiguration& configuration() const {
                if (!started_)
                    throw std::logic_error("BenchmarkFrameCollector has not started");

                return configuration_;
            }
    };
}
