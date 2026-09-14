#include "Window.h"

#include <GLFW/glfw3.h>

#include <stdexcept>


namespace
{

int toGLFWKey(ve::Key key)
{
    switch (key)
    {
        case ve::Key::Escape:
            return GLFW_KEY_ESCAPE;

        case ve::Key::W:
            return GLFW_KEY_W;

        case ve::Key::A:
            return GLFW_KEY_A;

        case ve::Key::S:
            return GLFW_KEY_S;

        case ve::Key::D:
            return GLFW_KEY_D;

        case ve::Key::Space:
            return GLFW_KEY_SPACE;
    }

    return GLFW_KEY_UNKNOWN;
}

}


namespace ve
{

Window::Window(
    int width,
    int height,
    const std::string& title
)
    : handle_(nullptr),
      width_(width),
      height_(height)
{
    if (!glfwInit())
    {
        throw std::runtime_error(
            "Failed to initialize GLFW"
        );
    }


    // OpenGL / Vulkan 등의 그래픽 API Context를
    // GLFW가 생성하지 않도록 한다.
    //
    // 즉 GLFW는 Window + Input + Event만 담당한다.
    glfwWindowHint(
        GLFW_CLIENT_API,
        GLFW_NO_API
    );


    handle_ = glfwCreateWindow(
        width_,
        height_,
        title.c_str(),
        nullptr,
        nullptr
    );


    if (!handle_)
    {
        glfwTerminate();

        throw std::runtime_error(
            "Failed to create GLFW window"
        );
    }
}


Window::~Window()
{
    if (handle_)
    {
        glfwDestroyWindow(handle_);
    }

    glfwTerminate();
}


bool Window::shouldClose() const
{
    return glfwWindowShouldClose(handle_);
}


void Window::requestClose()
{
    glfwSetWindowShouldClose(
        handle_,
        GLFW_TRUE
    );
}


void Window::pollEvents() const
{
    glfwPollEvents();
}


void Window::swapBuffers() const
{
    // 현재는 OpenGL Context를 사용하지 않으므로
    // swap할 OpenGL back buffer가 없다.
    //
    // 기존 Application 인터페이스를 유지하기 위해
    // 일단 빈 함수로 둔다.
}


bool Window::isKeyPressed(Key key) const
{
    return glfwGetKey(
        handle_,
        toGLFWKey(key)
    ) == GLFW_PRESS;
}


int Window::width() const
{
    return width_;
}


int Window::height() const
{
    return height_;
}

}