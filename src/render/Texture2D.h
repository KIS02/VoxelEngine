#pragma once

#include <vector>

#include "math/Vec2.h"
#include "render/Color.h"


namespace ve
{

class Texture2D
{
public:

    Texture2D(
        int width,
        int height
    );


    int width() const;

    int height() const;


    void setPixel(
        int x,
        int y,
        Color32 color
    );


    Color32 pixel(
        int x,
        int y
    ) const;


    Color32 sampleNearest(
        const Vec2& uv
    ) const;


    static Texture2D checkerboard(
        int width,
        int height,
        int cellSize,
        Color32 colorA,
        Color32 colorB
    );


private:

    int width_;

    int height_;

    std::vector<Color32> pixels_;
};

}