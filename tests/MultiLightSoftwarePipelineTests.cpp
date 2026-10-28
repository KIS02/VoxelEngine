#include <iostream>
#include <stdexcept>

#include "render/backends/software/pipeline/shading/SoftwareFragmentShading.h"
#include "render/lighting/Lighting.h"


namespace
{
    void require( bool valid, const char* message ) {
        if (!valid)
            throw std::runtime_error(message);
    }
}


int main() {
    try {
        ve::LightingSettings lighting{};
        ve::SceneLightData point{};
        point.type = ve::LightType::Point;
        point.position = ve::Vec3{ 0.0f, 0.0f, 2.0f };
        point.range = 5.0f;
        point.ambient = 0.0f;
        point.diffuse = 0.8f;
        lighting.sceneLights.push_back(point);

        ve::MaterialData material{};
        material.specularStrength = 0.2f;
        material.shininess = 12.0f;
        const ve::Color32 texel = ve::makeColor(100, 120, 150, 73);
        const ve::Vec3 normal{ 0.0f, 0.0f, 1.0f };
        const ve::Vec3 world{};
        const ve::Vec3 camera{ 0.0f, 0.0f, 5.0f };

        const ve::Color32 expected = ve::shadeColorMultiLight(
            texel, normal, world, camera, lighting, material
        );
        const ve::Color32 actual = ve::SoftwareFragmentShading::shade(
            texel, normal, world, &camera, &lighting.directional, &material, &lighting
        );
        require(actual == expected, "Software Fragment Stage did not select Multi-Light");

        lighting.sceneLights.clear();
        lighting.directional.ambient = 0.0f;
        lighting.directional.diffuse = 0.0f;
        const ve::Color32 dark = ve::SoftwareFragmentShading::shade(
            texel, normal, world, &camera, &lighting.directional, &material, &lighting
        );
        require(ve::getRed(dark) == 0 && ve::getGreen(dark) == 0 && ve::getBlue(dark) == 0,
                "No-light snapshot must stay dark");
        require(ve::getAlpha(dark) == 73, "Software alpha changed");
        std::cout << "[PASS] Software Fragment Multi-Light path and no-light state\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
