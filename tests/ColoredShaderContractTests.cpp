#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

#include "math/LinearRGB.h"
#include "render/backends/opengl/OpenGLRenderCommand.h"
#include "render/backends/opengl/pipeline/shading/DefaultTextureShaders.h"


namespace
{
    void requireContains( std::string_view text, std::string_view token ) {
        if (text.find(token) == std::string_view::npos)
            throw std::runtime_error("Shader contract missing: " + std::string(token));
    }
}


int main() {
    try {
        ve::OpenGLDrawCommand command{};
        command.diffuseTint = ve::LinearRGB{ 0.5f, 0.8f, 1.0f };
        command.specularColor = ve::LinearRGB{ 1.0f, 0.0f, 0.0f };
        command.directionalLight.color = ve::LinearRGB{ 1.0f, 0.5f, 0.3f };

        if (command.specularColor.r != 1.0f || command.directionalLight.color.g != 0.5f)
            throw std::runtime_error("Draw color settings mismatch");

        const std::string_view vertex = ve::DefaultTextureVertexShader;
        const std::string_view fragment = ve::DefaultTextureFragmentShader;

        requireContains(vertex, "vWorldPosition = worldPosition.xyz;");
        requireContains(fragment, "uniform vec3 uLightColor;");
        requireContains(fragment, "uniform vec3 uDiffuseTint;");
        requireContains(fragment, "uniform vec3 uSpecularColor;");
        requireContains(fragment, "uniform int uLightingDebugMode;");
        requireContains(fragment, "texture(uTexture, vUV)");
        requireContains(fragment, "pow(alignment, uShininess)");
        requireContains(fragment, "vec3 linearResult = ambient + diffuse + specularColor;");
        requireContains(fragment, "vec4(encodeSrgb(linearResult), alpha)");

        std::cout << "[PASS] Colored Shader uniforms and linear output contract\n";
        std::cout << "All ColoredShaderContract tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}