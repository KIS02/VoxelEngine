#include "render/color/ColorSpace.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <stdexcept>


namespace ve
{
    float srgbToLinear( float srgb ) {
        if (!std::isfinite(srgb) || srgb < 0.0f || srgb > 1.0f)
            throw std::invalid_argument("sRGB channel must be finite and within [0, 1]");

        if (srgb <= 0.04045f)
            return srgb / 12.92f;

        return std::pow((srgb + 0.055f) / 1.055f, 2.4f);
    }


    float linearToSrgb( float linear ) {
        if (!std::isfinite(linear))
            throw std::invalid_argument("Linear RGB channel must be finite");

        const float value = std::clamp(linear, 0.0f, 1.0f);

        if (value <= 0.0031308f)
            return 12.92f * value;

        return 1.055f * std::pow(value, 1.0f / 2.4f) - 0.055f;
    }


    LinearRGB decodeSrgb( Color32 encodedColor ) {
        constexpr float Inv255 = 1.0f / 255.0f;

        static const std::array<float, 256> decodeTable = []() {
            std::array<float, 256> values{};

            for (std::size_t i = 0; i < values.size(); ++i)
                values[i] = srgbToLinear(static_cast<float>(i) * Inv255);

            return values;
        }();

        return LinearRGB{
            decodeTable[getRed(encodedColor)],
            decodeTable[getGreen(encodedColor)],
            decodeTable[getBlue(encodedColor)]
        };
    }


    Color32 encodeSrgb( const LinearRGB& linearColor, std::uint8_t alpha ) {
        const auto encodeChannel = []( float value ) {
            return static_cast<std::uint8_t>(std::lround(linearToSrgb(value) * 255.0f));
        };

        return makeColor(
            encodeChannel(linearColor.r),
            encodeChannel(linearColor.g),
            encodeChannel(linearColor.b), alpha
        );
    }
}