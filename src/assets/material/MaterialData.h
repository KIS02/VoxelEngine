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

        // Optional PBR maps; Texture2D ownership stays in AssetManager.
        const Texture2D* normalMap = nullptr;
        const Texture2D* roughnessMap = nullptr;
        const Texture2D* metallicMap = nullptr;
        const Texture2D* aoMap = nullptr;
        float roughnessFactor = 0.7f;
        float metallicFactor = 0.0f;
        float aoStrength = 1.0f;
        float normalStrength = 1.0f;
    };
}