#include <cstdint>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <vector>

#include "assets/material/MaterialData.h"
#include "assets/mesh/MeshData.h"
#include "assets/texture/Texture2D.h"
#include "math/Color.h"
#include "render/backends/software/DepthBuffer.h"
#include "render/backends/software/FrameBuffer.h"
#include "render/backends/software/Rasterizer.h"
#include "render/backends/software/Viewport.h"
#include "render/backends/software/pipeline/geometry/SoftwareGeometryPass.h"
#include "render/frame/RenderFrame.h"


namespace
{
    void require( bool condition, const char* description ) {
        if (!condition)
            throw std::runtime_error(description);
    }


    void testFrameLightingSnapshot() {
        constexpr int size = 33;

        ve::FrameBuffer frameBuffer(size, size);
        ve::DepthBuffer depthBuffer(size, size);
        ve::Rasterizer rasterizer(frameBuffer, depthBuffer);
        ve::Viewport viewport(size, size);

        ve::Texture2D texture(1, 1);
        texture.setPixel(0, 0, ve::makeColor(255, 255, 255));

        ve::MaterialData material{};
        material.texture = &texture;
        material.specularStrength = 0.0f;

        const ve::Vec3 normal{ 0.0f, 0.0f, 1.0f };
        const ve::MeshData mesh(
            std::vector<ve::Vertex>{
                ve::Vertex{ ve::Vec3{ -0.8f, -0.8f, 0.0f }, ve::makeColor(255, 255, 255), ve::Vec2{}, normal },
                ve::Vertex{ ve::Vec3{ 0.8f, -0.8f, 0.0f }, ve::makeColor(255, 255, 255), ve::Vec2{}, normal },
                ve::Vertex{ ve::Vec3{ 0.0f, 0.8f, 0.0f }, ve::makeColor(255, 255, 255), ve::Vec2{}, normal }
            },
            std::vector<std::uint32_t>{ 0, 1, 2 }
        );

        ve::RenderFrame frame{};
        frame.draws.push_back(ve::RenderDraw{ &mesh, ve::Mat4::identity(), 1, 1, &material });
        frame.lighting.directional.directionToLight = ve::Vec3{ 0.0f, 0.0f, 1.0f };
        frame.lighting.directional.ambient = 0.0f;
        frame.lighting.directional.diffuse = 1.0f;

        frameBuffer.clear(ve::makeColor(0, 0, 0));
        depthBuffer.clear(1.0f);
        frame.lighting.directional.color = ve::LinearRGB{ 1.0f, 0.0f, 0.0f };
        ve::SoftwareGeometryPass::draw(frame, rasterizer, viewport);
        const ve::Color32 red = frameBuffer.pixel(16, 16);
        require(ve::getRed(red) == 255 && ve::getGreen(red) == 0 && ve::getBlue(red) == 0,
                "Software geometry must read red light from RenderFrame");

        frameBuffer.clear(ve::makeColor(0, 0, 0));
        depthBuffer.clear(1.0f);
        frame.lighting.directional.color = ve::LinearRGB{ 0.0f, 0.0f, 1.0f };
        ve::SoftwareGeometryPass::draw(frame, rasterizer, viewport);
        const ve::Color32 blue = frameBuffer.pixel(16, 16);
        require(ve::getRed(blue) == 0 && ve::getGreen(blue) == 0 && ve::getBlue(blue) == 255,
                "Software geometry must read blue light from RenderFrame");

        std::cout << "[PASS] SoftwareGeometryPass consumes the RenderFrame light snapshot\n";
    }


    void testInvalidDraw() {
        ve::FrameBuffer frameBuffer(4, 4);
        ve::DepthBuffer depthBuffer(4, 4);
        ve::Rasterizer rasterizer(frameBuffer, depthBuffer);
        ve::Viewport viewport(4, 4);

        ve::RenderFrame frame{};
        frame.draws.push_back(ve::RenderDraw{});

        bool rejected = false;
        try {
            ve::SoftwareGeometryPass::draw(frame, rasterizer, viewport);
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected, "Software geometry must reject null meshes");
        std::cout << "[PASS] Invalid RenderDraw is rejected\n";
    }
}


int main() {
    try {
        testFrameLightingSnapshot();
        testInvalidDraw();
        std::cout << "All SoftwareGeometryPipeline tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
