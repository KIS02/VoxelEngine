#include <iostream>
#include <stdexcept>

#include "assets/AssetManager.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }
}


int main() {
    try {
        ve::AssetManager assets;
        ve::Texture2D base(1, 1, ve::TextureColorSpace::SRGB);
        base.setPixel(0, 0, ve::makeColor(150, 160, 170));
        const std::uint64_t baseId = assets.addTexture(std::move(base));
        const std::uint64_t materialA = assets.addMaterial(baseId);
        const std::uint64_t materialB = assets.addMaterial(baseId);

        ve::Texture2D normal(1, 1, ve::TextureColorSpace::Linear);
        normal.setPixel(0, 0, ve::makeColor(128, 128, 255));
        const std::uint64_t normalId = assets.addTexture(std::move(normal));
        assets.configurePbrMaterial(materialA, normalId, 0, 0, 0, 0.4f, 0.0f);
        require(assets.material(materialA).normalMap == &assets.texture(normalId),
                "Normal map was not resolved through AssetManager");
        require(assets.material(materialB).normalMap == nullptr,
                "Independent Material sharing BaseColor inherited an unrelated map");
        require(assets.material(materialA).texture == assets.material(materialB).texture,
                "Shared BaseColor pointer was modified");

        bool failed = false;
        try {
            assets.configurePbrMaterial(materialA, baseId, 0, 0, 0);
        } catch (const std::invalid_argument&) {
            failed = true;
        }
        require(failed, "sRGB Normal data should be rejected");
        require(assets.material(materialA).normalMap == &assets.texture(normalId),
                "Failed update must not change existing Material");
        std::cout << "[PASS] PBR assets, shared BaseColor, Linear validation, rollback\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
