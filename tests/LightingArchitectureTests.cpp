#include <cmath>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string_view>

#include "render/lighting/Lighting.h"
#include "render/frame/RenderFrame.h"
#include "render/backends/software/RasterInterpolation.h"
#include "render/backends/opengl/OpenGLRenderCommand.h"
#include "render/backends/opengl/pipeline/shading/DefaultTextureShaders.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    bool near( float a, float b ) {
        return std::fabs(a - b) < 0.0001f;
    }


    void testSharedLightingSettings() {
        ve::RenderFrame frame{};

        require(near(frame.lighting.directional.ambient, 0.25f),
                "Shared Ambient setting is incorrect");

        require(near(frame.lighting.directional.diffuse, 0.75f),
                "Shared Diffuse setting is incorrect");

        ve::OpenGLDrawCommand command{};
        command.directionalLight = frame.lighting.directional;

        require(near(command.directionalLight.ambient, frame.lighting.directional.ambient),
                "OpenGL Draw did not receive shared Ambient");

        require(near(command.directionalLight.diffuse, frame.lighting.directional.diffuse),
                "OpenGL Draw did not receive shared Diffuse");

        std::cout << "[PASS] Shared directional lighting settings\n";
    }


    void testMaterialProperties() {
        ve::MaterialData material{};
        material.specularStrength = 0.4f;
        material.shininess = 16.0f;

        ve::OpenGLDrawCommand command{};
        command.specularStrength = material.specularStrength;
        command.shininess = material.shininess;

        require(near(command.specularStrength, 0.4f) &&
                near(command.shininess, 16.0f),
                "OpenGL Draw Material properties are incorrect");

        ve::validateMaterialLighting(material);
        ve::validateMaterialLighting(command.specularStrength, command.shininess);

        std::cout << "[PASS] Shared Material property contract\n";
    }


    void testCameraDependentSpecular() {
        ve::MaterialData material{};
        material.specularStrength = 0.4f;
        material.shininess = 16.0f;

        const ve::DirectionalLight light{
            ve::Vec3{ 0.0f, 1.0f, 0.0f }, 0.2f, 0.8f
        };

        const ve::Vec3 normal{ 0.0f, 1.0f, 0.0f };
        const ve::Vec3 position{};
        const ve::Vec3 frontCamera{ 0.0f, 5.0f, 0.0f };
        const ve::Vec3 sideCamera{ 5.0f, 0.0f, 0.0f };

        const float front = ve::blinnPhongSpecular(
            normal, position, frontCamera, light, material
        );

        const float side = ve::blinnPhongSpecular(
            normal, position, sideCamera, light, material
        );

        require(near(front, 0.4f) && front > side,
                "Specular must change with camera direction");

        const ve::Color32 color = ve::shadeColorBlinnPhong(
            ve::makeColor(100, 120, 150, 73),
            normal, position, frontCamera, light, material
        );

        require(color == ve::makeColor(192, 202, 219, 73),
                "Common Blinn-Phong RGB or Alpha is incorrect");

        std::cout << "[PASS] Common Blinn-Phong and camera-dependent highlights\n";
    }


    void testPerspectiveInterpolation() {
        ve::RasterVertex a{};
        ve::RasterVertex b{};
        ve::RasterVertex c{};

        a.inverseW = 1.0f;
        b.inverseW = 0.5f;
        c.inverseW = 0.25f;

        b.worldPositionOverW = ve::Vec3{ 2.0f, 0.0f, 0.0f };
        c.worldPositionOverW = ve::Vec3{ 0.0f, 2.0f, 0.0f };

        const ve::Vec3 result = ve::interpolatePerspectiveWorldPosition(
            a, b, c, ve::Vec3{ 0.25f, 0.25f, 0.5f }, 0.5f
        );

        require(near(result.x, 1.0f) && near(result.y, 2.0f) && near(result.z, 0.0f),
                "Perspective-correct World Position is incorrect");

        std::cout << "[PASS] Backend-specific perspective interpolation\n";
    }


    void testShaderContract() {
        const std::string_view vertex = ve::DefaultTextureVertexShader;
        const std::string_view fragment = ve::DefaultTextureFragmentShader;

        require(vertex.find("layout(location = 3) in vec3 aNormal") != std::string_view::npos,
                "Normal Attribute 3 is missing");

        require(vertex.find("vWorldPosition = worldPosition.xyz") != std::string_view::npos,
                "World Position must reach Fragment Shader");

        require(fragment.find("uCameraPosition - vWorldPosition") != std::string_view::npos,
                "View Vector calculation is missing");

        require(fragment.find("uSpecularStrength") != std::string_view::npos &&
                fragment.find("uShininess") != std::string_view::npos,
                "Material Uniforms are missing");

        require(fragment.find("pow(alignment, uShininess)") != std::string_view::npos,
                "Blinn-Phong calculation is missing");

        require(fragment.find("uLightingDebugMode") != std::string_view::npos,
                "OpenGL diagnostic mode is missing");

        std::cout << "[PASS] OpenGL Shader data contract\n";
    }
}


int main() {
    try {
        testSharedLightingSettings();
        testMaterialProperties();
        testCameraDependentSpecular();
        testPerspectiveInterpolation();
        testShaderContract();

        std::cout << "All LightingArchitecture tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}