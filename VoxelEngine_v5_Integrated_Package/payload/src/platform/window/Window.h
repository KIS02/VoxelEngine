
#pragma once

#include "input/MouseTracker.h"


struct GLFWwindow;


namespace ve
{
    enum class Key
    {
        Escape,
        F3,
        P,
        W,
        A,
        S,
        D,
        Space,
        LeftShift
    };


    enum class MouseButton
    {
        Left,
        Right
    };


    enum class WindowClientApi
    {
        None,
        OpenGL
    };


    class Window {
        private:
            WindowClientApi clientApi_;

            GLFWwindow* handle_ = nullptr;

            int width_ = 0;
            int height_ = 0;

            int requestedSwapInterval_ = -1;

            MouseTracker mouseTracker_;

            bool cursorCaptured_ = false;


        public:
            Window( int width, int height, const char* title, WindowClientApi clientApi );

            ~Window();


            Window( const Window& ) = delete;
            Window& operator=( const Window& ) = delete;


            bool shouldClose() const;
            void requestClose();

            void pollEvents();
            void swapBuffers();

            void setSwapInterval( int interval );
            int requestedSwapInterval() const;

            bool isKeyPressed( Key key ) const;
            bool isMouseButtonPressed( MouseButton button ) const;

            void setCursorCaptured( bool captured );
            bool isCursorCaptured() const;

            MouseDelta mouseDelta() const;

            int width() const;
            int height() const;

            void* nativeHandle() const;
    };
}
