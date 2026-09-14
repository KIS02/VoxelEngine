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
    :
    handle_(nullptr),
    width_(width),
    height_(height)
{
    if (!glfwInit())
    {
        throw std::runtime_error(
            "Failed to initialize GLFW"
        );
    }


    glfwWindowHint(
        GLFW_CONTEXT_VERSION_MAJOR,
        3
    );

    glfwWindowHint(
        GLFW_CONTEXT_VERSION_MINOR,
        3
    );

    glfwWindowHint(
        GLFW_OPENGL_PROFILE,
        GLFW_OPENGL_CORE_PROFILE
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


    glfwMakeContextCurrent(handle_);


    // VSync
    glfwSwapInterval(1);
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
    glfwSwapBuffers(handle_);
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