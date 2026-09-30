#pragma once

#include "math/Vec2.h"
#include "math/Vec3.h"

#include "render/Color.h"


namespace ve
{

struct Vertex
{
    Vec3 position{};

    Color32 color =
        makeColor(
            255,
            255,
            255
        );

    Vec2 uv{};
};

}