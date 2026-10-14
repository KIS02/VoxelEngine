#pragma once

#include "render/IRenderBackend.h"



namespace ve
{
    class FrameBuffer;
    class DepthBuffer;
    class Rasterizer;
    class Viewport;
    class Win32FramePresenter;
    class Texture2D;



    class SoftwareRenderBackend final : public IRenderBackend {
        private:
            FrameBuffer& frameBuffer_;
            DepthBuffer& depthBuffer_;
            Rasterizer& rasterizer_;
            Viewport& viewport_;
            Win32FramePresenter& presenter_;

            const Texture2D& texture_;


        public:
            SoftwareRenderBackend( FrameBuffer& frameBuffer, DepthBuffer& depthBuffer,
                                   Rasterizer& rasterizer, Viewport& viewport,
                                   Win32FramePresenter& presenter, const Texture2D& texture );


            void initialize() override;

            RenderSize framebufferSize() const override;

            void render( const RenderFrame& frame ) override;
    };
}