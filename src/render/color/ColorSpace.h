#pragma once

#include <cstdint>

#include "math/Color.h"
#include "math/LinearRGB.h"


namespace ve
{
    float srgbToLinear( float srgb );
    float linearToSrgb( float linear );

    LinearRGB decodeSrgb( Color32 encodedColor );
    Color32 encodeSrgb( const LinearRGB& linearColor, std::uint8_t alpha = 255 );
}