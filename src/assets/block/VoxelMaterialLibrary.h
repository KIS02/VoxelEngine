#pragma once

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <utility>

#include "assets/AssetManager.h"
#include "assets/block/BlockDefinition.h"
#include "math/Color.h"


namespace ve
{
    inline std::map<BlockMaterialKey, std::uint64_t> createVoxelMaterials( AssetManager& assets ) {
        const auto create = [&assets]( const char* filename, Color32 fallback ) {
            const std::filesystem::path path =
                std::filesystem::path("assets") / "textures" / "voxel" / filename;

            std::uint64_t textureId = 0;

            if (std::filesystem::is_regular_file(path)) {
                textureId = assets.loadTexture(path);

                std::cout << "[VoxelMaterial] Loaded " << path.string() << '\n';
            } else {
                textureId = assets.addTexture(Texture2D::checkerboard(
                    16, 16, 4,
                    fallback,
                    makeColor(
                        static_cast<std::uint8_t>(getRed(fallback) * 3 / 4),
                        static_cast<std::uint8_t>(getGreen(fallback) * 3 / 4),
                        static_cast<std::uint8_t>(getBlue(fallback) * 3 / 4)
                    )
                ));

                std::cout << "[VoxelMaterial] Using procedural fallback: "
                          << path.string() << '\n';
            }

            const std::uint64_t materialId = assets.addMaterial(textureId, 0.2f, 24.0f);
            const std::string stem = std::filesystem::path(filename).stem().string();
            const auto loadOptional = [&assets, &stem]( const char* suffix ) {
                const std::filesystem::path mapPath = std::filesystem::path("assets") / "textures" /
                    "voxel" / (stem + suffix + ".png");
                if (!std::filesystem::is_regular_file(mapPath))
                    return std::uint64_t{ 0 };
                return assets.loadTexture(mapPath, TextureColorSpace::Linear);
            };

            const std::uint64_t normalId = loadOptional("_normal");
            const std::uint64_t roughnessId = loadOptional("_roughness");
            const std::uint64_t metallicId = loadOptional("_metallic");
            const std::uint64_t aoId = loadOptional("_ao");
            const float roughnessFactor = roughnessId != 0 ? 1.0f : 0.8f;
            const float metallicFactor = metallicId != 0 ? 1.0f : 0.0f;
            assets.configurePbrMaterial(materialId, normalId, roughnessId, metallicId, aoId,
                                        roughnessFactor, metallicFactor, 1.0f, 1.0f);
            return materialId;
        };

        std::map<BlockMaterialKey, std::uint64_t> result;

        result.emplace(
            BlockMaterialKey::GrassTop,
            create("grass_top.png", makeColor(76, 176, 56))
        );

        result.emplace(
            BlockMaterialKey::GrassSide,
            create("grass_side.png", makeColor(102, 142, 66))
        );

        result.emplace(
            BlockMaterialKey::Dirt,
            create("dirt.png", makeColor(145, 99, 62))
        );

        result.emplace(
            BlockMaterialKey::Stone,
            create("stone.png", makeColor(135, 138, 143))
        );

        return result;
    }
}