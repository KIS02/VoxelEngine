#include <cmath>
#include <iostream>
#include <stdexcept>
#include <utility>

#include "assets/material/MaterialData.h"
#include "assets/mesh/MeshData.h"
#include "render/color/ColorSpace.h"
#include "render/frame/RenderFrame.h"
#include "render/lighting/Lighting.h"
#include "render/shadow/ShadowFrameAdapter.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }

    ve::MeshData ceiling() {
        const ve::Color32 white = ve::makeColor(255, 255, 255);
        std::vector<ve::Vertex> vertices{
            { ve::Vec3{ -2.0f, 2.0f, -2.0f }, white, {}, { 0.0f, 1.0f, 0.0f } },
            { ve::Vec3{  2.0f, 2.0f, -2.0f }, white, {}, { 0.0f, 1.0f, 0.0f } },
            { ve::Vec3{  2.0f, 2.0f,  2.0f }, white, {}, { 0.0f, 1.0f, 0.0f } },
            { ve::Vec3{ -2.0f, 2.0f,  2.0f }, white, {}, { 0.0f, 1.0f, 0.0f } }
        };
        return ve::MeshData{ std::move(vertices), { 0, 1, 2, 0, 2, 3 } };
    }

    void testShadowsPreserveAmbientAndAlpha() {
        const ve::MeshData block = ceiling();
        ve::RenderFrame frame{};
        frame.cameraPosition = ve::Vec3{ 0.0f, 0.0f, 8.0f };
        frame.draws.push_back(ve::RenderDraw{ &block, ve::Mat4::identity(), 1, 7, nullptr });

        ve::SceneLightData light{};
        light.objectId = 42;
        light.type = ve::LightType::Directional;
        light.emissionDirection = ve::Vec3{ 0.0f, -1.0f, 0.0f };
        light.directionToLight = ve::Vec3{ 0.0f, 1.0f, 0.0f };
        light.ambient = 0.15f;
        light.diffuse = 0.8f;
        light.castsShadows = true;
        frame.lighting.sceneLights.push_back(light);

        const ve::ShadowFrame shadows = ve::ShadowFrameAdapter::build(frame);
        require(shadows.lights.size() == 1, "Shadow frame was not generated");
        require(shadows.lights.front().objectId == 42, "SceneLight ID mapping changed");

        ve::MaterialData material{};
        material.specularStrength = 0.0f;
        const ve::Color32 color = ve::makeColor(200, 160, 120, 73);
        const ve::Vec3 origin{};
        const ve::Vec3 up{ 0.0f, 1.0f, 0.0f };
        const ve::Color32 illuminated = ve::shadeColorMultiLight(
            color, up, origin, frame.cameraPosition, frame.lighting, material
        );
        const ve::Color32 shadowed = ve::shadeColorMultiLight(
            color, up, origin, frame.cameraPosition, frame.lighting, material, &shadows
        );
        require(ve::getRed(shadowed) < ve::getRed(illuminated), "Direct diffuse should be shadowed");
        require(ve::getRed(shadowed) > 0, "Ambient should remain visible inside shadows");
        require(ve::getAlpha(shadowed) == 73, "Shadow changes alpha");

        frame.lighting.sceneLights.front().castsShadows = false;
        const ve::ShadowFrame disabled = ve::ShadowFrameAdapter::build(frame);
        require(disabled.lights.empty(), "Non-casting lights must not allocate Shadow Maps");
        std::cout << "[PASS] Shadowed Blinn-Phong, ambient, alpha and light toggle\n";
    }
}


int main() {
    try {
        testShadowsPreserveAmbientAndAlpha();
        std::cout << "All ShadowLightingIntegrationTests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
