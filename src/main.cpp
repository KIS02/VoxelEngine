
#include <exception>
#include <iostream>
#include <string>

#include "core/Application.h"
#include "render/api/RenderBackend.h"


int main( int argc, char* argv[] ) {
    ve::RenderBackend backend = ve::RenderBackend::Software;
    bool benchmarkMode = false;


    // Parse Command-line Arguments

    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];

        if (argument == "--software")
            backend = ve::RenderBackend::Software;
        else if (argument == "--opengl")
            backend = ve::RenderBackend::OpenGL;
        else if (argument == "--benchmark")
            benchmarkMode = true;
        else if (argument == "--play")
            benchmarkMode = false;
        else {
            std::cerr << "[Application] Unknown argument: "
                      << argument << '\n';

            std::cerr << "Usage: VoxelEngine "
                      << "[--software | --opengl] "
                      << "[--play | --benchmark]\n";

            return 1;
        }
    }


    // Initialize and Run the Engine

    try {
        ve::Application application(backend);

        application.run(benchmarkMode);

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[Application] Fatal error: "
                  << error.what() << '\n';

        return 1;
    }
}
