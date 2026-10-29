#pragma once

#include <cmath>
#include <stdexcept>

#include "gameplay/physics/CollisionAabb.h"


namespace ve
{
    struct PlayerCollider
    {
        float radius = 0.30f;
        float height = 1.80f;
        float eyeHeight = 1.62f;
        float stepHeight = 0.60f;


        void validate() const {
            if (!std::isfinite(radius) || radius <= 0.0f || radius > 2.0f ||
                !std::isfinite(height) || height <= 0.0f || height > 4.0f ||
                !std::isfinite(eyeHeight) || eyeHeight <= 0.0f || eyeHeight > height ||
                !std::isfinite(stepHeight) || stepHeight < 0.0f || stepHeight > 1.0f)
                throw std::invalid_argument("Invalid PlayerCollider");
        }

        CollisionAabb boundsAt( const Vec3& eyePosition ) const {
            const float bottom = eyePosition.y - eyeHeight;
            return CollisionAabb{
                Vec3{ eyePosition.x - radius, bottom, eyePosition.z - radius },
                Vec3{ eyePosition.x + radius, bottom + height, eyePosition.z + radius }
            };
        }
    };
}
