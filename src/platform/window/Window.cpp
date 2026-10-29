
#include "Window.h"

#define GLFW_EXPOSE_NATIVE_WIN32

#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <cstdlib>
#include <stdexcept>


namespace
{
    int toGLFWKey( ve::Key key ) {
        switch (key) {
            case ve::Key::Escape:
                return GLFW_KEY_ESCAPE;

            case ve::Key::P:
                return GLFW_KEY_P;

            case ve::Key::G:
                return GLFW_KEY_G;

            case ve::Key::R:
                return GLFW_KEY_R;

            case ve::Key::M:
                return GLFW_KEY_M;

            case ve::Key::N:
                return GLFW_KEY_N;

            case ve::Key::F3:
                return GLFW_KEY_F3;

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
            case ve::Key::E: return GLFW_KEY_E;
            case ve::Key::Q: return GLFW_KEY_Q;
            case ve::Key::F: return GLFW_KEY_F;
            case ve::Key::Digit1: return GLFW_KEY_1;
            case ve::Key::Digit2: return GLFW_KEY_2;
            case ve::Key::Digit3: return GLFW_KEY_3;
            case ve::Key::Digit4: return GLFW_KEY_4;
            case ve::Key::Digit5: return GLFW_KEY_5;
            case ve::Key::Digit6: return GLFW_KEY_6;
            case ve::Key::Digit7: return GLFW_KEY_7;
            case ve::Key::Digit8: return GLFW_KEY_8;
            case ve::Key::Digit9: return GLFW_KEY_9;
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

        if (!glfwInit())
            throw std::runtime_error("Failed to initialize GLFW");


        if (clientApi_ == WindowClientApi::OpenGL) {
            glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
            glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
            glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
            glfwWindowHint(GLFW_DEPTH_BITS, 24);
        } else {
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        }


        handle_ = glfwCreateWindow(width, height, title, nullptr, nullptr);

        if (!handle_) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }


        glfwSetWindowUserPointer(handle_, this);
        glfwSetScrollCallback(handle_, []( GLFWwindow* handle, double, double y ) {
            auto* window = static_cast<Window*>(glfwGetWindowUserPointer(handle));
            if (window != nullptr)
                window->scrollY_ += y;
        });

        if (clientApi_ == WindowClientApi::OpenGL) {
            glfwMakeContextCurrent(handle_);

            try {
                const char* disableVsync = std::getenv("VOXELENGINE_DISABLE_VSYNC");
                const bool uncapped = disableVsync != nullptr &&
                                      disableVsync[0] == '1' && disableVsync[1] == '\0';
                setSwapInterval(uncapped ? 0 : 1);
            } catch (...) {
                glfwDestroyWindow(handle_);
                handle_ = nullptr;
                glfwTerminate();
                throw;
            }
        }
    }


    Window::~Window() {
        if (handle_)
            glfwDestroyWindow(handle_);

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
            if (cursorCaptured_)
                setCursorCaptured(false);
            else
                mouseTracker_.reset();

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
        if (clientApi_ == WindowClientApi::OpenGL)
            glfwSwapBuffers(handle_);
    }


    void Window::setSwapInterval( int interval ) {
        if (clientApi_ != WindowClientApi::OpenGL)
            throw std::logic_error("Swap Interval requires an OpenGL Window");

        if (interval != 0 && interval != 1)
            throw std::invalid_argument("Swap Interval must be 0 or 1");

        if (glfwGetCurrentContext() != handle_)
            throw std::runtime_error("Window OpenGL context must be current");

        glfwSwapInterval(interval);
        requestedSwapInterval_ = interval;
    }


    Vec2 Window::cursorPosition() const {
        double x = 0.0;
        double y = 0.0;
        glfwGetCursorPos(handle_, &x, &y);
        int windowWidth = 0, windowHeight = 0, framebufferWidth = 0, framebufferHeight = 0;
        glfwGetWindowSize(handle_, &windowWidth, &windowHeight);
        glfwGetFramebufferSize(handle_, &framebufferWidth, &framebufferHeight);
        if (windowWidth > 0 && windowHeight > 0) {
            x *= static_cast<double>(framebufferWidth) / windowWidth;
            y *= static_cast<double>(framebufferHeight) / windowHeight;
        }
        return Vec2{ static_cast<float>(x), static_cast<float>(y) };
    }

    int Window::takeScrollSteps() {
        const int steps = static_cast<int>(scrollY_);
        scrollY_ -= steps;
        return steps;
    }

    int Window::requestedSwapInterval() const {
        return requestedSwapInterval_;
    }


    bool Window::isKeyPressed( Key key ) const {
        if (glfwGetWindowAttrib(handle_, GLFW_FOCUSED) != GLFW_TRUE)
            return false;

        const int glfwKey = toGLFWKey(key);

        if (glfwKey == GLFW_KEY_UNKNOWN)
            return false;

        return glfwGetKey(handle_, glfwKey) == GLFW_PRESS;
    }


    bool Window::isMouseButtonPressed( MouseButton button ) const {
        if (glfwGetWindowAttrib(handle_, GLFW_FOCUSED) != GLFW_TRUE)
            return false;

        const int glfwButton = toGLFWMouseButton(button);

        if (glfwButton < 0)
            return false;

        return glfwGetMouseButton(handle_, glfwButton) == GLFW_PRESS;
    }


    void Window::setCursorCaptured( bool captured ) {
        // Avoid Reconfiguring GLFW Every Frame

        if (cursorCaptured_ == captured)
            return;


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
