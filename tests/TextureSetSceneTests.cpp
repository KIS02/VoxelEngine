#include <array>
#include <cstdint>
#include <cstddef>
#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <stdexcept>
#include <system_error>

#include "assets/AssetManager.h"
#include "assets/block/VoxelTextureSetConfig.h"
#include "assets/block/VoxelTextureSetLibrary.h"
#include "input/ApplicationInput.h"
#include "render/frame/FrameBuilder.h"
#include "scene/demo/DemoScene.h"
#include "ui/UIManager.h"


namespace
{
    void require( bool condition, const char* message ) {
        if (!condition)
            throw std::runtime_error(message);
    }

    void sameGeometry( const ve::RenderFrame& first, const ve::RenderFrame& second ) {
        require(first.draws.size() == second.draws.size(), "Texture cycling changed draw count");
        for (std::size_t i = 0; i < first.draws.size(); ++i) {
            const ve::RenderDraw& a = first.draws[i];
            const ve::RenderDraw& b = second.draws[i];
            require(a.mesh == b.mesh && a.meshId == b.meshId &&
                    a.meshRevision == b.meshRevision,
                    "Texture cycling rebuilt or replaced Mesh assets");
        }
    }

    ve::RenderFrame makeFrame( const ve::DemoScene& scene, const ve::AssetManager& assets,
                                const ve::UIManager& ui ) {
        return ve::FrameBuilder::build(scene, assets, ui, 640, 480, false, ve::RenderBackend::Software);
    }

    void verifyTextureLabel( const ve::RenderFrame& frame, const char* expected ) {
        bool found = false;
        for (const std::string& line : frame.ui.debugOverlay.lines) {
            if (line == expected)
                found = true;
        }
        require(found, "F3 HUD did not show selected Texture Set");
    }

