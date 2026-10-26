#pragma once

#include "math/LinearRGB.h"


namespace ve
{
    class Texture2D;


    struct MaterialData
    {
        const Texture2D* texture = nullptr;
        float specularStrength = 0.35f;
        float shininess = 32.0f;

        LinearRGB diffuseTint{ 1.0f, 1.0f, 1.0f };
        LinearRGB specularColor{ 1.0f, 1.0f, 1.0f };
    };
}