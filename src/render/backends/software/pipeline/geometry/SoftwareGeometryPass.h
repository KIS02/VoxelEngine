#pragma once


namespace ve
{
    struct RenderFrame;
    class Rasterizer;
    class Viewport;


    class SoftwareGeometryPass {
        public:
            static void draw( const RenderFrame& frame, Rasterizer& rasterizer,
                              const Viewport& viewport );
    };
}
