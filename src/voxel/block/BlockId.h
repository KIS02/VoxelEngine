#pragma once

#include <cstdint>



namespace ve
{
    enum class BlockId : std::uint8_t {
        Air = 0,
        Grass,
        Dirt,
        Stone
    };



    constexpr bool isSolid( BlockId type ) {
        return type != BlockId::Air;
    }
}