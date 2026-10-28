#include "render/backends/software/resources/HdrColorBuffer.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>

#include "render/backends/software/FrameBuffer.h"


namespace ve
{
    HdrColorBuffer::HdrColorBuffer( int width, int height ) {
        resize(width, height);
    }


    void HdrColorBuffer::resize( int width, int height ) {
        if (width <= 0 || height <= 0)
            throw std::invalid_argument("HDR buffer dimensions must be positive");

        width_ = width;
        height_ = height;
        pixels_.assign(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), HdrPixel{});
    }


    void HdrColorBuffer::clear( const HdrPixel& color ) {
        std::fill(pixels_.begin(), pixels_.end(), color);
    }


    void HdrColorBuffer::setPixel( int x, int y, const HdrPixel& value ) {
        if (x < 0 || x >= width_ || y < 0 || y >= height_)
            return;

        pixels_[static_cast<std::size_t>(y) * width_ + x] = value;
    }


    HdrPixel HdrColorBuffer::pixel( int x, int y ) const {
        if (x < 0 || x >= width_ || y < 0 || y >= height_)
            return HdrPixel{};

        return pixels_[static_cast<std::size_t>(y) * width_ + x];
    }


    void HdrColorBuffer::resolve( FrameBuffer& target, const ToneMappingSettings& settings ) const {
        if (target.width() != width_ || target.height() != height_)
            throw std::invalid_argument("HDR and SDR framebuffer dimensions differ");

        validateToneMapping(settings);
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                const HdrPixel value = pixel(x, y);
                target.setPixel(x, y, toneMapToSrgb(value.color, value.alpha, settings));
            }
        }
    }


    int HdrColorBuffer::width() const { return width_; }
    int HdrColorBuffer::height() const { return height_; }
}
