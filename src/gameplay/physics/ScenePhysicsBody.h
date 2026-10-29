#pragma once

#include <cmath>
#include <stdexcept>

#include "gameplay/physics/CollisionAabb.h"


namespace ve
{
    struct ScenePhysicsBody
    {
        Vec3 halfExtents{ 0.5f, 0.5f, 0.5f };
        Vec3 velocity{};
        bool solid = true;

        CollisionAabb boundsAt( const Vec3& position ) const {
            if (!std::isfinite(halfExtents.x) || !std::isfinite(halfExtents.y) ||
                !std::isfinite(halfExtents.z) || halfExtents.x <= 0.0f ||
                halfExtents.y <= 0.0f || halfExtents.z <= 0.0f)
                throw std::invalid_argument("Invalid ScenePhysicsBody half extents");
            const CollisionAabb result{ position - halfExtents, position + halfExtents };
            result.validate();
            return result;
        }
    };
}
