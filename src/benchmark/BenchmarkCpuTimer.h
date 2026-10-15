
#pragma once

#include <chrono>
#include <stdexcept>


namespace ve
{
    class BenchmarkCpuStopwatch {
        private:
            using Clock = std::chrono::steady_clock;

            Clock::time_point startedAt_{};
            double accumulatedMs_ = 0.0;
            bool running_ = false;


        public:
            void start() {
                if (running_)
                    throw std::logic_error("BenchmarkCpuStopwatch is already running");

                startedAt_ = Clock::now();
                running_ = true;
            }

            void stop() {
                if (!running_)
                    throw std::logic_error("BenchmarkCpuStopwatch is not running");

                accumulatedMs_ += std::chrono::duration<double, std::milli>(Clock::now() - startedAt_).count();
                running_ = false;
            }

            void reset() {
                accumulatedMs_ = 0.0;
                running_ = false;
            }

            double elapsedMs() const {
                if (!running_)
                    return accumulatedMs_;

                return accumulatedMs_ +
                       std::chrono::duration<double, std::milli>(Clock::now() - startedAt_).count();
            }

            bool isRunning() const {
                return running_;
            }
    };


    class BenchmarkCpuScope {
        private:
            using Clock = std::chrono::steady_clock;

            double& destinationMs_;
            Clock::time_point startedAt_;


        public:
            explicit BenchmarkCpuScope( double& destinationMs )
                : destinationMs_(destinationMs), startedAt_(Clock::now()) { }

            ~BenchmarkCpuScope() {
                destinationMs_ += std::chrono::duration<double, std::milli>(Clock::now() - startedAt_).count();
            }

            BenchmarkCpuScope( const BenchmarkCpuScope& ) = delete;
            BenchmarkCpuScope& operator=( const BenchmarkCpuScope& ) = delete;
            BenchmarkCpuScope( BenchmarkCpuScope&& ) = delete;
            BenchmarkCpuScope& operator=( BenchmarkCpuScope&& ) = delete;
    };
}
