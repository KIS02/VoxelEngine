#pragma once


namespace ve
{
    struct RenderFrame;
    struct CrosshairSettings;


    class OpenGLOverlayRenderer {
        private:
            unsigned int outlineProgram_ = 0;
            unsigned int outlineVao_ = 0;
            unsigned int outlineVbo_ = 0;


        public:
            OpenGLOverlayRenderer() = default;
            ~OpenGLOverlayRenderer();

            OpenGLOverlayRenderer( const OpenGLOverlayRenderer& ) = delete;
            OpenGLOverlayRenderer& operator=( const OpenGLOverlayRenderer& ) = delete;

            void initialize();
            void drawSelectionOutline( const RenderFrame& frame );
            void drawCrosshair( const CrosshairSettings& settings, int width, int height );
    };
}
