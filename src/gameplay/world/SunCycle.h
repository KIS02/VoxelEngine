#pragma once

#include "gameplay/world/WorldClock.h"
#include "math/LinearRGB.h"
#include "math/Vec3.h"


namespace ve
{
    struct SunCycleState
    {
        Vec3 rotationDegrees{};
        LinearRGB color{};
        float intensity = 1.0f;
        float ambient = 0.25f;
        float diffuse = 0.75f;
        bool castsShadows = false;
        bool daylight = false;
    };


    class SunCycle {
        public:
            static SunCycleState sample( const WorldClock& clock, bool shadowsEnabled );
    };
}
