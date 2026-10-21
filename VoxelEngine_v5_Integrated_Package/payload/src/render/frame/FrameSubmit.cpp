#include "render/frame/FrameBuilder.h"
#include "render/host/RenderHost.h"


namespace ve
{
    void FrameBuilder::submit( RenderHost& host, const RenderFrame& frame,
                               BenchmarkCpuTimings& cpuTimings, std::uint64_t frameId,
                               BenchmarkRenderStatistics* statistics ) {
        host.renderMeasured(frame, cpuTimings, frameId, statistics);
    }
}
