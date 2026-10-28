#include <cmath>
#include <iostream>
#include <stdexcept>

#include "render/color/ToneMapping.h"
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
        ve::MaterialData material{};
        material.specularStrength = 0.0f;
        ve::LightingSettings lighting{};
        lighting.sceneLights.clear();
        ve::SceneLightData sun{};
        sun.type = ve::LightType::Directional;
        sun.directionToLight = ve::Vec3{ 0.0f, 0.0f, 1.0f };
        sun.ambient = 0.0f;
        sun.diffuse = 1.0f;
        sun.intensity = 4.0f;
        lighting.sceneLights.push_back(sun);
        const ve::Color32 base = ve::makeColor(255, 255, 255, 73);
        const ve::LinearRGB radiance = ve::shadeLinearMultiLight(
            base, ve::Vec3{ 0.0f, 0.0f, 1.0f }, ve::Vec3{},
            ve::Vec3{ 0.0f, 0.0f, 5.0f }, lighting, material);
        require(std::fabs(radiance.r - 4.0f) < 0.0001f, "Lighting saturated before Tone Mapping");
        require(std::fabs(radiance.g - 4.0f) < 0.0001f, "Green HDR calculation mismatch");
        const ve::ToneMappingSettings tone{ true, 1.0f, ve::ToneMappingOperator::Reinhard };
        const ve::LinearRGB mapped = ve::toneMapLinear(radiance, tone);
        require(std::fabs(mapped.r - 0.8f) < 0.0001f, "HDR Tone Mapping result is wrong");
        std::cout << "[PASS] Full Multi-Light linear radiance > 1 survives shading\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
