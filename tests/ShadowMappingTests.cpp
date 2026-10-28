#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "assets/mesh/MeshData.h"
#include "render/shadow/ShadowMapping.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }

    ve::MeshData makeOccluder() {
        const ve::Color32 white = ve::makeColor(255, 255, 255);
        std::vector<ve::Vertex> vertices{
            { ve::Vec3{ -1.5f, 2.0f, -1.5f }, white, ve::Vec2{}, ve::Vec3{ 0.0f, 1.0f, 0.0f } },
            { ve::Vec3{  1.5f, 2.0f, -1.5f }, white, ve::Vec2{}, ve::Vec3{ 0.0f, 1.0f, 0.0f } },
            { ve::Vec3{  1.5f, 2.0f,  1.5f }, white, ve::Vec2{}, ve::Vec3{ 0.0f, 1.0f, 0.0f } },
            { ve::Vec3{ -1.5f, 2.0f,  1.5f }, white, ve::Vec2{}, ve::Vec3{ 0.0f, 1.0f, 0.0f } }
        };
        return ve::MeshData{ std::move(vertices), { 0, 1, 2, 0, 2, 3 } };
    }

    void testOcclusion( ve::ShadowLightKind type ) {
        const ve::MeshData occluder = makeOccluder();
        ve::ShadowLightInput light{};
        light.objectId = static_cast<std::uint64_t>(type) + 1;
        light.type = type;
        light.position = ve::Vec3{ 0.0f, 6.0f, 0.0f };
        light.emissionDirection = ve::Vec3{ 0.0f, -1.0f, 0.0f };
        light.range = 15.0f;
        light.outerConeDegrees = 40.0f;
        light.castsShadows = true;

        ve::ShadowSettings settings{};
        settings.resolution = 192;
        settings.directionalHalfExtent = 12.0f;
        settings.pcfRadius = 0;
        const ve::ShadowFrame frame = ve::ShadowMapping::build(
            { light }, { ve::ShadowDrawInput{ &occluder, ve::Mat4::identity() } },
            ve::Vec3{ 0.0f, 0.0f, 0.0f }, settings
        );

        require(frame.lights.size() == 1, "Expected one shadow light");
        require(frame.lights.front().faces.size() == (type == ve::ShadowLightKind::Point ? 6u : 1u),
                "Incorrect shadow map face count");

        const ve::Vec3 normal{ 0.0f, 1.0f, 0.0f };
        const ve::Vec3 toLight{ 0.0f, 1.0f, 0.0f };
        const float hidden = frame.visibility(light.objectId, ve::Vec3{ 0.0f, 0.0f, 0.0f }, normal, toLight);
        const float lit = frame.visibility(light.objectId, ve::Vec3{ 6.0f, 0.0f, 0.0f }, normal, toLight);
        require(hidden < 0.01f, "Blocked receiver should be shadowed");
        require(lit > 0.99f, "Unblocked receiver should be lit");
        require(frame.visibility(99999, ve::Vec3{}, normal, toLight) == 1.0f,
                "Lights without Shadow maps must remain visible");
        std::cout << "[PASS] Shadow occlusion for type " << static_cast<int>(type) << '\n';
    }

    void testPcfAndValidation() {
        ve::ShadowSettings settings{};
        settings.resolution = 64;
        settings.pcfRadius = 1;
        ve::ShadowLightInput light{};
        light.objectId = 6;
        light.castsShadows = true;
        light.emissionDirection = ve::Vec3{ 0.0f, -1.0f, 0.0f };
        const ve::ShadowFrame empty = ve::ShadowMapping::build({ light }, {}, {}, settings);
        require(empty.lights[0].faces[0].depth(0, 0) == 1.0f, "Empty Shadow map must be fully lit");
        require(empty.visibility(6, {}, { 0, 1, 0 }, { 0, 1, 0 }) > 0.99f,
                "Empty scene must not cast shadows");

        settings.resolution = 0;
        bool rejected = false;
        try {
            (void)ve::ShadowMapping::build({}, {}, {}, settings);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }
        require(rejected, "Invalid Shadow map size must be rejected");
        std::cout << "[PASS] Empty shadow, PCF settings and input validation\n";
    }
}


int main() {
    try {
        testOcclusion(ve::ShadowLightKind::Directional);
        testOcclusion(ve::ShadowLightKind::Spot);
        testOcclusion(ve::ShadowLightKind::Point);
        testPcfAndValidation();
        std::cout << "All ShadowMappingTests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
