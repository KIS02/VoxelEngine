#include <iostream>
#include <stdexcept>
#include <string_view>

#include "render/backends/opengl/pipeline/shading/DefaultTextureShaders.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }
}


int main() {
    try {
        const std::string_view shader = ve::DefaultTextureFragmentShader;
        require(shader.find("uniform int uHdrOutput;") != std::string_view::npos,
                "HDR Shader Uniform is missing");
        require(shader.find("vec4(linearResult, alpha)") != std::string_view::npos,
                "HDR output must remain linear");
        require(shader.find("vec4(encodeSrgb(linearResult), alpha)") != std::string_view::npos,
                "Legacy SDR output must remain available");
        std::cout << "[PASS] OpenGL Linear HDR output and legacy sRGB contract\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
