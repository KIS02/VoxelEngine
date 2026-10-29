#include <iostream>
#include <stdexcept>
#include <string_view>

#include "render/backends/opengl/pipeline/shading/DefaultTextureShaders.h"
#include "render/frame/RenderFrame.h"


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
        for (const std::string_view token : {
                "uniform int uShadingModel;", "uniform sampler2D uNormalMap;",
                "uniform sampler2D uRoughnessMap;", "uniform sampler2D uMetallicMap;",
                "uniform sampler2D uAoMap;", "uHasNormalMap", "uRoughnessFactor",
                "uMetallicFactor", "uAoStrength", "uNormalStrength",
                "evaluateShadow(i, normal, l)", "vec4(pbrColor, alpha)",
                "vec4(encodeSrgb(pbrColor), alpha)", "vec4(linearResult, alpha)" }) {
            require(shader.find(token) != std::string_view::npos, "PBR shader contract token missing");
        }
        require(ve::RenderFrame{}.shadingModel == ve::ShadingModel::BlinnPhong,
                "Legacy default shading must remain Blinn-Phong");
        std::cout << "[PASS] PBR shader maps, shadow, HDR and legacy defaults\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
