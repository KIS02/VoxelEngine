#include <cmath>
#include <iostream>
#include <stdexcept>

#include "render/lighting/Lighting.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }

    ve::SceneLightData sun() {
        ve::SceneLightData light{};
        light.type = ve::LightType::Directional;
        light.directionToLight = ve::Vec3{ 0.0f, 0.0f, 1.0f };
        light.emissionDirection = ve::Vec3{ 0.0f, 0.0f, -1.0f };
        light.ambient = 0.15f;
        light.diffuse = 0.35f;
        return light;
    }

    ve::SceneLightData point() {
        ve::SceneLightData light{};
        light.type = ve::LightType::Point;
        light.position = ve::Vec3{ 0.0f, 0.0f, 2.0f };
        light.range = 6.0f;
        light.ambient = 0.0f;
        light.diffuse = 0.8f;
        return light;
    }

    ve::SceneLightData spot() {
        ve::SceneLightData light = point();
        light.type = ve::LightType::Spot;
        light.emissionDirection = ve::Vec3{ 0.0f, 0.0f, -1.0f };
        light.innerConeDegrees = 15.0f;
        light.outerConeDegrees = 25.0f;
        return light;
    }

    ve::Color32 shade( const ve::LightingSettings& lighting,
                       const ve::Vec3& surfacePosition = ve::Vec3{} ) {
        ve::MaterialData material{};
        material.specularStrength = 0.0f;
        return ve::shadeColorMultiLight(
            ve::makeColor(110, 130, 150, 73), ve::Vec3{ 0.0f, 0.0f, 1.0f },
            surfacePosition, ve::Vec3{ 0.0f, 0.0f, 5.0f }, lighting, material
        );
    }

    void testDirectionalLegacyParity() {
        ve::LightingSettings lighting{};
        const ve::SceneLightData direct = sun();
        lighting.sceneLights.push_back(direct);
        ve::MaterialData material{};
        material.specularStrength = 0.6f;
        material.shininess = 16.0f;
        lighting.directional = ve::DirectionalLight{
            direct.directionToLight, direct.ambient, direct.diffuse, direct.color
        };
        const auto expected = ve::shadeColorBlinnPhong(
            ve::makeColor(100, 140, 180, 73), ve::Vec3{ 0.0f, 0.0f, 1.0f }, ve::Vec3{},
            ve::Vec3{ 0.0f, 0.0f, 5.0f }, lighting.directional, material
        );
        const auto actual = ve::shadeColorMultiLight(
            ve::makeColor(100, 140, 180, 73), ve::Vec3{ 0.0f, 0.0f, 1.0f }, ve::Vec3{},
            ve::Vec3{ 0.0f, 0.0f, 5.0f }, lighting, material
        );
        require(actual == expected, "Single directional light must match legacy Blinn-Phong");
        std::cout << "[PASS] Single directional compatibility and alpha\n";
    }

    void testPointAttenuation() {
        ve::LightingSettings lighting{};
        lighting.sceneLights.push_back(point());
        const auto close = shade(lighting);
        const auto distant = shade(lighting, ve::Vec3{ 0.0f, 0.0f, -2.0f });
        const auto outside = shade(lighting, ve::Vec3{ 0.0f, 0.0f, -7.0f });
        require(ve::getBlue(close) > ve::getBlue(distant), "Point attenuation must decrease with distance");
        require(ve::getRed(outside) == 0 && ve::getGreen(outside) == 0 && ve::getBlue(outside) == 0,
                "Point light must vanish beyond range");
        require(ve::getAlpha(close) == 73 && ve::getAlpha(outside) == 73, "Alpha must be preserved");
        std::cout << "[PASS] Point attenuation, range, and alpha\n";
    }

    void testSpotCone() {
        ve::LightingSettings lighting{};
        lighting.sceneLights.push_back(spot());
        const auto inside = shade(lighting);
        const auto outside = shade(lighting, ve::Vec3{ 2.0f, 0.0f, 0.0f });
        require(ve::getBlue(inside) > 0, "Spotlight center must be illuminated");
        require(ve::getRed(outside) == 0 && ve::getGreen(outside) == 0 && ve::getBlue(outside) == 0,
                "Spotlight outside cone must be dark");
        std::cout << "[PASS] Spotlight cone and falloff\n";
    }

    void testMultiLightColor() {
        ve::LightingSettings lighting{};
        ve::SceneLightData red = sun();
        red.color = ve::LinearRGB{ 1.0f, 0.0f, 0.0f };
        red.ambient = 0.0f;
        ve::SceneLightData green = sun();
        green.color = ve::LinearRGB{ 0.0f, 1.0f, 0.0f };
        green.ambient = 0.0f;
        lighting.sceneLights.push_back(red);
        const auto redOnly = shade(lighting);
        lighting.sceneLights.push_back(green);
        const auto combined = shade(lighting);
        require(ve::getRed(combined) == ve::getRed(redOnly), "Green light must not affect red channel");
        require(ve::getGreen(combined) > ve::getGreen(redOnly), "Green light must add green contribution");
        std::cout << "[PASS] Multi-Light linear color accumulation\n";
    }

    void testNoLightSpecular() {
        ve::LightingSettings lighting{};
        lighting.directional.ambient = 0.0f;
        lighting.directional.diffuse = 0.0f;
        ve::MaterialData material{};
        material.specularStrength = 0.9f;
        const ve::Color32 unlit = ve::shadeColorMultiLight(
            ve::makeColor(100, 120, 150, 73), ve::Vec3{ 0.0f, 0.0f, 1.0f },
            ve::Vec3{}, ve::Vec3{ 0.0f, 0.0f, 5.0f }, lighting, material
        );
        require(ve::getRed(unlit) == 0 && ve::getGreen(unlit) == 0 && ve::getBlue(unlit) == 0,
                "Disabled lights must not leave a specular highlight");
        require(ve::getAlpha(unlit) == 73, "Alpha was not preserved in darkness");
        std::cout << "[PASS] No-light state has no residual specular\n";
    }

    void testValidation() {
        ve::LightingSettings lighting{};
        lighting.sceneLights.push_back(point());
        lighting.sceneLights[0].range = 0.0f;
        bool rejected = false;
        try {
            ve::validateSceneLights(lighting);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected, "Invalid range was accepted");
        lighting.sceneLights.assign(ve::MaxSceneLights + 1, sun());
        rejected = false;
        try {
            ve::validateSceneLights(lighting);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected, "Too many lights were silently truncated");
        std::cout << "[PASS] Invalid light and count validation\n";
    }
}


int main() {
    try {
        testDirectionalLegacyParity();
        testPointAttenuation();
        testSpotCone();
        testMultiLightColor();
        testNoLightSpecular();
        testValidation();
        std::cout << "All MultiLightMath tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
