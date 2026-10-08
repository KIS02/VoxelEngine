#pragma once

#include "core/Timer.h"
#include "core/Window.h"

#include "render/FrameBuffer.h"
#include "platform/windows/Win32FramePresenter.h"

#include "render/FrameBuffer.h"
#include "render/Rasterizer.h"
#include "render/DepthBuffer.h"
#include "render/Viewport.h"

#include "render/RenderBackend.h"
#include "render/opengl/OpenGLRenderer.h"



namespace ve
{

    class Application {
        public:

            explicit Application( RenderBackend backend );

            void run();


        private:


            void processInput();

            void update(float deltaTime);

            void render();

            void renderSoftware();

            void renderOpenGL();


        private:
            RenderBackend backend_;

            Window window_;

            Timer timer_;

            FrameBuffer frameBuffer_;

            DepthBuffer depthBuffer_;

            Rasterizer rasterizer_;

            Viewport viewport_;

            Win32FramePresenter presenter_;

            OpenGLRenderer openGLRenderer_;
    };

}