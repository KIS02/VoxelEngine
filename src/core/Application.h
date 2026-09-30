#pragma once

#include "core/Timer.h"
#include "core/Window.h"

#include "render/FrameBuffer.h"
#include "platform/windows/Win32FramePresenter.h"

#include "render/FrameBuffer.h"
#include "render/Rasterizer.h"
#include "render/DepthBuffer.h"
#include "render/Viewport.h"

namespace ve
{

class Application
{
public:

    Application();

    void run();


private:


    void processInput();

    void update(float deltaTime);

    void render();


private:

    Window window_;

    Timer timer_;

    FrameBuffer frameBuffer_;

    DepthBuffer depthBuffer_;

    Rasterizer rasterizer_;

    Viewport viewport_;

    Win32FramePresenter presenter_;
};

}