#include <charconv>
#include <cstdint>
#include <exception>
#include <iostream>
#include <string>
#include <system_error>

#include "core/Application.h"
#include "render/api/RenderBackend.h"


int main( int argc, char* argv[] ) {
    ve::RenderBackend backend = ve::RenderBackend::OpenGL;
    bool benchmarkMode = false;
    std::uint64_t worldSeed = 1337;


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
        else if (argument == "--seed") {
            if (i + 1 >= argc) {
                std::cerr << "[Application] Missing --seed value\n";
                return 1;
            }

            const std::string value = argv[++i];
            const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), worldSeed);

            if (error != std::errc{} || end != value.data() + value.size()) {
                std::cerr << "[Application] Invalid --seed value: " << value << '\n';
                return 1;
            }
        } else {
            std::cerr << "[Application] Unknown argument: " << argument << '\n';
            std::cerr << "Usage: VoxelEngine [--software | --opengl] "
                      << "[--play | --benchmark] [--seed unsigned_integer]\n";
            return 1;
        }
    }


    try {
        const bool proceduralTerrain = !benchmarkMode;

        ve::Application application(backend, proceduralTerrain, worldSeed);

        application.run(benchmarkMode);

        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[Application] Fatal error: " << error.what() << '\n';
        return 1;
    }
}