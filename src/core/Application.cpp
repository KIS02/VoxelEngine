#include "Application.h"

#include <iostream>


namespace ve
{

Application::Application()
    :
    window_(
        800,
        600,
        "Voxel Engine"
    )
{
}


void Application::run()
{
    std::cout
        << "Voxel Engine Started"
        << '\n';


    while (!window_.shouldClose())
    {
        window_.pollEvents();


        const float deltaTime =
            timer_.tick();


        processInput();


        update(deltaTime);


        render();


        window_.swapBuffers();
    }


    std::cout
        << "Voxel Engine Shutdown"
        << '\n';
}


void Application::processInput()
{
    if (window_.isKeyPressed(Key::Escape))
    {
        window_.requestClose();
    }
}


void Application::update(float deltaTime)
{
    // 아직 Game Logic 없음.

    (void)deltaTime;
}


void Application::render()
{
    // 아직 Renderer 없음.
}

}