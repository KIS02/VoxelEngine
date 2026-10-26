#pragma once

#include "math/LinearRGB.h"
#include "math/Vec3.h"


namespace ve
{
    struct DirectionalLight
    {
        Vec3 directionToLight{ 0.0f, 1.0f, 0.0f };
        float ambient = 0.25f;
        float diffuse = 0.75f;

        LinearRGB color{ 1.0f, 1.0f, 1.0f };
    };


    float lightingFactor( const Vec3& normal, const DirectionalLight& light );
}