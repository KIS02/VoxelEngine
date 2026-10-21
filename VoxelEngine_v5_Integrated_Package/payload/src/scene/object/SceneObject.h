#pragma once

#include <cstdint>

#include "math/Vec3.h"
#include "scene/object/SceneObjectId.h"


namespace ve
{
    struct SceneObject
    {
        SceneObjectId id{};
        Vec3 position{};
        Vec3 rotationDegrees{};
        Vec3 scale{ 1.0f, 1.0f, 1.0f };

        // Resource IDs are references, not owned resource objects.
        std::uint64_t meshAssetId = 0;
        std::uint64_t materialAssetId = 0;
    };
}
