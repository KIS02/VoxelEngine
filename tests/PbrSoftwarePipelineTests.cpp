#include <cmath>
#include <iostream>
#include <stdexcept>

#include "assets/texture/Texture2D.h"
#include "render/backends/software/DepthBuffer.h"
#include "render/backends/software/FrameBuffer.h"
#include "render/backends/software/Rasterizer.h"
#include "render/backends/software/resources/HdrColorBuffer.h"
#include "render/lighting/pbr/PbrLighting.h"
#include "render/material/PbrSurfaceSampler.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }
}


int main() {
    try {
        ve::FrameBuffer sdr(32, 32);
        ve::DepthBuffer depth(32, 32);
        ve::HdrColorBuffer hdr(32, 32);
        ve::Rasterizer rasterizer(sdr, depth);
        depth.clear(1.0f);
        hdr.clear(ve::HdrPixel{});

        ve::Texture2D base(1, 1, ve::TextureColorSpace::SRGB);
        base.setPixel(0, 0, ve::makeColor(160, 180, 200, 73));
        ve::Texture2D rough(1, 1, ve::TextureColorSpace::Linear);
        rough.setPixel(0, 0, ve::makeColor(170, 0, 0, 255));
        ve::MaterialData material{};
        material.texture = &base;
        material.roughnessMap = &rough;
        material.roughnessFactor = 1.0f;
        material.metallicFactor = 0.0f;

        ve::LightingSettings lighting{};
        ve::SceneLightData sun{};
        sun.type = ve::LightType::Directional;
        sun.directionToLight = ve::Vec3{ 0.0f, 0.0f, 1.0f };
        sun.emissionDirection = ve::Vec3{ 0.0f, 0.0f, -1.0f };
        lighting.sceneLights = { sun };

        const ve::Vec3 camera{ 0.0f, 0.0f, 5.0f };
        ve::RasterVertex a{}, b{}, c{};
        a.position = ve::Vec2{ 2.0f, 2.0f };
        b.position = ve::Vec2{ 2.0f, 26.0f };
        c.position = ve::Vec2{ 26.0f, 2.0f };
        a.depth = b.depth = c.depth = 0.5f;
        a.inverseW = b.inverseW = c.inverseW = 1.0f;
        a.normalOverW = b.normalOverW = c.normalOverW = ve::Vec3{ 0.0f, 0.0f, 1.0f };
        a.worldPositionOverW = b.worldPositionOverW = c.worldPositionOverW = ve::Vec3{};
        a.uvOverW = b.uvOverW = c.uvOverW = ve::Vec2{ 0.5f, 0.5f };

        rasterizer.drawTriangleTextured(a, b, c, base, &lighting.directional, &material,
                                        &camera, &lighting, nullptr, &hdr, true, ve::ShadingModel::Pbr);
        const ve::PbrSurface surface = ve::samplePbrSurface(material, ve::Vec2{ 0.5f, 0.5f },
                                                             ve::Vec3{ 0.0f, 0.0f, 1.0f }, {});
        const ve::LinearRGB expected = ve::shadePbrDirect(surface, {}, camera, lighting);
        const ve::HdrPixel actual = hdr.pixel(8, 8);
        require(std::fabs(expected.r - actual.color.r) < 0.0001f, "PBR Rasterizer red mismatch");
        require(std::fabs(expected.g - actual.color.g) < 0.0001f, "PBR Rasterizer green mismatch");
        require(std::fabs(expected.b - actual.color.b) < 0.0001f, "PBR Rasterizer blue mismatch");
        require(actual.alpha == 73, "PBR alpha was not preserved");
        require(depth.depth(8, 8) < 1.0f, "PBR triangle did not update depth");
        std::cout << "[PASS] Software PBR pixel path, depth, linear HDR and alpha\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
