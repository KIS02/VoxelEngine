#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "assets/material/MaterialData.h"
#include "math/Color.h"
#include "render/color/ColorSpace.h"
#include "render/lighting/Lighting.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    void expectInvalidMaterial( const ve::MaterialData& material ) {
        bool rejected = false;

        try {
            ve::validateMaterialLighting(material);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected, "Invalid material properties must be rejected");
    }


    void testSrgbConversion() {
        for (int value = 0; value <= 255; ++value) {
            const std::uint8_t component = static_cast<std::uint8_t>(value);
            const ve::Color32 original = ve::makeColor(component, component, component, 73);
            const ve::Color32 restored = ve::encodeSrgb(ve::decodeSrgb(original), 73);
            require(restored == original, "ColorSpace round trip failed");
        }

        const ve::LinearRGB half = ve::decodeSrgb(ve::makeColor(128, 128, 128)) * 0.5f;
        require(ve::getRed(ve::encodeSrgb(half)) == 92, "Half light must be computed in linear space");
        std::cout << "[PASS] sRGB conversion and linear half-light\n";
    }


    void testWhiteCompatibility() {
        ve::MaterialData material{};
        material.specularStrength = 0.0f;

        const ve::DirectionalLight light{
            ve::Vec3{ 0.0f, 0.0f, 1.0f }, 0.2f, 0.8f
        };

        const ve::Color32 base = ve::makeColor(100, 120, 150, 73);
        const ve::Color32 shaded = ve::shadeColorBlinnPhong(
            base, ve::Vec3{ 0.0f, 0.0f, 1.0f }, ve::Vec3{},
            ve::Vec3{ 0.0f, 0.0f, 5.0f }, light, material
        );

        require(shaded == base, "White light with unit intensity must preserve albedo");
        require(ve::shadeColorLambert(base, ve::Vec3{ 0.0f, 0.0f, 1.0f }, light) == base,
                "Lambert white compatibility failed");
        std::cout << "[PASS] Neutral defaults and alpha preservation\n";
    }


    void testColoredLight() {
        ve::MaterialData material{};
        material.specularStrength = 0.0f;

        ve::DirectionalLight light{
            ve::Vec3{ 0.0f, 0.0f, 1.0f }, 0.2f, 0.8f
        };

        light.color = ve::LinearRGB{ 1.0f, 0.0f, 0.0f };

        const ve::Color32 result = ve::shadeColorBlinnPhong(
            ve::makeColor(128, 128, 128, 73), ve::Vec3{ 0.0f, 0.0f, 1.0f },
            ve::Vec3{}, ve::Vec3{ 0.0f, 0.0f, 5.0f }, light, material
        );

        require(ve::getRed(result) == 128, "Red direct light should preserve red");
        require(ve::getGreen(result) == 59 && ve::getBlue(result) == 59,
                "Neutral ambient must remain in green and blue");
        require(ve::getAlpha(result) == 73, "Light must not modify alpha");
        std::cout << "[PASS] Colored direct light and neutral ambient\n";
    }


    void testDiffuseAndSpecularColors() {
        ve::MaterialData material{};
        material.diffuseTint = ve::LinearRGB{ 0.0f, 0.0f, 1.0f };
        material.specularColor = ve::LinearRGB{ 1.0f, 0.0f, 0.0f };
        material.specularStrength = 0.5f;
        material.shininess = 16.0f;

        const ve::DirectionalLight light{
            ve::Vec3{ 0.0f, 0.0f, 1.0f }, 0.2f, 0.8f
        };

        const ve::Color32 result = ve::shadeColorBlinnPhong(
            ve::makeColor(100, 120, 150, 73), ve::Vec3{ 0.0f, 0.0f, 1.0f },
            ve::Vec3{}, ve::Vec3{ 0.0f, 0.0f, 5.0f }, light, material
        );

        require(ve::getRed(result) == 188, "Colored red specular must encode 0.5 linear to 188");
        require(ve::getGreen(result) == 0, "Disabled green material must stay zero");
        require(ve::getBlue(result) == 150, "Blue diffuse tint should preserve original blue");
        require(ve::getAlpha(result) == 73, "Specular color must not modify alpha");
        std::cout << "[PASS] Material diffuse tint, specular color, alpha\n";
    }


    void testValidation() {
        ve::MaterialData material{};
        material.diffuseTint.r = -0.01f;
        expectInvalidMaterial(material);

        material.diffuseTint.r = 1.0f;
        material.specularColor.g = std::numeric_limits<float>::quiet_NaN();
        expectInvalidMaterial(material);

        material.specularColor.g = 1.0f;
        material.shininess = 0.0f;
        expectInvalidMaterial(material);

        ve::DirectionalLight light{};
        light.color.b = 1.1f;
        bool rejected = false;

        try {
            ve::validateDirectionalLighting(light);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected, "Invalid light color must be rejected");
        std::cout << "[PASS] Invalid material and light colors rejected\n";
    }
}


int main() {
    try {
        testSrgbConversion();
        testWhiteCompatibility();
        testColoredLight();
        testDiffuseAndSpecularColors();
        testValidation();
        std::cout << "All ColoredLighting tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}