#pragma once

#include <cstdint>


namespace ve
{

using Color32 = std::uint32_t;


constexpr Color32 makeColor(
    std::uint8_t red,
    std::uint8_t green,
    std::uint8_t blue,
    std::uint8_t alpha = 255
)
{
    return
        (static_cast<Color32>(alpha) << 24)
        |
        (static_cast<Color32>(red) << 16)
        |
        (static_cast<Color32>(green) << 8)
        |
        static_cast<Color32>(blue);
}


constexpr std::uint8_t getRed(
    Color32 color
)
{
    return static_cast<std::uint8_t>(
        (color >> 16)
        &
        0xFF
    );
}


constexpr std::uint8_t getGreen(
    Color32 color
)
{
    return static_cast<std::uint8_t>(
        (color >> 8)
        &
        0xFF
    );
}


constexpr std::uint8_t getBlue(
    Color32 color
)
{
    return static_cast<std::uint8_t>(
        color
        &
        0xFF
    );
}


constexpr std::uint8_t getAlpha(
    Color32 color
)
{
    return static_cast<std::uint8_t>(
        (color >> 24)
        &
        0xFF
    );
}

}