#include <iostream>
#include <stdexcept>
#include <string_view>

#include "render/backends/opengl/OpenGLRenderCommand.h"
#include "render/backends/opengl/pipeline/shading/DefaultTextureShaders.h"
#include "render/lighting/Lighting.h"


namespace
{
    void require( bool valid, const char* message ) {
        if (!valid)
            throw std::runtime_error(message);
    }

    void requireContains( std::string_view text, std::string_view token ) {
        require(text.find(token) != std::string_view::npos, "Multi-Light shader contract missing token");
    }
}


int main() {
    try {
        const std::string_view shader = ve::DefaultTextureFragmentShader;
        requireContains(shader, "#version 330 core");
        requireContains(shader, "uniform int uUseSceneLights;");
        requireContains(shader, "uniform int uSceneLightCount;");
        requireContains(shader, "uniform int uLightTypes[16];");
        requireContains(shader, "uniform vec4 uLightPositionRange[16];");
        requireContains(shader, "uniform vec4 uLightEmissionInnerCos[16];");
        requireContains(shader, "uniform vec4 uLightColorIntensity[16];");
        requireContains(shader, "uniform vec4 uLightAttenuation[16];");
        requireContains(shader, "for (int i = 0; i < uSceneLightCount; ++i)");
        requireContains(shader, "vec3 linearResult = ambient + diffuse + specularColor;");
        requireContains(shader, "vec4(encodeSrgb(linearResult), alpha)");
        require(ve::MaxSceneLights == 16, "C++/GLSL light capacity mismatch");
        ve::OpenGLDrawCommand command{};
        require(command.sceneLighting == nullptr, "Legacy command must not use Scene Lights implicitly");
        std::cout << "[PASS] Multi-Light Shader uniforms, loop, capacity and legacy contract\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
