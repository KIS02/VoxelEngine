#pragma once

#include "benchmark/BenchmarkTypes.h"


namespace ve
{
    struct RenderFrame;
    class OpenGLRenderer;
    class OpenGLResourceCache;
    class OpenGLShadowPass;

    class OpenGLGeometryPass {
        public:
            static void draw( const RenderFrame& frame, OpenGLRenderer& renderer,
                              OpenGLResourceCache& resources, BenchmarkRenderStatistics* statistics,
                              const OpenGLShadowPass* shadows = nullptr, bool hdrOutput = false );
    };
}
