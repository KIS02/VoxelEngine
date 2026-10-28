#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "assets/mesh/MeshData.h"
#include "render/shadow/ShadowMapping.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }

    void testNormalOffsetPreservesRealOcclusion() {
        const ve::Color32 white = ve::makeColor(255, 255, 255);
        std::vector<ve::Vertex> vertices{
            { ve::Vec3{ -2.0f, 2.0f, -2.0f }, white, {}, { 0.0f, 1.0f, 0.0f } },
            { ve::Vec3{  2.0f, 2.0f, -2.0f }, white, {}, { 0.0f, 1.0f, 0.0f } },
            { ve::Vec3{  2.0f, 2.0f,  2.0f }, white, {}, { 0.0f, 1.0f, 0.0f } },
            { ve::Vec3{ -2.0f, 2.0f,  2.0f }, white, {}, { 0.0f, 1.0f, 0.0f } }
        };
        const ve::MeshData blocker{ std::move(vertices), { 0, 1, 2, 0, 2, 3 } };
        ve::ShadowLightInput light{};
        light.objectId = 1;
        light.castsShadows = true;
        light.emissionDirection = ve::Vec3{ 0.0f, -1.0f, 0.0f };
        ve::ShadowSettings settings{};
        settings.resolution = 256;
        settings.normalOffset = 0.05f;
        settings.pcfRadius = 0;
        const ve::ShadowFrame shadows = ve::ShadowMapping::build(
            { light }, { ve::ShadowDrawInput{ &blocker, ve::Mat4::identity() } },
            ve::Vec3{ 0.0f, 0.0f, 0.0f }, settings
        );
        const ve::Vec3 up{ 0.0f, 1.0f, 0.0f };
        const float blocked = shadows.visibility(1, {}, up, up);
        const float clear = shadows.visibility(1, ve::Vec3{ 6.0f, 0.0f, 0.0f }, up, up);
        require(blocked < 0.1f, "Normal offset must not erase a two-block high shadow");
        require(clear > 0.9f, "Normal offset must not create shadows outside blocker");
    }
}


int main() {
    try {
        testNormalOffsetPreservesRealOcclusion();
        std::cout << "[PASS] Shadow receiver normal offset keeps cast shadows\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
