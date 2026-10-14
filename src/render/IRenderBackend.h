#pragma once



namespace ve
{
    struct RenderFrame;



    struct RenderSize
    {
        int width = 0;
        int height = 0;
    };



    class IRenderBackend {
        public:
            virtual ~IRenderBackend() = default;


            virtual void initialize() = 0;

            virtual RenderSize framebufferSize() const = 0;

            virtual void render( const RenderFrame& frame ) = 0;
    };
}