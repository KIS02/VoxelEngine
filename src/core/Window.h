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


class Window
{
public:

    Window(
        int width,
        int height,
        const std::string& title
    );

    ~Window();


    Window(const Window&) = delete;

    Window& operator=(const Window&) = delete;


    bool shouldClose() const;

    void requestClose();


    void pollEvents() const;

    void swapBuffers() const;


    bool isKeyPressed(Key key) const;


    int width() const;

    int height() const;


private:

    GLFWwindow* handle_;

    int width_;

    int height_;
};

}