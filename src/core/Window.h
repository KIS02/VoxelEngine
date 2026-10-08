#pragma once

#include <string>


struct GLFWwindow;


namespace ve
{

enum class Key
{
    Escape,
    W,
    A,
    S,
    D,
    Space
};


enum class WindowClientApi
{
    None,
    OpenGL
};


class Window
{
    public:

        Window( int width, int height, const char* title, WindowClientApi clientApi );

        ~Window();


        Window(const Window&) = delete;

        Window& operator=(const Window&) = delete;


        bool shouldClose() const;

        void requestClose();


        void pollEvents() const;
        
        void swapBuffers();


        bool isKeyPressed(Key key) const;


        int width() const;

        int height() const;

        void* nativeHandle() const;

    private:
        WindowClientApi clientApi_;

        GLFWwindow* handle_;

        int width_;

        int height_;
    };

}