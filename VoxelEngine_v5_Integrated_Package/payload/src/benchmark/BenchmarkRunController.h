
#pragma once

#include <cstdint>
#include <stdexcept>

#include "benchmark/BenchmarkTypes.h"


namespace ve
{
    enum class BenchmarkRunPhase {
        Idle,
        Warmup,
        Measurement,
        Completed
    };


    class BenchmarkRunController {
        private:
            BenchmarkRunConfiguration configuration_{};
            BenchmarkRunPhase phase_ = BenchmarkRunPhase::Idle;

            std::uint32_t warmupFramesCompleted_ = 0;
            std::uint32_t measurementFramesCompleted_ = 0;
            std::uint64_t totalFramesCompleted_ = 0;


        public:
            void begin( const BenchmarkRunConfiguration& configuration ) {
                if (isRunning())
                    throw std::logic_error("Benchmark run is already active");

                if (!configuration.isValid())
                    throw std::invalid_argument("Invalid benchmark run configuration");

                configuration_ = configuration;
                warmupFramesCompleted_ = 0;
                measurementFramesCompleted_ = 0;
                totalFramesCompleted_ = 0;
                phase_ = BenchmarkRunPhase::Warmup;
            }

            void commitFrame() {
                if (phase_ == BenchmarkRunPhase::Warmup) {
                    ++warmupFramesCompleted_;
                    ++totalFramesCompleted_;

                    if (warmupFramesCompleted_ == configuration_.warmupFrameCount)
                        phase_ = BenchmarkRunPhase::Measurement;
                } else if (phase_ == BenchmarkRunPhase::Measurement) {
                    ++measurementFramesCompleted_;
                    ++totalFramesCompleted_;

                    if (measurementFramesCompleted_ == configuration_.measurementFrameCount)
                        phase_ = BenchmarkRunPhase::Completed;
                } else {
                    throw std::logic_error("Cannot commit a frame outside an active benchmark run");
                }
            }

            void reset() {
                configuration_ = BenchmarkRunConfiguration{};
                phase_ = BenchmarkRunPhase::Idle;
                warmupFramesCompleted_ = 0;
                measurementFramesCompleted_ = 0;
                totalFramesCompleted_ = 0;
            }

            BenchmarkRunPhase phase() const {
                return phase_;
            }

            bool isRunning() const {
                return phase_ == BenchmarkRunPhase::Warmup ||
                       phase_ == BenchmarkRunPhase::Measurement;
            }

            bool shouldRecordCurrentFrame() const {
                return phase_ == BenchmarkRunPhase::Measurement;
            }

            bool isCompleted() const {
                return phase_ == BenchmarkRunPhase::Completed;
            }

            const BenchmarkRunConfiguration& configuration() const {
                if (phase_ == BenchmarkRunPhase::Idle)
                    throw std::logic_error("Benchmark run has not been started");

                return configuration_;
            }

            std::uint32_t warmupFramesCompleted() const {
                return warmupFramesCompleted_;
            }

            std::uint32_t measurementFramesCompleted() const {
                return measurementFramesCompleted_;
            }

            std::uint64_t totalFramesCompleted() const {
                return totalFramesCompleted_;
            }
    };
}
