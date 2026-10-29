#pragma once

#include <cstdint>
#include <optional>

#include "math/Vec3.h"
#include "gameplay/physics/ScenePhysicsBody.h"
#include "scene/object/SceneObjectId.h"
#include "scene/lighting/LightComponent.h"


namespace ve
{
    struct SceneObject
    {
        SceneObjectId id{};

        Vec3 position{};
        Vec3 rotationDegrees{};
        Vec3 scale{ 1.0f, 1.0f, 1.0f };

        std::uint64_t meshAssetId = 0;
        std::uint64_t materialAssetId = 0;

        std::optional<LightComponent> light{};
        std::optional<ScenePhysicsBody> collision{};
    };
}