#pragma once

#include <cstdint>

#include "math/LinearRGB.h"
#include "math/Vec3.h"


namespace ve
{
    enum class LightType : std::uint8_t {
        Directional,
        Point,
        Spot
    };


    struct SceneLightData
    {
        std::uint64_t objectId = 0;

        LightType type = LightType::Directional;

        Vec3 position{};
        Vec3 emissionDirection{ 0.0f, 0.0f, -1.0f };
        Vec3 directionToLight{ 0.0f, 0.0f, 1.0f };

        LinearRGB color{ 1.0f, 1.0f, 1.0f };

        float intensity = 1.0f;
        float ambient = 0.25f;
        float diffuse = 0.75f;

        float range = 16.0f;
        float innerConeDegrees = 20.0f;
        float outerConeDegrees = 30.0f;

        bool castsShadows = false;
    };
}