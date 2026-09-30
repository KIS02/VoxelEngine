#pragma once

#include "math/Vec3.h"
#include "render/Color.h"
#include "render/RasterVertex.h"


namespace ve
{

class Viewport
{
public:

    Viewport(
        int width,
        int height
    )
        :
        width_(width),
        height_(height)
    {
    }


    RasterVertex transform(
        const Vec3& ndcPosition,
        Color32 color
    ) const
    {
        const float screenX =
            (ndcPosition.x + 1.0f)
            *
            0.5f
            *
            static_cast<float>(width_);


        const float screenY =
            (1.0f - ndcPosition.y)
            *
            0.5f
            *
            static_cast<float>(height_);


        return RasterVertex{
            Vec2{
                screenX,
                screenY
            },

            ndcPosition.z,

            color
        };
    }


    int width() const
    {
        return width_;
    }


    int height() const
    {
        return height_;
    }


private:

    int width_;

    int height_;
};

}