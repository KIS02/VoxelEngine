#include <cmath>
#include <iostream>
#include <stdexcept>

#include "assets/texture/Texture2D.h"
#include "render/material/PbrSurfaceSampler.h"


namespace
{
    void require( bool valid, const char* message ) {
        if (!valid)
            throw std::runtime_error(message);
    }
}


int main() {
    try {
        ve::Texture2D base(1, 1, ve::TextureColorSpace::SRGB);
        base.setPixel(0, 0, ve::makeColor(255, 128, 0, 255));
        ve::Texture2D normal(1, 1, ve::TextureColorSpace::Linear);
        normal.setPixel(0, 0, ve::makeColor(128, 128, 255, 255));
        ve::Texture2D rough(1, 1, ve::TextureColorSpace::Linear);
        rough.setPixel(0, 0, ve::makeColor(128, 255, 255, 255));
        ve::Texture2D metallic(1, 1, ve::TextureColorSpace::Linear);
        metallic.setPixel(0, 0, ve::makeColor(255, 0, 0, 255));
        ve::Texture2D ao(1, 1, ve::TextureColorSpace::Linear);
        ao.setPixel(0, 0, ve::makeColor(128, 0, 0, 255));

        ve::MaterialData material{};
        material.texture = &base;
        material.normalMap = &normal;
        material.roughnessMap = &rough;
        material.metallicMap = &metallic;
        material.aoMap = &ao;
        material.roughnessFactor = 1.0f;
        material.metallicFactor = 1.0f;

        const ve::Vec3 n{ 0.0f, 0.0f, 1.0f };
        const auto basis = ve::tangentFrameForTriangle(
            { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
            { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, n
        );
        require(basis.valid, "Valid UVs failed to produce a tangent basis");
        const auto pixel = ve::samplePbrSurface(material, {}, n, basis);
        require(pixel.baseColor.g > 0.20f && pixel.baseColor.g < 0.23f, "BaseColor must be sRGB decoded");
        require(pixel.roughness > 0.49f && pixel.roughness < 0.51f, "Roughness channel sampled incorrectly");
        require(pixel.metallic == 1.0f, "Metallic channel sampled incorrectly");
        require(pixel.ao > 0.49f && pixel.ao < 0.51f, "AO channel sampled incorrectly");
        require(pixel.worldNormal.z > 0.999f, "Flat normal map must not tilt the normal");

        const auto degenerate = ve::tangentFrameForTriangle(
            { 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
            {}, {}, {}, n
        );
        require(!degenerate.valid, "Degenerate UV must have a safe fallback");
        const auto fallback = ve::samplePbrSurface(material, {}, n, degenerate);
        require(fallback.worldNormal.z == 1.0f, "Degenerate TBN must use geometry normal");
        std::cout << "[PASS] PBR five maps, linear channels, normal TBN and degeneracy\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
