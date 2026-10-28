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

    class TestVisibility final : public ve::ShadowVisibility {
        public:
            mutable int calls = 0;

            float visibility( std::uint64_t, const ve::Vec3&, const ve::Vec3&,
                              const ve::Vec3& ) const override {
                ++calls;
                return 0.4f;
            }
    };

    void testPreparedPathMatchesValidatedPath() {
        ve::LightingSettings lighting{};
        ve::SceneLightData sun{};
        sun.objectId = 10;
        sun.type = ve::LightType::Directional;
        sun.directionToLight = ve::Vec3{ -0.3f, 1.0f, 0.2f };
        sun.emissionDirection = -sun.directionToLight;
        sun.ambient = 0.2f;
        sun.diffuse = 0.7f;
        sun.castsShadows = true;
        lighting.sceneLights.push_back(sun);

        ve::SceneLightData point{};
        point.objectId = 11;
        point.type = ve::LightType::Point;
        point.position = ve::Vec3{ 3.0f, 5.0f, 2.0f };
        point.range = 20.0f;
        point.diffuse = 0.8f;
        point.castsShadows = false;
        lighting.sceneLights.push_back(point);

        ve::MaterialData material{};
        material.specularStrength = 0.25f;
        material.shininess = 24.0f;
        ve::validateSceneLights(lighting);
        ve::validateMaterialLighting(material);

        const ve::Vec3 normal{ 0.0f, 1.0f, 0.0f };
        const ve::Vec3 camera{ 1.0f, 6.0f, 10.0f };
        const TestVisibility shadow{};
        for (int channel = 0; channel < 8; ++channel) {
            const ve::Color32 sample = ve::makeColor(20 + channel * 27, 45 + channel * 19, 90, 173);
            const ve::Vec3 world{ 0.15f * channel, 1.0f, 0.2f * channel };
            const ve::LinearRGB expected = ve::shadeLinearMultiLight(
                sample, normal, world, camera, lighting, material, &shadow
            );
            const ve::LinearRGB actual = ve::shadeLinearMultiLightValidated(
                sample, normal, world, camera, lighting, material, &shadow
            );
            require(std::fabs(expected.r - actual.r) < 1.0e-6f &&
                    std::fabs(expected.g - actual.g) < 1.0e-6f &&
                    std::fabs(expected.b - actual.b) < 1.0e-6f,
                    "Software prepared lighting changes output");
        }
        require(shadow.calls == 16, "Only the shadow-casting light should sample shadows");
    }

    void testPublicPathStillValidates() {
        ve::LightingSettings lighting{};
        ve::MaterialData material{};
        material.specularStrength = -1.0f;
        bool rejected = false;
        try {
            (void)ve::shadeLinearMultiLight(ve::makeColor(255, 255, 255),
                ve::Vec3{ 0.0f, 1.0f, 0.0f }, {}, {}, lighting, material);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected, "Validated public lighting path must reject bad materials");
    }
}


int main() {
    try {
        testPreparedPathMatchesValidatedPath();
        testPublicPathStillValidates();
        std::cout << "[PASS] Software lighting fast path and validation contract\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
