#pragma once

#include "math/Vec3.h"


namespace ve
{
    struct PlayerMovementCommand
    {
        float forward = 0.0f;
        float strafe = 0.0f;
        Vec3 viewForward{ 0.0f, 0.0f, -1.0f };
        bool jump = false;
        bool sprint = false;
    };
}
