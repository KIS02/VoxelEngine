#pragma once

#include <cstdint>



namespace ve
{
    enum class BlockId : std::uint8_t {
        Air = 0,
        Grass,
        Dirt,
        Stone,
        CraftingTable = 4
    };



    constexpr bool isSolid( BlockId type ) {
        return type != BlockId::Air;
    }
}