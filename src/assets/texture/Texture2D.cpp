#include "assets/texture/Texture2D.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <utility>


namespace ve
{
    namespace
    {
        int validatedImageWidth( const ImageData& image ) {
            image.validate();
            return image.width;
        }

        std::size_t checkedPixelCount( int width, int height ) {
            if (width <= 0 || height <= 0)
                throw std::invalid_argument("Texture dimensions must be positive");

            const std::size_t w = static_cast<std::size_t>(width);
            const std::size_t h = static_cast<std::size_t>(height);

            if (w > std::numeric_limits<std::size_t>::max() / h)
                throw std::overflow_error("Texture pixel count overflow");

            return w * h;
        }
    }


    std::uint64_t Texture2D::allocateId() {
        static std::atomic<std::uint64_t> nextId{ 1 };
        std::uint64_t candidate = nextId.load(std::memory_order_relaxed);

        while (candidate != 0) {
            if (nextId.compare_exchange_weak(candidate, candidate + 1, std::memory_order_relaxed))
                return candidate;
        }

        throw std::overflow_error("Texture Resource ID space exhausted");
    }


    void Texture2D::advanceRevision() {
        if (revision_ == std::numeric_limits<std::uint64_t>::max())
            throw std::overflow_error("Texture revision space exhausted");

        ++revision_;
    }


    Texture2D::Texture2D( int width, int height, TextureColorSpace colorSpace )
        : width_(width), height_(height), pixels_(checkedPixelCount(width, height)),
          id_(allocateId()), colorSpace_(colorSpace) { }


    Texture2D::Texture2D( const ImageData& image )
        : Texture2D(validatedImageWidth(image), image.height, image.colorSpace) {

        for (std::size_t index = 0; index < pixels_.size(); ++index) {
            const std::size_t offset = index * 4;

            pixels_[index] = makeColor(image.rgba8[offset], image.rgba8[offset + 1],
                                       image.rgba8[offset + 2], image.rgba8[offset + 3]);
        }
    }


    Texture2D::Texture2D( const Texture2D& other )
        : width_(other.width_), height_(other.height_), pixels_(other.pixels_),
          id_(allocateId()), revision_(other.revision_), colorSpace_(other.colorSpace_) { }


    Texture2D::Texture2D( Texture2D&& other )
        : width_(other.width_), height_(other.height_), pixels_(std::move(other.pixels_)),
          id_(other.id_), revision_(other.revision_), colorSpace_(other.colorSpace_) {
        const std::uint64_t replacementId = allocateId();

        other.id_ = replacementId;
        other.width_ = 0;
        other.height_ = 0;
        other.revision_ = 1;
        other.colorSpace_ = TextureColorSpace::SRGB;
    }


    Texture2D& Texture2D::operator=( const Texture2D& other ) {
        if (this == &other)
            return *this;

        std::vector<Color32> replacement = other.pixels_;
        advanceRevision();

        pixels_ = std::move(replacement);
        width_ = other.width_;
        height_ = other.height_;
        colorSpace_ = other.colorSpace_;

        return *this;
    }


    Texture2D& Texture2D::operator=( Texture2D&& other ) {
        if (this == &other)
            return *this;

        advanceRevision();

        pixels_ = std::move(other.pixels_);
        width_ = other.width_;
        height_ = other.height_;
        colorSpace_ = other.colorSpace_;

        other.width_ = 0;
        other.height_ = 0;
        other.revision_ = 1;
        other.colorSpace_ = TextureColorSpace::SRGB;

        return *this;
    }


    int Texture2D::width() const {
        return width_;
    }

    int Texture2D::height() const {
        return height_;
    }

    std::uint64_t Texture2D::id() const {
        return id_;
    }

    std::uint64_t Texture2D::revision() const {
        return revision_;
    }

    TextureColorSpace Texture2D::colorSpace() const {
        return colorSpace_;
    }


    void Texture2D::setPixel( int x, int y, Color32 color ) {
        if (x < 0 || y < 0 || x >= width_ || y >= height_)
            return;

        const std::size_t index = static_cast<std::size_t>(y) *
                                  static_cast<std::size_t>(width_) +
                                  static_cast<std::size_t>(x);

        if (pixels_[index] == color)
            return;

        advanceRevision();
        pixels_[index] = color;
    }


    Color32 Texture2D::pixel( int x, int y ) const {
        if (x < 0 || y < 0 || x >= width_ || y >= height_)
            return makeColor(255, 0, 255);

        const std::size_t index = static_cast<std::size_t>(y) *
                                  static_cast<std::size_t>(width_) +
                                  static_cast<std::size_t>(x);

        return pixels_[index];
    }


    Color32 Texture2D::sampleNearest( const Vec2& uv ) const {
        if (width_ <= 0 || height_ <= 0)
            throw std::logic_error("Cannot sample an empty Texture");

        if (!std::isfinite(uv.x) || !std::isfinite(uv.y))
            throw std::invalid_argument("Texture UV must be finite");

        const float u = std::clamp(uv.x, 0.0f, 1.0f);
        const float v = std::clamp(uv.y, 0.0f, 1.0f);

        const int x = std::clamp(
            static_cast<int>(std::floor(u * static_cast<float>(width_))), 0, width_ - 1
        );

        const int y = std::clamp(
            static_cast<int>(std::floor(v * static_cast<float>(height_))), 0, height_ - 1
        );

        return pixel(x, y);
    }


    Texture2D Texture2D::checkerboard( int width, int height, int cellSize,
                                      Color32 colorA, Color32 colorB ) {
        Texture2D texture{ width, height };

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const int cellX = x / cellSize;
                const int cellY = y / cellSize;
                const bool useColorA = (cellX + cellY) % 2 == 0;

                texture.setPixel(x, y, useColorA ? colorA : colorB);
            }
        }

        return texture;
    }
}