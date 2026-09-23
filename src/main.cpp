#include "core/Application.h"
#include "math/Mat4.h"
#include "math/Vec4.h"
#include "math/Transform.h"

#include "math/MathUtil.h"

#include <exception>
#include <iostream>

int startApplication();

int main()
{

    return startApplication();
}

int startApplication()
{
    std::cout
        << "\nStarting application...\n\n";

    try {
        ve::Application application;

        application.run();

        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr
            << "Fatal Error: "
            << e.what()
            << '\n';

        return 1;
    }
}