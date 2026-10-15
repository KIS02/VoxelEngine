#include "core/Application.h"

#include "render/RenderBackend.h"


#include <iostream>
#include <string_view>

#include "core/Application.h"

#include "render/RenderBackend.h"


int main( int argc, char* argv[] ) {
    ve::RenderBackend backend = ve::RenderBackend::Software;
    bool benchmarkMode = false;

    for (int i = 1; i < argc; ++i) {
        const std::string_view argument(argv[i]);

        if (argument == "--software")
            backend = ve::RenderBackend::Software;
        else if (argument == "--opengl")
            backend = ve::RenderBackend::OpenGL;
        else if (argument == "--benchmark")
            benchmarkMode = true;
        else if (argument == "--play")
            benchmarkMode = false;
        else if (argument == "--help") {
            std::cout << "Usage: VoxelEngine [--software | --opengl] [--benchmark | --play]\n";
            return 0;
        } else {
            std::cerr << "Unknown argument: " << argument << '\n';
            return 1;
        }
    }


    ve::Application application(backend);

    application.run(benchmarkMode);

    return 0;
}
