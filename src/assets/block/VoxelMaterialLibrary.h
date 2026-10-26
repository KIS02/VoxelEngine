#pragma once

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <map>
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

            return assets.addMaterial(textureId, 0.2f, 24.0f);
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