    void testSceneCycle() {
        ve::AssetManager assets{};
        ve::DemoScene scene(assets);
        ve::UIManager ui{};
        ui.setDebugEnabled(true);

        const auto projection = scene.camera().projectionMode();
        const ve::ShadingModel shading = scene.shadingModel();
        const ve::RenderFrame original = makeFrame(scene, assets, ui);
        require(scene.voxelTextureSetIndex() == 0, "Default set must be active initially");
        verifyTextureLabel(original, "VOXEL TEXTURES: DEFAULT");

        const std::array<ve::BlockMaterialKey, 4> keys{
            ve::BlockMaterialKey::GrassTop, ve::BlockMaterialKey::GrassSide,
            ve::BlockMaterialKey::Dirt, ve::BlockMaterialKey::Stone
        };
        std::array<std::uint64_t, 4> defaultIds{};
        for (std::size_t i = 0; i < keys.size(); ++i)
            defaultIds[i] = scene.voxelMaterialId(keys[i]);

        ve::InputActions actions{};
        for (std::uint8_t expected = 1; expected <= 3; ++expected) {
            actions.cycleTextureSet = true;
            scene.update(0.0f, false, ve::MouseDelta{}, actions);
            const std::uint8_t actualIndex = expected % 3;
            require(scene.voxelTextureSetIndex() == actualIndex, "N cycle order is incorrect");
            const ve::RenderFrame frame = makeFrame(scene, assets, ui);
            require(frame.voxelTextureSetIndex == actualIndex, "FrameBuilder lost Texture Set selection");
            sameGeometry(original, frame);
            require(frame.shadingModel == shading, "Texture cycle changed the shading model");
            require(scene.camera().projectionMode() == projection, "Texture cycle changed projection");
            verifyTextureLabel(frame, actualIndex == 0 ? "VOXEL TEXTURES: DEFAULT" :
                                     actualIndex == 1 ? "VOXEL TEXTURES: SET 1" :
                                                        "VOXEL TEXTURES: SET 2");
        }
        for (std::size_t i = 0; i < keys.size(); ++i)
            require(scene.voxelMaterialId(keys[i]) == defaultIds[i], "Default Material ID was not restored");

        std::cout << "[PASS] Default -> Set 1 -> Set 2 -> Default; Draw and Mesh preserved\n";
    }

#ifdef _WIN32
    void testActualPngSetLoading() {
        const auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
        const std::filesystem::path root = std::filesystem::temp_directory_path() /
            ("ve_texture_set_test_" + std::to_string(unique));
        std::filesystem::create_directories(root);

        struct Cleanup
        {
            std::filesystem::path path;
            ~Cleanup() {
                std::error_code error;
                std::filesystem::remove_all(path, error);
            }
        } cleanup{ root };

        const std::array<unsigned char, 70> png{
            0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00,
            0x00, 0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x01,
            0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00, 0x00, 0x1f,
            0x15, 0xc4, 0x89, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x44, 0x41,
            0x54, 0x78, 0x9c, 0x63, 0xa8, 0xe8, 0x59, 0xf0, 0x1f, 0x00,
            0x05, 0xc8, 0x02, 0xa4, 0xae, 0xfa, 0xe7, 0xa2, 0x00, 0x00,
            0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82
        };
        const auto writePng = [&png, &root]( const char* name ) {
            std::ofstream output(root / name, std::ios::binary);
            if (!output)
                throw std::runtime_error("Could not create PNG fixture");
            output.write(reinterpret_cast<const char*>(png.data()),
                         static_cast<std::streamsize>(png.size()));
        };
        writePng("custom_base.png");
        writePng("custom_roughness.png");

        ve::AssetManager assets{};
        const std::uint64_t textureId = assets.addTexture(ve::Texture2D::checkerboard(
            2, 2, 1, ve::makeColor(200, 200, 200), ve::makeColor(50, 50, 50)
        ));
        const std::uint64_t originalId = assets.addMaterial(textureId);
        const std::map<ve::BlockMaterialKey, std::uint64_t> defaults{
            { ve::BlockMaterialKey::GrassTop, originalId },
            { ve::BlockMaterialKey::GrassSide, originalId },
            { ve::BlockMaterialKey::Dirt, originalId },
            { ve::BlockMaterialKey::Stone, originalId }
        };
        ve::VoxelTextureSetConfig config{};
        config.root = root;
        config.materials[0].baseColor = "custom_base.png";
        config.materials[0].roughness = "custom_roughness.png";
        const auto generated = ve::createVoxelTextureSetMaterials(assets, defaults, config);
        const ve::MaterialData& changed = assets.material(generated.at(ve::BlockMaterialKey::GrassTop));
        require(changed.texture != assets.material(originalId).texture,
                "Alternate BaseColor must use a separate Texture asset");
        require(changed.texture->colorSpace() == ve::TextureColorSpace::SRGB,
                "Alternate BaseColor must be sRGB");
        require(changed.roughnessMap != nullptr &&
                changed.roughnessMap->colorSpace() == ve::TextureColorSpace::Linear,
                "Alternate Roughness must be Linear");
        require(changed.roughnessFactor == 1.0f,
                "The texture must control the roughness when a map is present");
        require(generated.at(ve::BlockMaterialKey::GrassSide) == originalId,
                "Missing alternative materials must retain their original IDs");
        std::cout << "[PASS] Windows WIC PNG loading, sRGB/Linear and partial set fallback\n";
    }
#endif

    void testMissingTextureSetFallback() {
        ve::AssetManager assets{};
        const std::uint64_t id = assets.addTexture(ve::Texture2D::checkerboard(
            2, 2, 1, ve::makeColor(255, 255, 255), ve::makeColor(0, 0, 0)
        ));
        const std::uint64_t materialId = assets.addMaterial(id);
        const std::map<ve::BlockMaterialKey, std::uint64_t> defaults{
            { ve::BlockMaterialKey::GrassTop, materialId },
            { ve::BlockMaterialKey::GrassSide, materialId },
            { ve::BlockMaterialKey::Dirt, materialId },
            { ve::BlockMaterialKey::Stone, materialId }
        };
        ve::VoxelTextureSetConfig missing{};
        missing.root = "__absent_voxel_set_928372__";
        const auto generated = ve::createVoxelTextureSetMaterials(assets, defaults, missing);
        require(generated == defaults, "Missing folders must retain original materials");
        std::cout << "[PASS] Missing Texture Set safely falls back to default materials\n";
    }
}


int main() {
    try {
        testSceneCycle();
        testMissingTextureSetFallback();
#ifdef _WIN32
        testActualPngSetLoading();
#endif
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "[FAIL] " << error.what() << '\n';
        return 1;
    }
}
