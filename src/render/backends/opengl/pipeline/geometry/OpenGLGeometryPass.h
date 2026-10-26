#pragma once


namespace ve
{
    struct RenderFrame;
    struct BenchmarkRenderStatistics;
    class OpenGLRenderer;
    class OpenGLResourceCache;


    class OpenGLGeometryPass {
        public:
            static void draw( const RenderFrame& frame, OpenGLRenderer& renderer,
                              OpenGLResourceCache& resources, BenchmarkRenderStatistics* statistics );
    };
}
