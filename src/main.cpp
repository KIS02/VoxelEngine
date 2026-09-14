#include "core/Application.h"

#include <exception>
#include <iostream>


int main()
{
    try
    {
        ve::Application application;

        application.run();
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "Fatal Error: "
            << e.what()
            << '\n';

        return 1;
    }


    return 0;
}