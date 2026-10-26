#pragma once

#include "math/Vec2.h"
#include "math/Vec3.h"
#include "math/Color.h"


namespace ve
{
    struct RasterVertex
    {
        Vec2 position{};
        float depth = 0.0f;

        Color32 color = makeColor(255, 255, 255);

        Vec2 uv{};
        float inverseW = 1.0f;
        Vec2 uvOverW{};

        Vec3 normal{};
        Vec3 normalOverW{};

        Vec3 worldPosition{};
        Vec3 worldPositionOverW{};
    };
}