#include "render/DepthBuffer.h"

#include <algorithm>
#include <cstddef>


namespace ve
{

DepthBuffer::DepthBuffer(
    int width,
    int height
)
    :
    width_(width),
    height_(height),
    depths_(
        static_cast<std::size_t>(width)
        *
        static_cast<std::size_t>(height)
    )
{
}


int DepthBuffer::width() const
{
    return width_;
}


int DepthBuffer::height() const
{
    return height_;
}


void DepthBuffer::clear(
    float depth
)
{
    std::fill(
        depths_.begin(),
        depths_.end(),
        depth
    );
}


float DepthBuffer::depth(
    int x,
    int y
) const
{
    const std::size_t index =
        static_cast<std::size_t>(y)
        *
        static_cast<std::size_t>(width_)
        +
        static_cast<std::size_t>(x);


    return depths_[index];
}


void DepthBuffer::setDepth(
    int x,
    int y,
    float depth
)
{
    const std::size_t index =
        static_cast<std::size_t>(y)
        *
        static_cast<std::size_t>(width_)
        +
        static_cast<std::size_t>(x);


    depths_[index] =
        depth;
}

}