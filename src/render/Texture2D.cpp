#include "render/Texture2D.h"

#include <algorithm>
#include <cmath>
#include <cstddef>


namespace ve
{

Texture2D::Texture2D(
    int width,
    int height
)
    :
    width_(width),
    height_(height),
    pixels_(
        static_cast<std::size_t>(width)
        *
        static_cast<std::size_t>(height)
    )
{
}


int Texture2D::width() const
{
    return width_;
}


int Texture2D::height() const
{
    return height_;
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


    pixels_[index] =
        color;
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