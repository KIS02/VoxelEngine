#pragma once



namespace ve
{
    enum class RenderBackend
    {
        Software,
        OpenGL
    };
}

// CPU
/*
    main
    ↓
    Application
    ↓
    WindowClientApi::None
    ↓
    GLFW_NO_API
    ↓
    renderSoftware()
    ↓
    CPU VertexProcessor
    ↓
    CPU Rasterizer
    ↓
    DepthBuffer
    ↓
    FrameBuffer
    ↓
    Win32FramePresenter
*/

// GPU
/*
    main
    ↓
    Application
    ↓
    WindowClientApi::OpenGL
    ↓
    OpenGL Context
    ↓
    renderOpenGL()
    ↓
    OpenGLRenderer
    ↓
    GPU
    ↓
    GPU Back Buffer
    ↓
    glfwSwapBuffers()
*/