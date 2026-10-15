
#pragma once

#include <cstdint>
#include <stdexcept>


namespace ve
{
    struct RenderFrame;
    struct BenchmarkCpuTimings;
    struct BenchmarkGpuTiming;
    struct BenchmarkRenderStatistics;


    struct RenderSize
    {
        int width = 0;
        int height = 0;
    };


    class IRenderBackend {
        private:
            BenchmarkCpuTimings* activeCpuTimings_ = nullptr;
            BenchmarkRenderStatistics* activeRenderStatistics_ = nullptr;

            std::uint64_t activeFrameId_ = 0;


        public:
            virtual ~IRenderBackend() = default;


            virtual void initialize() = 0;

            virtual RenderSize framebufferSize() const = 0;

            virtual void render( const RenderFrame& frame ) = 0;


            void renderMeasured( const RenderFrame& frame, BenchmarkCpuTimings& cpuTimings,
                                 std::uint64_t frameId = 0,
                                 BenchmarkRenderStatistics* renderStatistics = nullptr ) {
                if (activeCpuTimings_ != nullptr)
                    throw std::logic_error("Nested Benchmark render measurement is not allowed");

                activeCpuTimings_ = &cpuTimings;
                activeRenderStatistics_ = renderStatistics;
                activeFrameId_ = frameId;

                try {
                    render(frame);
                } catch (...) {
                    activeCpuTimings_ = nullptr;
                    activeRenderStatistics_ = nullptr;
                    activeFrameId_ = 0;
                    throw;
                }

                activeCpuTimings_ = nullptr;
                activeRenderStatistics_ = nullptr;
                activeFrameId_ = 0;
            }


            virtual bool tryPopGpuTiming( std::uint64_t&, BenchmarkGpuTiming& ) {
                return false;
            }


        protected:
            BenchmarkCpuTimings* measuredCpuTimings() const {
                return activeCpuTimings_;
            }

            BenchmarkRenderStatistics* measuredRenderStatistics() const {
                return activeRenderStatistics_;
            }

            std::uint64_t measuredFrameId() const {
                return activeFrameId_;
            }
    };
}
