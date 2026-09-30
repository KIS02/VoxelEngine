#pragma once

#include "render/Color.h"

#include <cstdint>
#include <vector>


namespace ve
{

class FrameBuffer
{
public:

    FrameBuffer(
        int width,
        int height
    );


    int width() const;
    int height() const;


    void clear(Color32 color);


    void setPixel(
        int x,
        int y,
        Color32 color
    );


    Color32 pixel(
        int x,
        int y
    ) const;


    Color32* data();

    const Color32* data() const;


private:

    int width_;
    int height_;

    std::vector<Color32> pixels_;
};

}