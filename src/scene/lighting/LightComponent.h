#pragma once

#include "math/LinearRGB.h"
#include "scene/lighting/SceneLightData.h"


namespace ve
{
    struct LightComponent
    {
        LightType type = LightType::Directional;

        bool enabled = true;
        bool castsShadows = true;

        LinearRGB color{ 1.0f, 1.0f, 1.0f };

        float intensity = 1.0f;
        float ambient = 0.25f;
        float diffuse = 0.75f;

        float range = 16.0f;
        float innerConeDegrees = 20.0f;
        float outerConeDegrees = 30.0f;
    };
}