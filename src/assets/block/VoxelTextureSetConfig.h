
#pragma once

#include <array>
#include <filesystem>


namespace ve
{
    struct VoxelMaterialTextureFiles
    {
        std::filesystem::path baseColor;
        std::filesystem::path normal;
        std::filesystem::path roughness;
        std::filesystem::path metallic;
        std::filesystem::path ao;
    };


    struct VoxelTextureSetConfig
    {
        std::filesystem::path root;
        std::array<VoxelMaterialTextureFiles, 4> materials{};
    };


    inline std::array<VoxelTextureSetConfig, 2> voxelTextureSetConfigs() {
        return {{
            {
                "assets/textures/voxel_pbr/",
                {{
                    {
                        "grass_top/grass_top_Basecolor.png",
                        "grass_top/grass_top_Normal.png",
                        "grass_top/grass_top_Roughness.png",
                        "grass_top/grass_top_Metallic.png",
                        "grass_top/grass_top_AO.png"
                    },
                    {
                        "grass_side/grass_side_Basecolor.png",
                        "grass_side/grass_side_Normal.png",
                        "grass_side/grass_side_Roughness.png",
                        "grass_side/grass_side_Metallic.png",
                        "grass_side/grass_side_AO.png"
                    },
                    {
                        "dirt/dirt_Basecolor.png",
                        "dirt/dirt_Normal.png",
                        "dirt/dirt_Roughness.png",
                        "dirt/dirt_Metallic.png",
                        "dirt/dirt_AO.png"
                    },
                    {
                        "stone/stone_Basecolor.png",
                        "stone/stone_Normal.png",
                        "stone/stone_Roughness.png",
                        "stone/stone_Metallic.png",
                        "stone/stone_AO.png"
                    }
                }}
            },
            {
                "assets/textures/voxel_pbr_realistic/",
                {{
                    {
                        "grass_top/grass_top_Basecolor.png",
                        "grass_top/grass_top_Normal.png",
                        "grass_top/grass_top_Roughness.png",
                        "grass_top/grass_top_Metallic.png",
                        "grass_top/grass_top_AO.png"
                    },
                    {
                        "grass_side/grass_side_Basecolor.png",
                        "grass_side/grass_side_Normal.png",
                        "grass_side/grass_side_Roughness.png",
                        "grass_side/grass_side_Metallic.png",
                        "grass_side/grass_side_AO.png"
                    },
                    {
                        "dirt/dirt_Basecolor.png",
                        "dirt/dirt_Normal.png",
                        "dirt/dirt_Roughness.png",
                        "dirt/dirt_Metallic.png",
                        "dirt/dirt_AO.png"
                    },
                    {
                        "stone/stone_Basecolor.png",
                        "stone/stone_Normal.png",
                        "stone/stone_Roughness.png",
                        "stone/stone_Metallic.png",
                        "stone/stone_AO.png"
                    }
                }}
            }
        }};
    }
}
