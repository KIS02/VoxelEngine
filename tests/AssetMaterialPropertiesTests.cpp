#include <cmath>
#include <cstdint>
#include <exception>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "assets/AssetManager.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }


    bool nearlyEqual( float a, float b ) {
        return std::fabs(a - b) < 0.00001f;
    }


    template <typename Callback>
    void requireInvalidArgument( Callback callback, const char* message ) {
        bool rejected = false;

        try {
            callback();
        } catch (const std::invalid_argument&) {
            rejected = true;
        }

        require(rejected, message);
    }


    void testMaterialProperties() {
        ve::AssetManager assets{};

        const std::uint64_t textureId = assets.addTexture(ve::Texture2D(1, 1));

        const std::uint64_t defaultId = assets.addMaterial(textureId);
        const std::uint64_t matteId = assets.addMaterial(textureId, 0.0f, 32.0f);
        const std::uint64_t glossyId = assets.addMaterial(textureId, 0.8f, 64.0f);

        const ve::MaterialData& defaultMaterial = assets.material(defaultId);
        const ve::MaterialData& matteMaterial = assets.material(matteId);
        const ve::MaterialData& glossyMaterial = assets.material(glossyId);

        require(defaultId != matteId && matteId != glossyId,
                "Material IDs must be unique");

        require(nearlyEqual(defaultMaterial.specularStrength, 0.35f) &&
                nearlyEqual(defaultMaterial.shininess, 32.0f),
                "Default Material properties are incorrect");

        require(nearlyEqual(matteMaterial.specularStrength, 0.0f),
                "Matte Material must have zero specular strength");

        require(nearlyEqual(glossyMaterial.specularStrength, 0.8f) &&
                nearlyEqual(glossyMaterial.shininess, 64.0f),
                "Glossy Material properties are incorrect");

        require(defaultMaterial.texture == &assets.texture(textureId) &&
                matteMaterial.texture == &assets.texture(textureId) &&
                glossyMaterial.texture == &assets.texture(textureId),
                "Materials must reference the registered Texture");

        std::cout << "[PASS] Default and custom Material properties\n";
    }


    void testInvalidProperties() {
        ve::AssetManager assets{};
        const std::uint64_t textureId = assets.addTexture(ve::Texture2D(1, 1));

        requireInvalidArgument([&] {
            assets.addMaterial(textureId, -0.1f, 32.0f);
        }, "Negative specular strength must be rejected");

        requireInvalidArgument([&] {
            assets.addMaterial(textureId, 1.1f, 32.0f);
        }, "Specular strength greater than one must be rejected");

        requireInvalidArgument([&] {
            assets.addMaterial(textureId, 0.5f, 0.0f);
        }, "Zero shininess must be rejected");

        requireInvalidArgument([&] {
            assets.addMaterial(textureId, std::numeric_limits<float>::quiet_NaN(), 32.0f);
        }, "NaN specular strength must be rejected");

        requireInvalidArgument([&] {
            assets.addMaterial(textureId, 0.5f, std::numeric_limits<float>::infinity());
        }, "Infinite shininess must be rejected");

        const std::uint64_t validId = assets.addMaterial(textureId);

        require(validId == 1,
                "Invalid Material requests must not consume IDs");

        std::cout << "[PASS] Invalid Material property validation\n";
    }


    void testStableResourceReferences() {
        ve::AssetManager assets{};

        const std::uint64_t textureId = assets.addTexture(ve::Texture2D(1, 1));
        const std::uint64_t materialId = assets.addMaterial(textureId, 0.7f, 48.0f);

        const ve::Texture2D* originalTexture = &assets.texture(textureId);
        const ve::MaterialData* originalMaterial = &assets.material(materialId);

        for (int i = 0; i < 100; ++i) {
            assets.addTexture(ve::Texture2D(1, 1));
            assets.addMaterial(textureId);
        }

        require(&assets.texture(textureId) == originalTexture,
                "Texture object address must remain stable");

        require(&assets.material(materialId) == originalMaterial,
                "Material object address must remain stable");

        require(assets.material(materialId).texture == originalTexture,
                "Material Texture pointer must remain valid");

        std::cout << "[PASS] Stable Material and Texture references\n";
    }
}


int main() {
    try {
        testMaterialProperties();
        testInvalidProperties();
        testStableResourceReferences();

        std::cout << "All AssetMaterialProperties tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
