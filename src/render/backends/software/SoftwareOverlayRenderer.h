#pragma once


namespace ve
{
    class FrameBuffer;
    class DepthBuffer;
    class Viewport;
    struct RenderFrame;
    struct CrosshairSettings;


    class SoftwareOverlayRenderer {
        public:
            static void drawSelectionOutline( FrameBuffer& frameBuffer, const DepthBuffer& depthBuffer,
                                              const Viewport& viewport, const RenderFrame& frame );

            static void drawCrosshair( FrameBuffer& frameBuffer, const CrosshairSettings& settings );
    };
}
