#pragma once

#include "Timer.h"
#include "Window.h"


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
};

}