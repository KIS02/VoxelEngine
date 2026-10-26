#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string_view>

#include "assets/material/MaterialData.h"
#include "math/Color.h"
#include "render/backends/opengl/OpenGLRenderCommand.h"
#include "render/backends/opengl/pipeline/shading/DefaultTextureShaders.h"
#include "render/frame/RenderFrame.h"
#include "render/lighting/Lighting.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    bool near( float a, float b ) {
        return std::fabs(a - b) < 0.0001f;
    }


    void requireContains( std::string_view source, std::string_view expression,
                          const char* message ) {
        require(source.find(expression) != std::string_view::npos, message);
    }


    void testVertexShaderContract() {
        const std::string_view source = ve::DefaultTextureVertexShader;

        requireContains(source, "layout(location = 3) in vec3 aNormal;",
                        "Vertex Shader must receive Normal Attribute 3");

        requireContains(source, "out vec3 vWorldPosition;",
                        "Vertex Shader must output World Position");

        requireContains(source, "out vec3 vNormalWorld;",
                        "Vertex Shader must output World Normal");

        requireContains(source, "uModel * vec4(aPosition, 1.0)",
                        "Vertex Shader must calculate World Position");

        requireContains(source, "uNormalMatrix * aNormal",
                        "Vertex Shader must transform Normal");

        requireContains(source, "vWorldPosition = worldPosition.xyz;",
                        "World Position must be forwarded to Fragment Shader");

        std::cout << "[PASS] OpenGL Vertex Shader world-space data contract\n";
    }


    void testFragmentShaderContract() {
        const std::string_view source = ve::DefaultTextureFragmentShader;

        requireContains(source, "in vec3 vWorldPosition;",
                        "Fragment Shader must receive World Position");

        requireContains(source, "uniform vec3 uCameraPosition;",
                        "Camera Position Uniform is missing");

        requireContains(source, "uniform float uSpecularStrength;",
                        "Specular Strength Uniform is missing");

        requireContains(source, "uniform float uShininess;",
                        "Shininess Uniform is missing");

        requireContains(source, "uCameraPosition - vWorldPosition",
                        "View Vector calculation is missing");

        requireContains(source, "lightDirection + viewDirection",
                        "Halfway Vector calculation is missing");

        requireContains(source, "pow(alignment, uShininess)",
                        "Blinn-Phong shininess calculation is missing");

        requireContains(source, "sampled.rgb * decodeSrgb(uTint.rgb) * uDiffuseTint",
                        "Tint must be decoded once and combined in linear RGB");

        requireContains(source, "vec4(encodeSrgb(linearResult), alpha)",
                        "Final Alpha must be preserved");

        std::cout << "[PASS] OpenGL Fragment Shader specular data contract\n";
    }


    void testDrawCommandProperties() {
        ve::MaterialData matte{};
        matte.specularStrength = 0.0f;
        matte.shininess = 32.0f;

        ve::MaterialData glossy{};
        glossy.specularStrength = 0.8f;
        glossy.shininess = 64.0f;

        ve::OpenGLDrawCommand first{};
        ve::OpenGLDrawCommand second{};

        ve::RenderFrame frame{};
        frame.cameraPosition = ve::Vec3{ 1.0f, 2.0f, 3.0f };
        first.specularStrength = matte.specularStrength;
        first.shininess = matte.shininess;

        
        second.specularStrength = glossy.specularStrength;
        second.shininess = glossy.shininess;

        require(near(first.specularStrength, 0.0f),
                "Matte Draw Command properties incorrect");

        require(near(second.specularStrength, 0.8f) &&
                near(second.shininess, 64.0f),
                "Glossy Draw Command properties incorrect");

        require(near(frame.cameraPosition.x, 1.0f) &&
                near(frame.cameraPosition.y, 2.0f) &&
                near(frame.cameraPosition.z, 3.0f),
                "Camera Position must be provided by RenderFrame");

        std::cout << "[PASS] Independent Draw Command material properties\n";
    }


    void testCPUReferenceLighting() {
        ve::MaterialData material{};
        material.specularStrength = 0.4f;
        material.shininess = 32.0f;

        const ve::DirectionalLight light{
            ve::Vec3{ 0.0f, 1.0f, 0.0f }, 0.2f, 0.8f
        };

        const ve::Vec3 normal{ 0.0f, 1.0f, 0.0f };
        const ve::Vec3 surface{ 0.0f, 0.0f, 0.0f };
        const ve::Vec3 camera{ 0.0f, 5.0f, 0.0f };

        const float specular = ve::blinnPhongSpecular(
            normal, surface, camera, light, material
        );

        require(near(specular, 0.4f),
                "CPU reference Specular value incorrect");

        const ve::Color32 result = ve::shadeColorBlinnPhong(
            ve::makeColor(100, 120, 150, 73),
            normal, surface, camera, light, material
        );

        require(result == ve::makeColor(192, 202, 219, 73),
                "CPU reference final color incorrect");

        std::cout << "[PASS] Blinn-Phong CPU reference calculation\n";
    }
}


int main() {
    try {
        testVertexShaderContract();
        testFragmentShaderContract();
        testDrawCommandProperties();
        testCPUReferenceLighting();

        std::cout << "All OpenGLSpecularContract tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}