#pragma once

#include <cstdint>



namespace ve
{
    enum class BlockType : std::uint8_t {
        Air = 0,
        Grass,
        Dirt,
        Stone
    };



    constexpr bool isSolid( BlockType type ) {
        return type != BlockType::Air;
    }
}