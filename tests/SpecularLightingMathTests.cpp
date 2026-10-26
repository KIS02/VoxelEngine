#include <cmath>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "render/lighting/Lighting.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    bool nearlyEqual( float a, float b, float epsilon = 0.0001f ) {
        return std::fabs(a - b) <= epsilon;
    }


    void testHeadOnHighlight() {
        ve::MaterialData material{};
        material.specularStrength = 0.8f;
        material.shininess = 32.0f;

        ve::DirectionalLight light{};
        light.directionToLight = ve::Vec3{ 0.0f, 2.0f, 0.0f };

        const float specular = ve::blinnPhongSpecular(
            ve::Vec3{ 0.0f, 1.0f, 0.0f }, ve::Vec3{ 0.0f, 0.0f, 0.0f },
            ve::Vec3{ 0.0f, 5.0f, 0.0f }, light, material
        );

        require(nearlyEqual(specular, 0.8f),
                "Aligned normal and halfway vector must give full strength");

        std::cout << "[PASS] Head-on specular highlight\n";
    }


    void testCameraAngleAndShininess() {
        ve::MaterialData material{};
        material.specularStrength = 0.8f;
        material.shininess = 4.0f;

        ve::DirectionalLight light{};
        light.directionToLight = ve::Vec3{ 0.0f, 1.0f, 0.0f };

        const ve::Vec3 n{ 0.0f, 1.0f, 0.0f };
        const ve::Vec3 p{ 0.0f, 0.0f, 0.0f };
        const ve::Vec3 camera{ 5.0f, 0.0f, 0.0f };

        const float broad = ve::blinnPhongSpecular(n, p, camera, light, material);

        require(nearlyEqual(broad, 0.2f),
                "45 degree halfway vector with power 4 must yield 0.2");

        material.shininess = 32.0f;

        const float narrow = ve::blinnPhongSpecular(n, p, camera, light, material);

        require(narrow < broad,
                "Larger shininess must narrow the highlight");

        std::cout << "[PASS] View angle and shininess\n";
    }


    void testNoHighlightCases() {
        ve::MaterialData material{};
        material.specularStrength = 0.8f;

        ve::DirectionalLight light{};
        light.directionToLight = ve::Vec3{ 0.0f, -1.0f, 0.0f };

        const ve::Vec3 p{ 0.0f, 0.0f, 0.0f };
        const ve::Vec3 camera{ 0.0f, 1.0f, 0.0f };
        const ve::Vec3 n{ 0.0f, 1.0f, 0.0f };

        require(nearlyEqual(ve::blinnPhongSpecular(n, p, camera, light, material), 0.0f),
                "Light behind the surface must produce no highlight");

        light.directionToLight = ve::Vec3{ 0.0f, 1.0f, 0.0f };

        require(nearlyEqual(ve::blinnPhongSpecular(n, p, p, light, material), 0.0f),
                "Camera at surface point must produce no highlight");

        require(nearlyEqual(ve::blinnPhongSpecular(ve::Vec3{}, p, camera, light, material), 0.0f),
                "Zero normal must produce no highlight");

        material.specularStrength = 0.0f;

        require(nearlyEqual(ve::blinnPhongSpecular(n, p, camera, light, material), 0.0f),
                "Zero specular strength must disable highlight");

        std::cout << "[PASS] No-highlight cases\n";
    }


    void testInvalidMaterialProperties() {
        ve::MaterialData material{};
        material.shininess = std::numeric_limits<float>::infinity();

        bool rejected = false;

        try {
            ve::blinnPhongSpecular(
                ve::Vec3{ 0.0f, 1.0f, 0.0f }, ve::Vec3{},
                ve::Vec3{ 0.0f, 1.0f, 0.0f }, ve::DirectionalLight{}, material
            );
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected,
                "Invalid direct Material values must be rejected");

        std::cout << "[PASS] Material validation\n";
    }
}


int main() {
    try {
        testHeadOnHighlight();
        testCameraAngleAndShininess();
        testNoHighlightCases();
        testInvalidMaterialProperties();

        std::cout << "All SpecularLighting math tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}