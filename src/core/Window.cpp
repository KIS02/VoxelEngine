#include "Window.h"

#define GLFW_EXPOSE_NATIVE_WIN32

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <stdexcept>



namespace
{
    int toGLFWKey( ve::Key key ) {
        switch (key) {
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

            case ve::Key::LeftShift:
                return GLFW_KEY_LEFT_SHIFT;
        }


        return GLFW_KEY_UNKNOWN;
    }



    int toGLFWMouseButton( ve::MouseButton button ) {
        switch (button) {
            case ve::MouseButton::Left:
                return GLFW_MOUSE_BUTTON_LEFT;

            case ve::MouseButton::Right:
                return GLFW_MOUSE_BUTTON_RIGHT;
        }


        return -1;
    }
}



namespace ve
{
    Window::Window( int width, int height, const char* title, WindowClientApi clientApi )
        : clientApi_(clientApi), handle_(nullptr), width_(width), height_(height) {

        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }



        if (clientApi_ == WindowClientApi::OpenGL) {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_DEPTH_BITS, 24);
        }
        else {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        }



        handle_ = glfwCreateWindow(width, height, title, nullptr, nullptr);

        if (!handle_) {
            glfwTerminate();

            throw std::runtime_error("Failed to create GLFW window");
        }



        if (clientApi_ == WindowClientApi::OpenGL) {
            glfwMakeContextCurrent(handle_);
            glfwSwapInterval(1);
        }
    }



    Window::~Window() {
        if (handle_) {
            glfwDestroyWindow(handle_);
        }

        glfwTerminate();
    }



    bool Window::shouldClose() const {
        return glfwWindowShouldClose(handle_) == GLFW_TRUE;
    }



    void Window::requestClose() {
        glfwSetWindowShouldClose(handle_, GLFW_TRUE);
    }



    void Window::pollEvents() {
        glfwPollEvents();



        // Release Cursor Capture When the Window Loses Focus

        if (glfwGetWindowAttrib(handle_, GLFW_FOCUSED) != GLFW_TRUE) {
            if (cursorCaptured_) {
                setCursorCaptured(false);
            }
            else {
                mouseTracker_.reset();
            }

            return;
        }



        // Ignore Mouse Movement While the Cursor Is Free

        if (!cursorCaptured_) {
            mouseTracker_.reset();
            return;
        }



        // Read the Virtual Cursor Position in FPS Mode

        double mouseX = 0.0;
        double mouseY = 0.0;

        glfwGetCursorPos(handle_, &mouseX, &mouseY);



        mouseTracker_.sample(mouseX, mouseY);
    }



    void Window::swapBuffers() {
        if (clientApi_ == WindowClientApi::OpenGL) {
            glfwSwapBuffers(handle_);
        }
    }



    bool Window::isKeyPressed( Key key ) const {
        if (glfwGetWindowAttrib(handle_, GLFW_FOCUSED) != GLFW_TRUE) {
            return false;
        }


        const int glfwKey = toGLFWKey(key);

        if (glfwKey == GLFW_KEY_UNKNOWN) {
            return false;
        }


        return glfwGetKey(handle_, glfwKey) == GLFW_PRESS;
    }



    bool Window::isMouseButtonPressed( MouseButton button ) const {
        if (glfwGetWindowAttrib(handle_, GLFW_FOCUSED) != GLFW_TRUE) {
            return false;
        }


        const int glfwButton = toGLFWMouseButton(button);

        if (glfwButton < 0) {
            return false;
        }


        return glfwGetMouseButton(handle_, glfwButton) == GLFW_PRESS;
    }



    void Window::setCursorCaptured( bool captured ) {
        // Avoid Reconfiguring GLFW Every Frame

        if (cursorCaptured_ == captured) {
            return;
        }



        const int cursorMode = captured
            ? GLFW_CURSOR_DISABLED
            : GLFW_CURSOR_NORMAL;



        glfwSetInputMode(handle_, GLFW_CURSOR, cursorMode);

        cursorCaptured_ = captured;



        // Prevent a Sudden Camera Rotation After Mode Changes

        mouseTracker_.reset();
    }



    bool Window::isCursorCaptured() const {
        return cursorCaptured_;
    }



    MouseDelta Window::mouseDelta() const {
        return mouseTracker_.delta();
    }



    int Window::width() const {
        return width_;
    }



    int Window::height() const {
        return height_;
    }



    void* Window::nativeHandle() const {
        return glfwGetWin32Window(handle_);
    }
}