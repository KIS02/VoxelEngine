#include <cmath>
#include <iostream>
#include <stdexcept>

#include "render/lighting/pbr/PbrLighting.h"


namespace
{
    void require( bool condition, const char* why ) {
        if (!condition)
            throw std::runtime_error(why);
    }

    ve::LightingSettings oneLight() {
        ve::LightingSettings settings{};
        ve::SceneLightData sun{};
        sun.type = ve::LightType::Directional;
        sun.directionToLight = ve::Vec3{ 0.0f, 0.0f, 1.0f };
        sun.intensity = 2.0f;
        sun.ambient = 0.0f;
        settings.sceneLights.push_back(sun);
        return settings;
    }
}


int main() {
    try {
        ve::PbrSurface material{};
        material.worldNormal = ve::Vec3{ 0.0f, 0.0f, 1.0f };
        material.baseColor = ve::LinearRGB{ 0.8f, 0.3f, 0.1f };
        material.metallic = 0.0f;
        material.roughness = 0.5f;
        const ve::LightingSettings light = oneLight();
        const ve::Vec3 eye{ 0.0f, 0.0f, 5.0f };
        const ve::LinearRGB dielectric = ve::shadePbrDirect(material, {}, eye, light);
        require(std::isfinite(dielectric.r) && dielectric.r > 0.0f, "Dielectric response invalid");

        material.metallic = 1.0f;
        const ve::LinearRGB metal = ve::shadePbrDirect(material, {}, eye, light);
        require(std::isfinite(metal.r) && metal.r > 0.0f, "Metallic response invalid");
        require(metal.r != dielectric.r, "Metal must differ from dielectric");

        material.roughness = 1.0f;
        const ve::LinearRGB rough = ve::shadePbrDirect(material, {}, eye, light);
        require(rough.r < metal.r, "Roughness should broaden and lower head-on highlight");

        material.metallic = 0.0f;
        material.roughness = 0.5f;
        material.ao = 1.0f;
        ve::LightingSettings ambient = light;
        ambient.sceneLights[0].diffuse = 0.0f;
        ambient.sceneLights[0].ambient = 0.2f;
        const ve::LinearRGB ambientBright = ve::shadePbrDirect(material, {}, eye, ambient);
        material.ao = 0.0f;
        const ve::LinearRGB ambientDark = ve::shadePbrDirect(material, {}, eye, ambient);
        require(ambientBright.r > ambientDark.r && ambientDark.r == 0.0f, "AO should attenuate ambient");

        material.roughness = -0.1f;
        bool rejected = false;
        try { (void)ve::shadePbrDirect(material, {}, eye, light); }
        catch (const std::invalid_argument&) { rejected = true; }
        require(rejected, "Invalid roughness was accepted");
        std::cout << "[PASS] PBR GGX dielectric, metal, roughness, AO and input validation\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "[FAIL] " << ex.what() << '\n';
        return 1;
    }
}
