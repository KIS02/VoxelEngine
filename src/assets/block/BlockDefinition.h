#pragma once

#include <cstdint>

#include "math/Color.h"
#include "voxel/block/BlockId.h"


namespace ve
{
    enum class BlockFace : std::uint8_t {
        Front, Back, Left, Right, Top, Bottom
    };


    enum class BlockMaterialKey : std::uint8_t {
        GrassTop, GrassSide, Dirt, Stone
    };


    struct BlockFaceMaterials
    {
        BlockMaterialKey front = BlockMaterialKey::Dirt;
        BlockMaterialKey back = BlockMaterialKey::Dirt;
        BlockMaterialKey left = BlockMaterialKey::Dirt;
        BlockMaterialKey right = BlockMaterialKey::Dirt;
        BlockMaterialKey top = BlockMaterialKey::Dirt;
        BlockMaterialKey bottom = BlockMaterialKey::Dirt;


        constexpr BlockMaterialKey forFace( BlockFace face ) const {
            switch (face) {
                case BlockFace::Front: return front;
                case BlockFace::Back: return back;
                case BlockFace::Left: return left;
                case BlockFace::Right: return right;
                case BlockFace::Top: return top;
                case BlockFace::Bottom: return bottom;
            }

            return bottom;
        }
    };


    struct BlockDefinition
    {
        BlockId id = BlockId::Air;
        bool solid = false;
        Color32 tint = makeColor(255, 255, 255);
        BlockFaceMaterials materials{};
    };
}