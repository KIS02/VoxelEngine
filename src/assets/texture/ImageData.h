#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <vector>


namespace ve
{
    enum class TextureColorSpace {
        SRGB,
        Linear
    };


    struct ImageData
    {
        int width = 0;
        int height = 0;
        TextureColorSpace colorSpace = TextureColorSpace::SRGB;
        std::vector<std::uint8_t> rgba8;


        void validate() const {
            if (width <= 0 || height <= 0)
                throw std::invalid_argument("Image dimensions must be positive");

            const std::size_t w = static_cast<std::size_t>(width);
            const std::size_t h = static_cast<std::size_t>(height);

            if (w > std::numeric_limits<std::size_t>::max() / h ||
                w * h > std::numeric_limits<std::size_t>::max() / 4 ||
                rgba8.size() != w * h * 4)
                throw std::invalid_argument("Image RGBA8 byte count is invalid");
        }
    };
}