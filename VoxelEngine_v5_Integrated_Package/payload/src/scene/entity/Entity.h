#pragma once

#include <optional>

#include "scene/entity/EntityId.h"
#include "scene/entity/GridPlacement.h"
#include "scene/object/SceneObjectId.h"


namespace ve
{
    enum class EntityKind {
        Generic,
        Player,
        Monster,
        InteractiveObject
    };


    struct Entity
    {
        EntityId id{};
        EntityKind kind = EntityKind::Generic;
        SceneObjectId visualObject{};
        bool active = true;
        std::optional<GridPlacement> gridPlacement{};
    };
}
