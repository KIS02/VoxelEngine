#include <iostream>
#include <stdexcept>
#include <string>

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
        const std::string shader = ve::DefaultTextureFragmentShader;
        require(shader.find("uniform sampler2DArrayShadow uShadowMap;") != std::string::npos,
                "OpenGL Depth Texture Array sampler is missing");
        require(shader.find("uniform mat4 uShadowMatrices[24];") != std::string::npos,
                "OpenGL Shadow Matrix array is missing");
        require(shader.find("uShadowBaseLayers[16]") != std::string::npos,
                "Scene Light to Shadow Layer mapping is missing");
        require(shader.find("uShadowBias") != std::string::npos,
                "Shadow Bias contract is missing");
        require(shader.find("evaluateShadow(i, normal, l)") != std::string::npos,
                "Multi-Light Shading does not sample its Shadow Map");
        require(shader.find("uLightAttenuation[i].x * intensityWeight") != std::string::npos,
                "Ambient term has been removed");
        require(shader.find("float visibility = evaluateShadow(i, normal, l);") != std::string::npos,
                "Shadow visibility is not evaluated per light");
        std::cout << "[PASS] GLSL 3.3 Shadow Map contract and per-Light shading\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
