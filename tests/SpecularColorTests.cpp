#include <exception>
#include <iostream>
#include <stdexcept>

#include "render/lighting/SpecularLighting.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    void testSpecularColor() {
        ve::DirectionalLight light{};
        light.directionToLight = ve::Vec3{ 0.0f, 1.0f, 0.0f };
        light.ambient = 0.2f;
        light.diffuse = 0.8f;

        ve::MaterialData material{};
        material.specularStrength = 0.4f;
        material.shininess = 32.0f;

        const ve::Color32 baseColor = ve::makeColor(100, 120, 150, 73);

        const ve::Color32 result = ve::shadeColorBlinnPhong(
            baseColor,
            ve::Vec3{ 0.0f, 1.0f, 0.0f },
            ve::Vec3{ 0.0f, 0.0f, 0.0f },
            ve::Vec3{ 0.0f, 5.0f, 0.0f },
            light, material
        );

        require(ve::getRed(result) == 202, "Incorrect red channel");
        require(ve::getGreen(result) == 222, "Incorrect green channel");
        require(ve::getBlue(result) == 252, "Incorrect blue channel");
        require(ve::getAlpha(result) == 73, "Alpha must remain unchanged");

        std::cout << "[PASS] Specular RGB calculation\n";
    }


    void testMatteMaterial() {
        ve::DirectionalLight light{};
        light.directionToLight = ve::Vec3{ 0.0f, 1.0f, 0.0f };
        light.ambient = 0.2f;
        light.diffuse = 0.8f;

        ve::MaterialData material{};
        material.specularStrength = 0.0f;

        const ve::Color32 baseColor = ve::makeColor(100, 120, 150, 73);

        const ve::Color32 result = ve::shadeColorBlinnPhong(
            baseColor,
            ve::Vec3{ 0.0f, 1.0f, 0.0f },
            ve::Vec3{},
            ve::Vec3{ 0.0f, 5.0f, 0.0f },
            light, material
        );

        require(result == baseColor,
                "Matte surface facing the light must retain its base color");

        std::cout << "[PASS] Matte Material has no specular highlight\n";
    }


    void testBackFacingSurface() {
        ve::DirectionalLight light{};
        light.directionToLight = ve::Vec3{ 0.0f, 1.0f, 0.0f };
        light.ambient = 0.2f;
        light.diffuse = 0.8f;

        ve::MaterialData material{};
        material.specularStrength = 0.8f;

        const ve::Color32 result = ve::shadeColorBlinnPhong(
            ve::makeColor(100, 120, 150, 73),
            ve::Vec3{ 0.0f, -1.0f, 0.0f },
            ve::Vec3{},
            ve::Vec3{ 0.0f, 5.0f, 0.0f },
            light, material
        );

        require(ve::getRed(result) == 20, "Incorrect ambient red");
        require(ve::getGreen(result) == 24, "Incorrect ambient green");
        require(ve::getBlue(result) == 30, "Incorrect ambient blue");
        require(ve::getAlpha(result) == 73, "Alpha changed unexpectedly");

        std::cout << "[PASS] Back-facing surface receives only ambient lighting\n";
    }


    void testCameraMovement() {
        ve::DirectionalLight light{};
        light.directionToLight = ve::Vec3{ 0.0f, 1.0f, 0.0f };
        light.ambient = 0.2f;
        light.diffuse = 0.8f;

        ve::MaterialData material{};
        material.specularStrength = 0.8f;
        material.shininess = 32.0f;

        const ve::Color32 baseColor = ve::makeColor(100, 120, 150, 73);
        const ve::Vec3 normal{ 0.0f, 1.0f, 0.0f };
        const ve::Vec3 position{ 0.0f, 0.0f, 0.0f };

        const ve::Color32 front = ve::shadeColorBlinnPhong(
            baseColor, normal, position, ve::Vec3{ 0.0f, 5.0f, 0.0f },
            light, material
        );

        const ve::Color32 side = ve::shadeColorBlinnPhong(
            baseColor, normal, position, ve::Vec3{ 5.0f, 0.0f, 0.0f },
            light, material
        );

        require(ve::getRed(front) > ve::getRed(side),
                "Specular highlight must change with camera direction");

        require(ve::getAlpha(front) == 73 && ve::getAlpha(side) == 73,
                "Camera movement must not affect alpha");

        std::cout << "[PASS] Camera direction changes highlight\n";
    }


    void testColorClamping() {
        ve::DirectionalLight light{};
        light.directionToLight = ve::Vec3{ 0.0f, 1.0f, 0.0f };
        light.ambient = 0.2f;
        light.diffuse = 0.8f;

        ve::MaterialData material{};
        material.specularStrength = 0.8f;

        const ve::Color32 result = ve::shadeColorBlinnPhong(
            ve::makeColor(240, 250, 255, 73),
            ve::Vec3{ 0.0f, 1.0f, 0.0f },
            ve::Vec3{},
            ve::Vec3{ 0.0f, 5.0f, 0.0f },
            light, material
        );

        require(ve::getRed(result) == 255, "Red must be clamped");
        require(ve::getGreen(result) == 255, "Green must be clamped");
        require(ve::getBlue(result) == 255, "Blue must be clamped");
        require(ve::getAlpha(result) == 73, "Alpha must remain unchanged");

        std::cout << "[PASS] Specular color clamping\n";
    }
}


int main() {
    try {
        testSpecularColor();
        testMatteMaterial();
        testBackFacingSurface();
        testCameraMovement();
        testColorClamping();

        std::cout << "All SpecularColor tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}