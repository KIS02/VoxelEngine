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

std::uint64_t Texture2D::allocateId() {
    static std::atomic<std::uint64_t> nextId{ 1 };
    std::uint64_t candidate = nextId.load(std::memory_order_relaxed);

    while (candidate != 0) {
        if (nextId.compare_exchange_weak(candidate, candidate + 1,
                                         std::memory_order_relaxed))
            return candidate;
    }

    throw std::overflow_error("Texture Resource ID space exhausted");
}


void Texture2D::advanceRevision() {
    if (revision_ == std::numeric_limits<std::uint64_t>::max())
        throw std::overflow_error("Texture revision space exhausted");

    ++revision_;
}


Texture2D::Texture2D( int width, int height )
    : width_(width), height_(height),
      pixels_(static_cast<std::size_t>(width) * static_cast<std::size_t>(height)),
      id_(allocateId()) {
    if (width <= 0 || height <= 0)
        throw std::invalid_argument("Texture dimensions must be positive");
}


Texture2D::Texture2D( const Texture2D& other )
    : width_(other.width_), height_(other.height_), pixels_(other.pixels_),
      id_(allocateId()), revision_(other.revision_) { }


Texture2D::Texture2D( Texture2D&& other )
    : width_(other.width_), height_(other.height_), pixels_(std::move(other.pixels_)),
      id_(other.id_), revision_(other.revision_) {
    const std::uint64_t replacementId = allocateId();
    other.id_ = replacementId;
    other.width_ = 0;
    other.height_ = 0;
    other.revision_ = 1;
}


Texture2D& Texture2D::operator=( const Texture2D& other ) {
    if (this == &other)
        return *this;

    std::vector<Color32> replacement = other.pixels_;
    advanceRevision();
    pixels_ = std::move(replacement);
    width_ = other.width_;
    height_ = other.height_;
    return *this;
}


Texture2D& Texture2D::operator=( Texture2D&& other ) {
    if (this == &other)
        return *this;

    advanceRevision();
    pixels_ = std::move(other.pixels_);
    width_ = other.width_;
    height_ = other.height_;
    other.width_ = 0;
    other.height_ = 0;
    other.revision_ = 1;
    return *this;
}


int Texture2D::width() const
{
    return width_;
}


int Texture2D::height() const
{
    return height_;
}


std::uint64_t Texture2D::id() const {
    return id_;
}


std::uint64_t Texture2D::revision() const {
    return revision_;
}


void Texture2D::setPixel(
    int x,
    int y,
    Color32 color
)
{
    if (
        x < 0 ||
        y < 0 ||
        x >= width_ ||
        y >= height_
    )
    {
        return;
    }


    const std::size_t index =
        static_cast<std::size_t>(y)
        *
        static_cast<std::size_t>(width_)
        +
        static_cast<std::size_t>(x);


    if (pixels_[index] == color)
        return;

    advanceRevision();
    pixels_[index] = color;
}


Color32 Texture2D::pixel(
    int x,
    int y
) const
{
    if (
        x < 0 ||
        y < 0 ||
        x >= width_ ||
        y >= height_
    )
    {
        return makeColor(
            255,
            0,
            255
        );
    }


    const std::size_t index =
        static_cast<std::size_t>(y)
        *
        static_cast<std::size_t>(width_)
        +
        static_cast<std::size_t>(x);


    return pixels_[index];
}


Color32 Texture2D::sampleNearest(
    const Vec2& uv
) const
{
    const float u =
        std::clamp(
            uv.x,
            0.0f,
            1.0f
        );


    const float v =
        std::clamp(
            uv.y,
            0.0f,
            1.0f
        );


    const int x =
        static_cast<int>(
            std::round(
                u
                *
                static_cast<float>(
                    width_ - 1
                )
            )
        );


    const int y =
        static_cast<int>(
            std::round(
                v
                *
                static_cast<float>(
                    height_ - 1
                )
            )
        );


    return pixel(
        x,
        y
    );
}


Texture2D Texture2D::checkerboard(
    int width,
    int height,
    int cellSize,
    Color32 colorA,
    Color32 colorB
)
{
    Texture2D texture{
        width,
        height
    };


    for (
        int y = 0;
        y < height;
        ++y
    )
    {
        for (
            int x = 0;
            x < width;
            ++x
        )
        {
            const int cellX =
                x
                /
                cellSize;


            const int cellY =
                y
                /
                cellSize;


            const bool useColorA =
                (
                    cellX
                    +
                    cellY
                )
                %
                2
                ==
                0;


            texture.setPixel(
                x,
                y,
                useColorA
                    ? colorA
                    : colorB
            );
        }
    }


    return texture;
}

}