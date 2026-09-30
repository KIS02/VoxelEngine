#include "render/FrameBuffer.h"

#include <algorithm>
#include <cstddef>


namespace ve
{

FrameBuffer::FrameBuffer(
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


int FrameBuffer::width() const
{
    return width_;
}


int FrameBuffer::height() const
{
    return height_;
}


void FrameBuffer::clear(Color32 color)
{
    std::fill(
        pixels_.begin(),
        pixels_.end(),
        color
    );
}


void FrameBuffer::setPixel(
    int x,
    int y,
    Color32 color
)
{
    if (
        x < 0 ||
        x >= width_ ||
        y < 0 ||
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


    pixels_[index] = color;
}


Color32 FrameBuffer::pixel(
    int x,
    int y
) const
{
    if (
        x < 0 ||
        x >= width_ ||
        y < 0 ||
        y >= height_
    )
    {
        return 0;
    }


    const std::size_t index =
        static_cast<std::size_t>(y)
        *
        static_cast<std::size_t>(width_)
        +
        static_cast<std::size_t>(x);


    return pixels_[index];
}


Color32* FrameBuffer::data()
{
    return pixels_.data();
}


const Color32* FrameBuffer::data() const
{
    return pixels_.data();
}

}