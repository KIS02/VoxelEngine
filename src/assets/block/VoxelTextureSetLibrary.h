#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <map>
#include <stdexcept>

#include "assets/AssetManager.h"
#include "assets/block/BlockDefinition.h"
#include "assets/block/VoxelTextureSetConfig.h"


namespace ve
{
    inline std::map<BlockMaterialKey, std::uint64_t> createVoxelTextureSetMaterials(
        AssetManager& assets, const std::map<BlockMaterialKey, std::uint64_t>& defaults,
        const VoxelTextureSetConfig& config ) {

        const std::array<BlockMaterialKey, 4> keys{
            BlockMaterialKey::GrassTop, BlockMaterialKey::GrassSide,
            BlockMaterialKey::Dirt, BlockMaterialKey::Stone
        };

        if (!std::filesystem::is_directory(config.root)) {
            std::cout << "[TextureSet] Missing directory: " << config.root.string()
                      << " | using default materials\n";
            return defaults;
        }

        std::map<BlockMaterialKey, std::uint64_t> result = defaults;
        std::size_t loadedFiles = 0;

        for (std::size_t i = 0; i < keys.size(); ++i) {
            const VoxelMaterialTextureFiles& files = config.materials[i];
            const MaterialData& original = assets.material(defaults.at(keys[i]));

            const auto exists = [&config]( const std::filesystem::path& relative ) {
                return !relative.empty() && std::filesystem::is_regular_file(config.root / relative);
            };

            if (!exists(files.baseColor) && !exists(files.normal) && !exists(files.roughness) &&
                !exists(files.metallic) && !exists(files.ao))
                continue;

            if (original.texture == nullptr)
                throw std::logic_error("Default voxel material has no BaseColor texture");

            const auto load = [&assets, &config, &exists, &loadedFiles](
                const std::filesystem::path& relative, TextureColorSpace colorSpace,
                std::uint64_t fallbackId ) {
                if (!exists(relative))
                    return fallbackId;

                const std::filesystem::path path = config.root / relative;
                const std::uint64_t id = assets.loadTexture(path, colorSpace);
                ++loadedFiles;
                std::cout << "[TextureSet] Loaded " << path.string() << '\n';
                return id;
            };

            const std::uint64_t baseId = load(files.baseColor, TextureColorSpace::SRGB,
                                              original.texture->id());
            const std::uint64_t normalId = load(files.normal, TextureColorSpace::Linear, 0);
            const std::uint64_t roughnessId = load(files.roughness, TextureColorSpace::Linear, 0);
            const std::uint64_t metallicId = load(files.metallic, TextureColorSpace::Linear, 0);
            const std::uint64_t aoId = load(files.ao, TextureColorSpace::Linear, 0);

            const std::uint64_t materialId = assets.addMaterial(
                baseId, original.specularStrength, original.shininess,
                original.diffuseTint, original.specularColor
            );
            const float roughnessFactor = roughnessId != 0 ? 1.0f : 0.8f;
            const float metallicFactor = metallicId != 0 ? 1.0f : 0.0f;
            assets.configurePbrMaterial(materialId, normalId, roughnessId, metallicId, aoId,
                                        roughnessFactor, metallicFactor,
                                        original.aoStrength, original.normalStrength);
            result[keys[i]] = materialId;
        }

        if (loadedFiles == 0)
            std::cout << "[TextureSet] No configured files found: " << config.root.string()
                      << " | using default materials\n";
        return result;
    }
}
